/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */
#include <QtTest>
#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusMessage>
#include <functional>
#include <intenturl.h>
#include "externalurlhandler.h"

class FileService : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.sailfishos.fileservice")
public:
    int queries = 0;
    int opens = 0;
    bool supported = true;
    bool deferReply = false;
    std::function<void()> reply;
    QString lastUrl;
public slots:
    bool checkUrlSupported(const QString &url) {
        ++queries;
        lastUrl = url;
        if (deferReply) {
            setDelayedReply(true);
            const QDBusMessage request = message();
            const QDBusConnection bus = connection();
            const bool result = supported;
            reply = [request, bus, result]() {
                bus.send(request.createReply(QVariantList() << result));
            };
        }
        return supported;
    }
    void openUrl(const QString &url) { ++opens; lastUrl = url; }
};

class tst_externalurl : public QObject
{
    Q_OBJECT
    FileService service;
    QDBusConnection serviceBus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, "fileservice-test");
    QVariantMap request(const QString &url, const QString &id = "1") {
        return {{"id", id}, {"url", url}, {"tabId", "17"}, {"locationRevision", "3"}};
    }
private slots:
    void initTestCase() {
        QVERIFY(serviceBus.registerService("org.sailfishos.fileservice"));
        QVERIFY(serviceBus.registerObject("/", &service, QDBusConnection::ExportAllSlots));
    }
    void init() {
        service.queries = service.opens = 0;
        service.supported = true;
        service.deferReply = false;
        service.reply = {};
    }
    void intent_data() {
        QTest::addColumn<QString>("url");
        QTest::addColumn<bool>("valid");
        QTest::newRow("package") << "intent://callback/path?q=1#Intent;scheme=oauth;package=com.example.app;end" << true;
        QTest::newRow("opaque") << "intent:track:123#Intent;scheme=spotify;end" << true;
        QTest::newRow("view") << "intent://maps/#Intent;scheme=https;action=android.intent.action.VIEW;category=android.intent.category.BROWSABLE;end" << true;
        QTest::newRow("component") << "intent://x#Intent;scheme=app;component=com.example/.Main;end" << false;
        QTest::newRow("action") << "intent://x#Intent;scheme=app;action=android.intent.action.DELETE;end" << false;
        QTest::newRow("flags") << "intent://x#Intent;scheme=app;launchFlags=1;end" << false;
        QTest::newRow("selector") << "intent://x#Intent;scheme=app;SEL;end" << false;
        QTest::newRow("data") << "intent:text/html,hi#Intent;scheme=data;end" << false;
        QTest::newRow("nested") << "intent://x#Intent;scheme=intent;end" << false;
        QTest::newRow("missing-scheme") << "intent://x#Intent;package=com.example.app;end" << false;
        QTest::newRow("duplicate") << "intent://x#Intent;scheme=app;scheme=other;end" << false;
        QTest::newRow("percent") << "intent://x#Intent;scheme=app;package=%gg;end" << false;
        QTest::newRow("control") << "intent://x#Intent;scheme=app;package=com.example%0a.app;end" << false;
        QTest::newRow("wrapper") << "intent://x#Intent;scheme=app;package=com.jolla.nativeapp.foo;end" << false;
        QTest::newRow("tail") << "intent://x#Intent;scheme=app;end;extra" << false;
    }
    void intent() { QFETCH(QString, url); QFETCH(bool, valid); QCOMPARE(Lipstick::IntentUrl::parse(url).valid, valid); }
    void fallback() {
        auto result = Lipstick::IntentUrl::parse("intent://callback/path?q=1#Intent;scheme=oauth;S.browser_fallback_url=https%3A%2F%2Fexample.com%2Ffallback%3Fa%3D1%26b%3D2%23fragment;end");
        QVERIFY(result.valid);
        QCOMPARE(result.target.toString(), QString("oauth://callback/path?q=1"));
        QCOMPARE(result.fallback.toString(), QString("https://example.com/fallback?a=1&b=2#fragment"));
        QVERIFY(Lipstick::IntentUrl::parse("intent://x#Intent;scheme=app;S.browser_fallback_url=javascript%3Aalert(1);end").fallback.isEmpty());
        QVERIFY(Lipstick::IntentUrl::parse("intent://x#Intent;scheme=app;S.browser_fallback_url=https%3A%2F%2Fu%3Ap%40example.com;end").fallback.isEmpty());
    }
    void completeUrlAndSingleDispatch() {
        ExternalUrlHandler handler;
        QSignalSpy checked(&handler, &ExternalUrlHandler::checked);
        QSignalSpy dispatched(&handler, &ExternalUrlHandler::dispatched);
        const QString url("oauth://host/path?a=1%26b#fragment");
        handler.check(request(url));
        handler.check(request(url));
        QTRY_COMPARE(checked.count(), 1);
        QCOMPARE(service.queries, 1);
        QCOMPARE(service.lastUrl, url);
        QCOMPARE(checked.first().at(2).toBool(), true);
        handler.open("1"); handler.open("1");
        QTRY_COMPARE(dispatched.count(), 1);
        QCOMPARE(service.opens, 1);
        QVERIFY(dispatched.first().at(1).toBool());
        QCOMPARE(service.lastUrl, url);
    }
    void webInternal() {
        ExternalUrlHandler handler;
        QSignalSpy checked(&handler, &ExternalUrlHandler::checked);
        handler.check(request("https://maps.google.com/?q=Paris"));
        QCOMPARE(checked.count(), 1);
        QVERIFY(!checked.first().at(2).toBool());
        QCOMPARE(service.queries, 0);
    }
    void cancelledQuery() {
        service.deferReply = true;
        ExternalUrlHandler handler;
        QSignalSpy checked(&handler, &ExternalUrlHandler::checked);
        handler.check(request("oauth://host/path"));
        QTRY_COMPARE(service.queries, 1);
        QVERIFY(static_cast<bool>(service.reply));
        QCOMPARE(checked.count(), 0);
        handler.cancel("1");
        service.reply();
        QTest::qWait(60);
        QCOMPARE(checked.count(), 0);
        handler.open("1");
        QCOMPARE(service.opens, 0);
    }
    void absentHandlerKeepsFallback() {
        service.supported = false;
        ExternalUrlHandler handler;
        QSignalSpy checked(&handler, &ExternalUrlHandler::checked);
        handler.check(request("intent://callback#Intent;scheme=app;S.browser_fallback_url=https%3A%2F%2Fexample.com%2Fsign-in;end"));
        QTRY_COMPARE(checked.count(), 1);
        QVERIFY(checked.first().at(1).toBool());
        QVERIFY(!checked.first().at(2).toBool());
        QCOMPARE(checked.first().at(0).toMap().value("fallback").toString(), QString("https://example.com/sign-in"));
        handler.open("1");
        QCOMPARE(service.opens, 0);
    }
    void unavailableService() {
        QVERIFY(serviceBus.unregisterService("org.sailfishos.fileservice"));
        ExternalUrlHandler handler;
        QSignalSpy checked(&handler, &ExternalUrlHandler::checked);
        handler.check(request("oauth://host/path"));
        QTRY_COMPARE(checked.count(), 1);
        QVERIFY(!checked.first().at(1).toBool());
        QVERIFY(!checked.first().at(2).toBool());
        QVERIFY(serviceBus.registerService("org.sailfishos.fileservice"));
    }
};
QTEST_GUILESS_MAIN(tst_externalurl)
#include "tst_externalurl.moc"
