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

    void buildsSuggestionText_data()
    {
        QTest::addColumn<QString>("linePrefix");
        QTest::addColumn<QString>("lineSuffix");
        QTest::addColumn<QString>("completion");
        QTest::addColumn<QString>("expected");

        QTest::newRow("incremental-at-line-end")
            << QString("setting.resetPoweron = ") << QString()
            << QString("settings.value(\"resetPoweron\", 0).toFloat();")
            << QString("setting.resetPoweron = settings.value(\"resetPoweron\", 0).toFloat();");
        QTest::newRow("complete-line")
            << QString("setting.resetPoweron = ") << QString()
            << QString("setting.resetPoweron = settings.value(\"resetPoweron\", 0).toFloat();")
            << QString("setting.resetPoweron = settings.value(\"resetPoweron\", 0).toFloat();");
        QTest::newRow("complete-line-with-indentation")
            << QString("    setting.resetPoweron = ") << QString()
            << QString("setting.resetPoweron = settings.value(\"resetPoweron\", 0).toFloat();")
            << QString("    setting.resetPoweron = settings.value(\"resetPoweron\", 0).toFloat();");
        QTest::newRow("preserves-line-suffix")
            << QString("foo(") << QString(");") << QString("bar") << QString("foo(bar);");
        QTest::newRow("preserves-space-before-line-suffix")
            << QString("return ") << QString("value;") << QString("await ")
            << QString("return await value;");
        QTest::newRow("deduplicates-punctuation-overlap")
            << QString("foo(") << QString(");") << QString("bar)") << QString("foo(bar);");
        QTest::newRow("deduplicates-complete-suffix")
            << QString("return ") << QString("value;") << QString("await value;")
            << QString("return await value;");
        QTest::newRow("complete-line-includes-suffix")
            << QString("foo(") << QString(");") << QString("foo(bar);") << QString("foo(bar);");
        QTest::newRow("keeps-single-alphanumeric-boundary")
            << QString("foo") << QString("return") << QString("bar") << QString("foobarreturn");
        QTest::newRow("multiline-preserves-suffix")
            << QString("foo(") << QString(");") << QString("bar,\nbaz")
            << QString("foo(bar,\nbaz);");
    }

    void buildsSuggestionText()
    {
        QFETCH(QString, linePrefix);
        QFETCH(QString, lineSuffix);
        QFETCH(QString, completion);
        QFETCH(QString, expected);

        QCOMPARE(completionSuggestionText(linePrefix, lineSuffix, completion), expected);
    }
};

QTEST_MAIN(DsCompleteProtocolTest)

#include "tst_dscompleteprotocol.moc"