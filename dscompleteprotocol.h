// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QJsonObject>
#include <QStringList>
#include <QUrl>

namespace DsComplete::Internal {

struct CompletionContext
{
    QString prompt;
    QString suffix;
};

CompletionContext completionContext(const QString &text,
                                    int cursorPosition,
                                    int maximumPromptCharacters,
                                    int maximumSuffixCharacters);
QJsonObject buildFimPayload(const QString &model,
                            const CompletionContext &context,
                            int maximumTokens);
QUrl completionUrl(const QString &baseUrl);
bool isAllowedEndpoint(const QUrl &url);
QStringList parseCompletionResponse(const QByteArray &data, QString *errorMessage = nullptr);

} // namespace DsComplete::Internal