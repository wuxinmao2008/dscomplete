// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dscompleteprotocol.h"

#include <QJsonDocument>
#include <QtTest>

using namespace DsComplete::Internal;

class DsCompleteProtocolTest final : public QObject
{
    Q_OBJECT

private slots:
    void contextIsTruncated()
    {
        const CompletionContext context = completionContext("0123456789", 6, 4, 3);
        QCOMPARE(context.prompt, "2345");
        QCOMPARE(context.suffix, "678");
    }

    void buildsFimPayload()
    {
        const QJsonObject payload = buildFimPayload("deepseek-chat", {"before", "after"}, 128);
        QCOMPARE(payload.value("model").toString(), "deepseek-chat");
        QCOMPARE(payload.value("prompt").toString(), "before");
        QCOMPARE(payload.value("suffix").toString(), "after");
        QCOMPARE(payload.value("max_tokens").toInt(), 128);
        QCOMPARE(payload.value("stream").toBool(), false);
    }

    void normalizesUrl()
    {
        QCOMPARE(completionUrl("https://api.deepseek.com").toString(),
                 "https://api.deepseek.com/beta/completions");
        QCOMPARE(completionUrl("https://example.test/root/beta").toString(),
                 "https://example.test/root/beta/completions");
    }

    void validatesEndpoint()
    {
        QVERIFY(isAllowedEndpoint(QUrl("https://api.deepseek.com/beta/completions")));
        QVERIFY(isAllowedEndpoint(QUrl("http://127.0.0.1:8080/beta/completions")));
        QVERIFY(!isAllowedEndpoint(QUrl("http://example.com/beta/completions")));
    }

    void parsesChoices()
    {
        const QByteArray response = R"({"choices":[{"text":"first"},{"text":"second"}]})";
        QCOMPARE(parseCompletionResponse(response), QStringList({"first", "second"}));
    }

    void rejectsMalformedResponse()
    {
        QString error;
        QVERIFY(parseCompletionResponse("not json", &error).isEmpty());
        QVERIFY(!error.isEmpty());
    }
};

QTEST_MAIN(DsCompleteProtocolTest)

#include "tst_dscompleteprotocol.moc"