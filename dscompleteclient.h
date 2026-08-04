// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QHash>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QNetworkReply;
class QTimer;
QT_END_NAMESPACE

namespace TextEditor {
class TextDocument;
class TextEditorWidget;
}

namespace DsComplete::Internal {

class DsCompleteClient final : public QObject
{
    Q_OBJECT

public:
    explicit DsCompleteClient(QObject *parent = nullptr);

    void requestCompletions(TextEditor::TextEditorWidget *editor);
    void cancelAll();

private:
    struct ScheduledRequest
    {
        int cursorPosition = -1;
        QTimer *timer = nullptr;
    };

    struct RunningRequest
    {
        QPointer<QNetworkReply> reply;
        quint64 id = 0;
        int cursorPosition = -1;
        int documentRevision = -1;
    };

    void openDocument(TextEditor::TextDocument *document);
    void closeDocument(TextEditor::TextDocument *document);
    void scheduleRequest(TextEditor::TextEditorWidget *editor);
    void sendRequest(TextEditor::TextEditorWidget *editor,
                     quint64 requestId,
                     const QString &apiKey);
    void handleReply(TextEditor::TextEditorWidget *editor,
                     quint64 requestId,
                     QNetworkReply *reply);
    void cancelRequest(TextEditor::TextEditorWidget *editor);
    bool canRequest(TextEditor::TextEditorWidget *editor) const;

    QHash<TextEditor::TextDocument *, QMetaObject::Connection> m_documentConnections;
    QHash<TextEditor::TextEditorWidget *, ScheduledRequest> m_scheduledRequests;
    QHash<TextEditor::TextEditorWidget *, RunningRequest> m_runningRequests;
    QHash<TextEditor::TextEditorWidget *, quint64> m_pendingRequests;
    quint64 m_nextRequestId = 0;
};

} // namespace DsComplete::Internal