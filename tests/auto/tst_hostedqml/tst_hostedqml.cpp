/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

#include <QtTest>

#include <QFile>
#include <QJSValue>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QScopedPointer>

class tst_hostedqml : public QObject
{
    Q_OBJECT

private slots:
    void contextMenuRouting();
    void navigation();
    void privateThumbnailCapture();
    void selectCancellation();
    void sessionOwnership();
    void snapshotApplicationTriggers();

private:
    static QString readResource(const QString &path);
    static QString functionSource(const QString &path, const QString &name);
    static void runJavaScriptTest(const QString &scriptPath,
                                  const QStringList &functions);
};

QString tst_hostedqml::readResource(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}

QString tst_hostedqml::functionSource(const QString &path, const QString &name)
{
    const QString source = readResource(path);
    const QRegularExpression expression(
                QStringLiteral("^    function %1\\([^\\n]*\\) \\{[\\s\\S]*?^    \\}")
                .arg(QRegularExpression::escape(name)),
                QRegularExpression::MultilineOption);
    return expression.match(source).captured(0).trimmed();
}

void tst_hostedqml::runJavaScriptTest(const QString &scriptPath,
                                      const QStringList &functions)
{
    QString source = QStringLiteral(
                "function check(value, message) {\n"
                "    if (!value) throw new Error(message || 'check failed');\n"
                "}\n"
                "function equal(actual, expected, message) {\n"
                "    if (actual !== expected) {\n"
                "        throw new Error((message || 'values differ') + ': '"
                " + actual + ' !== ' + expected);\n"
                "    }\n"
                "}\n"
                "function deepEqual(actual, expected, message) {\n"
                "    equal(JSON.stringify(actual), JSON.stringify(expected), message);\n"
                "}\n");
    for (const QString &function : functions) {
        QVERIFY2(!function.isEmpty(), "Production QML function was not found");
        source.append(function);
        source.append(QLatin1Char('\n'));
    }
    source.append(readResource(scriptPath));

    QQmlEngine engine;
    const QJSValue result = engine.evaluate(source, scriptPath);
    QVERIFY2(!result.isError(), qPrintable(QStringLiteral("%1:%2: %3")
             .arg(result.property(QStringLiteral("fileName")).toString())
             .arg(result.property(QStringLiteral("lineNumber")).toInt())
             .arg(result.toString())));
    QVERIFY(result.toBool());
}

void tst_hostedqml::contextMenuRouting()
{
    runJavaScriptTest(QStringLiteral(":/context-menu.js"),
                      QStringList() << functionSource(
                          QStringLiteral(":/BrowserPage.qml"),
                          QStringLiteral("presentHostedPopup")));
}

void tst_hostedqml::navigation()
{
    runJavaScriptTest(QStringLiteral(":/navigation.js"),
                      QStringList()
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("goBack"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("goForward")));
}

void tst_hostedqml::privateThumbnailCapture()
{
    runJavaScriptTest(QStringLiteral(":/private-thumbnail.js"),
                      QStringList()
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("prunePrivateTabGrabs"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("requestPrivateCover"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("cancelPrivateCoverCapture"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("beginHostedTabViewThumbnailCapture"))
                      << functionSource(QStringLiteral(":/Overlay.qml"),
                                        QStringLiteral("finishHostedTabViewCapture")));
}

void tst_hostedqml::selectCancellation()
{
    runJavaScriptTest(QStringLiteral(":/select-cancellation.js"),
                      QStringList() << functionSource(
                          QStringLiteral(":/BrowserPage.qml"),
                          QStringLiteral("cancelHostedSelect")));
}

void tst_hostedqml::sessionOwnership()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/SessionHarness.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> harness(component.create());
    QVERIFY2(harness, qPrintable(component.errorString()));

    QVariant failure;
    QVERIFY(QMetaObject::invokeMethod(harness.data(), "runTest",
                                      Q_RETURN_ARG(QVariant, failure)));
    QCOMPARE(failure.toString(), QString());
}

void tst_hostedqml::snapshotApplicationTriggers()
{
    const QString browserPage = readResource(QStringLiteral(":/BrowserPage.qml"));
    QCOMPARE(browserPage.count(QStringLiteral(
                 "tabSession.applyRuntimeSnapshot(false, chromeView)")), 1);
    QVERIFY(!browserPage.contains(QStringLiteral("function applyRuntimeSnapshot(")));

    const QString captivePortal = readResource(
                QStringLiteral(":/CaptivePortalView.qml"));
    QCOMPARE(captivePortal.count(QStringLiteral(
                 "viewSession.applyRuntimeSnapshot(false)")), 1);
}

QTEST_MAIN(tst_hostedqml)
#include "tst_hostedqml.moc"
