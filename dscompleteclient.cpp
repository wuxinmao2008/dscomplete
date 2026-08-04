// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dscompleteclient.h"

#include "dscompleteprotocol.h"
#include "dscompletesettings.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/textsuggestion.h>

#include <utils/networkaccessmanager.h>
#include <utils/textutils.h>

#include <QJsonDocument>
#include <QLoggingCategory>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace DsComplete::Internal {

static Q_LOGGING_CATEGORY(dsCompleteLog, "qtc.dscomplete", QtWarningMsg)
static constexpr qint64 maximumResponseSize = 1024 * 1024;

DsCompleteClient::DsCompleteClient(QObject *parent)
    : QObject(parent)
{
    auto open = [this](IDocument *document) {
        if (auto *textDocument = qobject_cast<TextDocument *>(document))
            openDocument(textDocument);
    };
    connect(EditorManager::instance(), &EditorManager::documentOpened, this, open);
    connect(EditorManager::instance(), &EditorManager::documentClosed, this, [this](IDocument *document) {
        if (auto *textDocument = qobject_cast<TextDocument *>(document))
            closeDocument(textDocument);
    });

    for (IDocument *document : DocumentModel::openedDocuments())
        open(document);

    settings().enabled.addOnChanged(this, [this] {
        if (!settings().enabled())
            cancelAll();
    });
    settings().autoComplete.addOnChanged(this, [this] {
        if (!settings().autoComplete()) {
            for (const ScheduledRequest &request : std::as_const(m_scheduledRequests))
                request.timer->stop();
        }
    });
}

void DsCompleteClient::openDocument(TextDocument *document)
{
    if (m_documentConnections.contains(document))
        return;
    const QMetaObject::Connection connection = connect(
        document,
        &TextDocument::contentsChangedWithPosition,
        this,
        [this, document](int position, int, int charactersAdded) {
            if (!settings().autoComplete())
                return;
            auto *editor = BaseTextEditor::currentTextEditor();
            if (!editor || editor->document() != document)
                return;
            TextEditorWidget *widget = editor->editorWidget();
            const int cursorPosition = widget->textCursor().position();
            if (cursorPosition < position || cursorPosition > position + charactersAdded)
                return;
            scheduleRequest(widget);
        });
    m_documentConnections.insert(document, connection);
}

void DsCompleteClient::closeDocument(TextDocument *document)
{
    const auto connection = m_documentConnections.take(document);
    disconnect(connection);
    if (auto *editor = BaseTextEditor::currentTextEditor(); editor && editor->document() == document)
        cancelRequest(editor->editorWidget());
}

bool DsCompleteClient::canRequest(TextEditorWidget *editor) const
{
    if (!editor || editor->isReadOnly())
        return false;
    if (!isDsCompleteEnabled())
        return false;
    const MultiTextCursor cursors = editor->multiTextCursor();
    return !cursors.hasMultipleCursors() && !cursors.hasSelection() && !editor->suggestionVisible();
}

void DsCompleteClient::scheduleRequest(TextEditorWidget *editor)
{
    cancelRequest(editor);
    if (!canRequest(editor))
        return;

    auto it = m_scheduledRequests.find(editor);
    if (it == m_scheduledRequests.end()) {
        auto *timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this, editor] {
            const auto it = m_scheduledRequests.constFind(editor);
            if (settings().autoComplete()
                && it != m_scheduledRequests.constEnd()
                && it->cursorPosition == editor->textCursor().position()) {
                requestCompletions(editor);
            }
        });
        connect(editor, &TextEditorWidget::destroyed, this, [this, editor] {
            if (const auto scheduled = m_scheduledRequests.take(editor); scheduled.timer)
                scheduled.timer->deleteLater();
            cancelRequest(editor);
        });
        connect(editor, &TextEditorWidget::cursorPositionChanged, this, [this, editor] {
            cancelRequest(editor);
        });
        it = m_scheduledRequests.insert(editor, {editor->textCursor().position(), timer});
    } else {
        it->cursorPosition = editor->textCursor().position();
    }
    it->timer->start(int(settings().debounceMilliseconds()));
}

void DsCompleteClient::requestCompletions(TextEditorWidget *editor)
{
    cancelRequest(editor);
    if (!canRequest(editor))
        return;

    const quint64 requestId = ++m_nextRequestId;
    m_pendingRequests.insert(editor, requestId);
    settings().apiKey.requestValue([this, editor = QPointer<TextEditorWidget>(editor), requestId](
                                       const Result<QString> &result) {
        if (!editor || m_pendingRequests.value(editor) != requestId)
            return;
        if (!result || result->trimmed().isEmpty()) {
            m_pendingRequests.remove(editor);
            qCWarning(dsCompleteLog) << "DeepSeek API key is not configured.";
            return;
        }
        if (!canRequest(editor)) {
            m_pendingRequests.remove(editor);
            return;
        }
        sendRequest(editor, requestId, *result);
    });
}

void DsCompleteClient::sendRequest(TextEditorWidget *editor,
                                   quint64 requestId,
                                   const QString &apiKey)
{
    const QUrl url = completionUrl(settings().baseUrl());
    if (!isAllowedEndpoint(url)) {
        m_pendingRequests.remove(editor);
        qCWarning(dsCompleteLog) << "DeepSeek endpoint must use HTTPS or a loopback HTTP address.";
        return;
    }

    const QTextCursor cursor = editor->textCursor();
    const CompletionContext context = completionContext(
        editor->document()->toPlainText(),
        cursor.position(),
        int(settings().maximumPromptCharacters()),
        int(settings().maximumSuffixCharacters()));
    const QJsonObject payload = buildFimPayload(
        settings().model(), context, int(settings().maximumTokens()));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());
    request.setTransferTimeout(30000);

    QNetworkReply *reply = NetworkAccessManager::instance()->post(
        request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    reply->setProperty("dscompleteResponseTooLarge", false);
    connect(reply, &QNetworkReply::readyRead, reply, [reply] {
        if (reply->bytesAvailable() > maximumResponseSize) {
            reply->setProperty("dscompleteResponseTooLarge", true);
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, editor = QPointer(editor), requestId, reply] {
        if (!editor) {
            reply->deleteLater();
            return;
        }
        handleReply(editor, requestId, reply);
    });

    m_pendingRequests.remove(editor);
    m_runningRequests.insert(editor,
                             {reply, requestId, cursor.position(), editor->document()->revision()});
}

void DsCompleteClient::handleReply(TextEditorWidget *editor,
                                   quint64 requestId,
                                   QNetworkReply *reply)
{
    const auto it = m_runningRequests.find(editor);
    if (it == m_runningRequests.end() || it->id != requestId || it->reply != reply) {
        reply->deleteLater();
        return;
    }
    const RunningRequest running = *it;
    m_runningRequests.erase(it);

    const bool responseTooLarge = reply->property("dscompleteResponseTooLarge").toBool()
                                  || reply->bytesAvailable() > maximumResponseSize;
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (responseTooLarge) {
        qCWarning(dsCompleteLog) << "DeepSeek response exceeded the size limit.";
        reply->deleteLater();
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(dsCompleteLog) << "DeepSeek request failed:" << reply->errorString()
                                 << "HTTP status:" << status;
        reply->deleteLater();
        return;
    }

    const QByteArray responseData = reply->readAll();
    reply->deleteLater();
    if (!canRequest(editor)
        || editor->document()->revision() != running.documentRevision
        || editor->textCursor().position() != running.cursorPosition) {
        return;
    }

    QString parseError;
    QStringList completions = parseCompletionResponse(responseData, &parseError);
    if (completions.isEmpty()) {
        qCWarning(dsCompleteLog) << "DeepSeek response could not be used:" << parseError;
        return;
    }

    QList<TextSuggestion::Data> suggestions;
    const QTextCursor cursor = editor->textCursor();
    const Text::Position position{cursor.blockNumber() + 1, cursor.positionInBlock()};
    const Text::Range range{position, position};
    for (QString completion : completions) {
        if (!completion.contains('\n')) {
            while (!completion.isEmpty() && completion.back().isSpace())
                completion.chop(1);
        }
        if (!completion.trimmed().isEmpty())
            suggestions.append({range, position, completion});
    }
    if (!suggestions.isEmpty() && !editor->suggestionVisible()) {
        editor->insertSuggestion(
            std::make_unique<CyclicSuggestion>(suggestions, editor->document()));
    }
}

void DsCompleteClient::cancelRequest(TextEditorWidget *editor)
{
    m_pendingRequests.remove(editor);
    const auto it = m_runningRequests.find(editor);
    if (it == m_runningRequests.end())
        return;
    QPointer<QNetworkReply> reply = it->reply;
    m_runningRequests.erase(it);
    if (reply)
        reply->abort();
}

void DsCompleteClient::cancelAll()
{
    m_pendingRequests.clear();
    const auto editors = m_runningRequests.keys();
    for (TextEditorWidget *editor : editors)
        cancelRequest(editor);
    for (const ScheduledRequest &request : std::as_const(m_scheduledRequests))
        request.timer->stop();
}

} // namespace DsComplete::Internal