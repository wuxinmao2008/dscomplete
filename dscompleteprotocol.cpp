// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dscompleteprotocol.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

namespace DsComplete::Internal {

CompletionContext completionContext(const QString &text,
                                    int cursorPosition,
                                    int maximumPromptCharacters,
                                    int maximumSuffixCharacters)
{
    const int position = qBound(0, cursorPosition, text.size());
    const QString prompt = text.left(position).right(qMax(0, maximumPromptCharacters));
    const QString suffix = text.mid(position).left(qMax(0, maximumSuffixCharacters));
    return {prompt, suffix};
}

QJsonObject buildFimPayload(const QString &model,
                            const CompletionContext &context,
                            int maximumTokens)
{
    return {
        {"model", model},
        {"prompt", context.prompt},
        {"suffix", context.suffix},
        {"max_tokens", qMax(1, maximumTokens)},
        {"stream", false},
    };
}

QUrl completionUrl(const QString &baseUrl)
{
    QUrl url = QUrl::fromUserInput(baseUrl.trimmed());
    QString path = url.path();
    while (path.endsWith('/'))
        path.chop(1);
    if (path.endsWith("/beta/completions"))
        return url;
    if (path.endsWith("/beta"))
        path += "/completions";
    else
        path += "/beta/completions";
    url.setPath(path);
    return url;
}

bool isAllowedEndpoint(const QUrl &url)
{
    if (!url.isValid() || url.host().isEmpty())
        return false;
    if (url.scheme() == "https")
        return true;
    if (url.scheme() != "http")
        return false;

    const QString host = url.host().toLower();
    if (host == "localhost")
        return true;
    QHostAddress address;
    return address.setAddress(host) && address.isLoopback();
}

QStringList parseCompletionResponse(const QByteArray &data, QString *errorMessage)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage)
            *errorMessage = parseError.errorString();
        return {};
    }

    const QJsonArray choices = document.object().value("choices").toArray();
    QStringList completions;
    for (const QJsonValue &value : choices) {
        const QString text = value.toObject().value("text").toString();
        if (!text.trimmed().isEmpty())
            completions.append(text);
    }
    if (completions.isEmpty() && errorMessage)
        *errorMessage = QStringLiteral("The response contains no completion text.");
    return completions;
}

} // namespace DsComplete::Internal