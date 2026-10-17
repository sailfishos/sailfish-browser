/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */
#include "portalflowservice.h"
#include "captiveportalservice.h"
#include <QtTest>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QEventLoop>
#include <QTimer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopedPointer>
#include <QDBusArgument>
#include <QProcess>
#include <QTemporaryDir>
#include <QFileInfo>

class tst_portalflows : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();
    void queuedCancellation();
    void headings();
    void legacyNetworkHeading();
    void ownershipAndCallback();
    void callerExit();
    void validation();
    void replyResults();
    void qmlReplies();
    void pendingWait();
    void resultLimits();
    void filteredResults();
private:
    QDBusMessage call(const QString &client, const QString &method, const QVariantList &args);
    PortalFlowService *m_service = nullptr;
};

QDBusMessage tst_portalflows::call(const QString &client, const QString &method, const QVariantList &args)
{
    auto message = QDBusMessage::createMethodCall(QStringLiteral("org.sailfishos.captiveportal"),
                        QStringLiteral("/flows"), QStringLiteral("org.sailfishos.captiveportal.Flow1"), method);
    message.setArguments(args);
    QDBusPendingCallWatcher pending(QDBusConnection(client).asyncCall(message));
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    connect(&pending, &QDBusPendingCallWatcher::finished, &loop, &QEventLoop::quit);
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(2000);
    if (!pending.isFinished()) loop.exec();
    if (!pending.isFinished()) return QDBusMessage::createError(QStringLiteral("Test.Timeout"), QString());
    return pending.reply();
}

void tst_portalflows::init()
{
    QVERIFY(QDBusConnection::sessionBus().registerService(QStringLiteral("org.sailfishos.captiveportal")));
    m_service = new PortalFlowService;
    QVERIFY(m_service->registered());
    QVERIFY(QDBusConnection::connectToBus(QDBusConnection::SessionBus, QStringLiteral("portal-client-a")).isConnected());
    QVERIFY(QDBusConnection::connectToBus(QDBusConnection::SessionBus, QStringLiteral("portal-client-b")).isConnected());
}

void tst_portalflows::cleanup()
{
    QDBusConnection::disconnectFromBus(QStringLiteral("portal-client-a"));
    QDBusConnection::disconnectFromBus(QStringLiteral("portal-client-b"));
    QDBusConnection::sessionBus().unregisterObject(QStringLiteral("/flows"));
    QDBusConnection::sessionBus().unregisterService(QStringLiteral("org.sailfishos.captiveportal"));
    delete m_service;
    m_service = nullptr;
}

void tst_portalflows::legacyNetworkHeading()
{
    CaptivePortalService network;
    QSignalSpy opened(&network, &CaptivePortalService::openUrlRequested);
    network.openUrl(QStringList() << "https://example.com" << "Network access");
    QCOMPARE(network.heading(), QStringLiteral("Network access"));
    QCOMPARE(opened.last().at(0).toString(), QStringLiteral("https://example.com"));
    network.openUrl(QStringList() << "https://example.com" << "");
    QVERIFY(network.heading().isEmpty());
    network.openUrl(QStringList() << "https://example.com");
    QCOMPARE(network.heading(), qtTrId("sailfish_captiveportal-la-sign_in"));
}

void tst_portalflows::headings()
{
    QSignalSpy opened(m_service, &PortalFlowService::openRequested);
    QCOMPARE(call("portal-client-a", "openFlowWithHeading", QVariantList()
                  << "headed" << "https://example.com" << "authentication" << "" << "Sign in").type(),
             QDBusMessage::ReplyMessage);
    m_service->setReady();
    QCOMPARE(opened.count(), 1);
    QCOMPARE(opened.at(0).at(3).toString(), QStringLiteral("Sign in"));
    QCOMPARE(call("portal-client-a", "openFlowWithHeading", QVariantList()
                  << "empty" << "https://example.com" << "terms" << "" << "").type(),
             QDBusMessage::ReplyMessage);
    QCOMPARE(opened.at(1).at(3).toString(), QString());
    QCOMPARE(call("portal-client-a", "openFlow", QVariantList()
                  << "legacy" << "https://example.com" << "terms" << "").type(),
             QDBusMessage::ReplyMessage);
    QCOMPARE(opened.at(2).at(3).toString(), QString());
    QCOMPARE(call("portal-client-a", "openFlowWithHeading", QVariantList()
                  << "long" << "https://example.com" << "terms" << "" << QString(257, 'x')).type(),
             QDBusMessage::ErrorMessage);
}

void tst_portalflows::queuedCancellation()
{
    QSignalSpy opened(m_service, &PortalFlowService::openRequested);
    QCOMPARE(call("portal-client-a", "openFlow", QVariantList() << "early" << "https://example.com" << "terms" << "").type(), QDBusMessage::ReplyMessage);
    QCOMPARE(opened.count(), 0);
    QCOMPARE(call("portal-client-a", "closeFlow", QVariantList() << "early").type(), QDBusMessage::ReplyMessage);
    m_service->setReady();
    QCOMPARE(opened.count(), 0);
}

void tst_portalflows::ownershipAndCallback()
{
    m_service->setReady();
    QSignalSpy opened(m_service, &PortalFlowService::openRequested);
    QSignalSpy closing(m_service, &PortalFlowService::closeRequested);
    // Request ids are caller-scoped. A second connection cannot close the first.
    QCOMPARE(call("portal-client-a", "openFlow", QVariantList() << "same" << "https://provider.example/auth" << "authentication" << "account-app:/callback?client=1").type(), QDBusMessage::ReplyMessage);
    QCOMPARE(call("portal-client-b", "closeFlow", QVariantList() << "same").type(), QDBusMessage::ErrorMessage);
    QCOMPARE(closing.count(), 0);
    QCOMPARE(call("portal-client-b", "openFlow", QVariantList() << "same" << "https://example.com/terms" << "terms" << "").type(), QDBusMessage::ReplyMessage);
    QCOMPARE(opened.count(), 2);
    const QString first = opened.at(0).at(0).toString();
    const QString second = opened.at(1).at(0).toString();
    QVERIFY(first != second);
    QVERIFY(m_service->callbackMatches(first, "account-app:/callback?client=1&code=abc&state=expected"));
    QVERIFY(!m_service->callbackMatches(first, "other-app:/callback?client=1&code=abc"));
    QVERIFY(!m_service->callbackMatches(first, "account-app:/callback-extra?client=1"));
    QVERIFY(!m_service->callbackMatches(first, "account-app:/callback?client=2"));
    QVERIFY(!m_service->callbackMatches(second, "account-app:/callback?client=1"));
    QCOMPARE(call("portal-client-a", "closeFlow", QVariantList() << "same").type(), QDBusMessage::ReplyMessage);
    QCOMPARE(closing.count(), 1);
    QCOMPARE(closing.at(0).at(0).toString(), first);
    m_service->closed(first, "completed");
    QVERIFY(!m_service->callbackMatches(first, "account-app:/callback?client=1"));
    QCOMPARE(call("portal-client-b", "closeFlow", QVariantList() << "same").type(), QDBusMessage::ReplyMessage);
    QCOMPARE(closing.at(1).at(0).toString(), second);
}

void tst_portalflows::callerExit()
{
    m_service->setReady();
    QSignalSpy opened(m_service, &PortalFlowService::openRequested);
    QSignalSpy closing(m_service, &PortalFlowService::closeRequested);
    QCOMPARE(call("portal-client-a", "openFlow", QVariantList() << "gone" << "https://example.com" << "terms" << "").type(), QDBusMessage::ReplyMessage);
    QCOMPARE(call("portal-client-b", "openFlow", QVariantList() << "survive" << "https://example.org" << "terms" << "").type(), QDBusMessage::ReplyMessage);
    QDBusConnection::disconnectFromBus(QStringLiteral("portal-client-a"));
    QTRY_COMPARE(closing.count(), 1);
    QCOMPARE(closing.at(0).at(0).toString(), opened.at(0).at(0).toString());
    QCOMPARE(call("portal-client-b", "closeFlow", QVariantList() << "survive").type(), QDBusMessage::ReplyMessage);
    QCOMPARE(closing.count(), 2);
}

void tst_portalflows::validation()
{
    // UI-only methods must not be exported through the public adaptor.
    QCOMPARE(call("portal-client-a", "setReady", QVariantList()).type(), QDBusMessage::ErrorMessage);
    QCOMPARE(call("portal-client-a", "updated", QVariantList() << "id" << "url" << "title").type(), QDBusMessage::ErrorMessage);
    QCOMPARE(call("portal-client-a", "openFlow", QVariantList() << "bad" << "file:///etc/passwd" << "terms" << "").type(), QDBusMessage::ErrorMessage);
    QCOMPARE(call("portal-client-a", "openFlow", QVariantList() << "bad" << "https://user:secret@example.com" << "terms" << "").type(), QDBusMessage::ErrorMessage);
    QCOMPARE(call("portal-client-a", "openFlow", QVariantList() << "bad" << "https://example.com" << "unknown" << "").type(), QDBusMessage::ErrorMessage);
    QCOMPARE(call("portal-client-a", "openFlow", QVariantList() << "ok" << "https://example.com" << "authentication" << "/relative").type(), QDBusMessage::ErrorMessage);
    QCOMPARE(call("portal-client-a", "openFlow", QVariantList() << "ok" << "https://example.com" << "terms" << "").type(), QDBusMessage::ReplyMessage);
    QCOMPARE(call("portal-client-a", "openFlow", QVariantList() << "ok" << "https://example.com" << "terms" << "").type(), QDBusMessage::ErrorMessage);
}

static QVariantMap readEvent(const QDBusMessage &reply)
{
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().size() != 1)
        return QVariantMap();
    return qdbus_cast<QVariantMap>(reply.arguments().first());
}

void tst_portalflows::replyResults()
{
    m_service->setReady();
    QSignalSpy opened(m_service, &PortalFlowService::openRequested);
    for (const QString &client : {QStringLiteral("portal-client-a"), QStringLiteral("portal-client-b")})
        QCOMPARE(call(client, "openFlow", QVariantList() << "same" << "https://example.com" << "authentication" << "account:/done").type(), QDBusMessage::ReplyMessage);
    const QString first = opened.at(0).at(0).toString();
    const QString second = opened.at(1).at(0).toString();
    // Updates may precede the open reply or arrive between waits. Never lose
    // an intermediate callback, nor leak it to another owner with the same ID.
    m_service->updated(first, "account:/done?code=first", "");
    m_service->updated(first, "https://example.com/after", "");
    m_service->updated(second, "account:/done?code=second", "");
    QCOMPARE(readEvent(call("portal-client-a", "waitForEvent", {"same"})).value("url").toString(), QStringLiteral("account:/done?code=first"));
    QCOMPARE(readEvent(call("portal-client-b", "waitForEvent", {"same"})).value("url").toString(), QStringLiteral("account:/done?code=second"));
    m_service->closed(first, "completed");
    QVERIFY(m_service->pendingResults());
    QCOMPARE(readEvent(call("portal-client-a", "waitForEvent", {"same"})).value("url").toString(), QStringLiteral("https://example.com/after"));
    QVERIFY(m_service->pendingResults());
    QCOMPARE(readEvent(call("portal-client-a", "waitForEvent", {"same"})).value("event").toString(), QStringLiteral("closed"));
    QVERIFY(!m_service->pendingResults());
    QCOMPARE(call("portal-client-a", "waitForEvent", {"same"}).errorName(), QStringLiteral("org.freedesktop.DBus.Error.AccessDenied"));
}

void tst_portalflows::qmlReplies()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        import Nemo.DBus 2.0
        DBusInterface {
            property string result
            property string error
            service: "org.sailfishos.captiveportal"
            path: "/flows"
            iface: "org.sailfishos.captiveportal.Flow1"
            function receive() {
                call("openFlow", ["qml", "https://example.com/result", "terms", ""], function() {
                    call("waitForEvent", ["qml"], function(event) {
                        result = event.url
                    }, function() { error = "wait failed" })
                }, function() { error = "open failed" })
            }
        }
    )", QUrl(QStringLiteral("file:reply-results.qml")));
    if (component.errorString().contains(QStringLiteral("module \"Nemo.DBus\" is not installed")))
        QSKIP("The Sailfish Nemo.DBus plugin is required for reply conversion");
    QScopedPointer<QObject> receiver(component.create());
    QVERIFY2(receiver, qPrintable(component.errorString()));
    // Nemo.DBus introspects methods synchronously. Keep the service in another
    // process, as in production, so local-call dispatch cannot deadlock it.
    QDBusConnection::sessionBus().unregisterService("org.sailfishos.captiveportal");
    QProcess server;
    server.start(QCoreApplication::applicationFilePath(), {"--filtered-service"});
    QVERIFY(server.waitForStarted());
    QTRY_VERIFY(server.bytesAvailable() > 0);
    QVERIFY(server.readAllStandardOutput().contains("ready"));
    QVERIFY(QMetaObject::invokeMethod(receiver.data(), "receive"));
    QTRY_COMPARE(receiver->property("result").toString(), QStringLiteral("https://example.com/result"));
    QVERIFY(receiver->property("error").toString().isEmpty());
    server.terminate();
    QVERIFY(server.waitForFinished());
}

void tst_portalflows::pendingWait()
{
    m_service->setReady();
    QSignalSpy opened(m_service, &PortalFlowService::openRequested);
    QCOMPARE(call("portal-client-a", "openFlow", {"wait", "https://example.com", "terms", ""}).type(), QDBusMessage::ReplyMessage);
    const QString id = opened.first().first().toString();
    auto request = QDBusMessage::createMethodCall("org.sailfishos.captiveportal", "/flows", "org.sailfishos.captiveportal.Flow1", "waitForEvent");
    request.setArguments({"wait"});
    QDBusPendingCallWatcher pending(QDBusConnection("portal-client-a").asyncCall(request));
    QCOMPARE(call("portal-client-a", "waitForEvent", {"wait"}).errorName(), QStringLiteral("org.freedesktop.DBus.Error.LimitsExceeded"));
    QCOMPARE(call("portal-client-b", "waitForEvent", {"wait"}).errorName(), QStringLiteral("org.freedesktop.DBus.Error.AccessDenied"));
    QVERIFY(!pending.isFinished());
    m_service->loaded(id, true);
    QTRY_VERIFY(pending.isFinished());
    QCOMPARE(readEvent(pending.reply()).value("secure").toBool(), true);
    // Exercise expiry without waiting twenty seconds in every suite run.
    QDBusPendingCallWatcher timeout(QDBusConnection("portal-client-a").asyncCall(request));
    QTRY_VERIFY(m_service->findChild<QTimer *>()->isActive());
    m_service->findChild<QTimer *>()->start(1);
    QTRY_VERIFY(timeout.isFinished());
    QCOMPARE(timeout.reply().type(), QDBusMessage::ReplyMessage);
    QVERIFY(readEvent(timeout.reply()).isEmpty());
    QDBusPendingCallWatcher terminal(QDBusConnection("portal-client-a").asyncCall(request));
    QTRY_VERIFY(m_service->findChild<QTimer *>()->isActive());
    m_service->closed(id, "cancelled");
    QTRY_VERIFY(terminal.isFinished());
    QCOMPARE(readEvent(terminal.reply()).value("reason").toString(), QStringLiteral("cancelled"));
    QVERIFY(!m_service->pendingResults());
}

void tst_portalflows::resultLimits()
{
    m_service->setReady();
    QSignalSpy opened(m_service, &PortalFlowService::openRequested);
    QSignalSpy closing(m_service, &PortalFlowService::closeRequested);
    QCOMPARE(call("portal-client-a", "openFlow", {"limit", "https://example.com", "terms", ""}).type(), QDBusMessage::ReplyMessage);
    const QString id = opened.first().first().toString();
    for (int i = 0; i < 65; ++i) m_service->updated(id, QString::number(i), "");
    QCOMPARE(closing.count(), 1);
    QCOMPARE(readEvent(call("portal-client-a", "waitForEvent", {"limit"})).value("reason").toString(), QStringLiteral("overflow"));
    m_service->closed(id, "failed");
    QVERIFY(m_service->pendingResults());
    m_service->findChild<QTimer *>()->start(1);
    QTRY_VERIFY(!m_service->pendingResults());
    QCOMPARE(call("portal-client-a", "waitForEvent", {"limit"}).type(), QDBusMessage::ErrorMessage);
}

void tst_portalflows::filteredResults()
{
    if (QStandardPaths::findExecutable("xdg-dbus-proxy").isEmpty())
        QSKIP("xdg-dbus-proxy is required for the sandbox regression");
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString socket = directory.path() + "/bus";
    QProcess proxy;
    proxy.start("xdg-dbus-proxy", {QString::fromLocal8Bit(qgetenv("DBUS_SESSION_BUS_ADDRESS")), socket,
                                  "--filter", "--own=org.sailfishos.captiveportal"});
    QVERIFY(proxy.waitForStarted());
    QTRY_VERIFY(QFileInfo::exists(socket));
    QDBusConnection::sessionBus().unregisterService("org.sailfishos.captiveportal");
    QProcess server;
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("DBUS_SESSION_BUS_ADDRESS", "unix:path=" + socket);
    server.setProcessEnvironment(environment);
    server.start(QCoreApplication::applicationFilePath(), {"--filtered-service"});
    QVERIFY(server.waitForStarted());
    QTRY_VERIFY(server.bytesAvailable() > 0);
    QVERIFY(server.readAllStandardOutput().contains("ready"));
    // Neither client owns a well-known name. The proxy grants no TALK rules.
    for (const QString &client : {QStringLiteral("portal-client-a"), QStringLiteral("portal-client-b")}) {
        const QString url = "https://example.com/" + client;
        QCOMPARE(call(client, "openFlow", {"same", url, "terms", ""}).type(), QDBusMessage::ReplyMessage);
        QCOMPARE(readEvent(call(client, "waitForEvent", {"same"})).value("url").toString(), url);
        QCOMPARE(readEvent(call(client, "waitForEvent", {"same"})).value("secure").toBool(), true);
    }
    QCOMPARE(call("portal-client-b", "waitForEvent", {"foreign"}).errorName(), QStringLiteral("org.freedesktop.DBus.Error.AccessDenied"));
    auto request = QDBusMessage::createMethodCall("org.sailfishos.captiveportal", "/flows", "org.sailfishos.captiveportal.Flow1", "waitForEvent");
    request.setArguments({"same"});
    QDBusPendingCallWatcher pending(QDBusConnection("portal-client-a").asyncCall(request));
    QCOMPARE(call("portal-client-a", "waitForEvent", {"same"}).errorName(), QStringLiteral("org.freedesktop.DBus.Error.LimitsExceeded"));
    QCOMPARE(call("portal-client-a", "closeFlow", {"same"}).type(), QDBusMessage::ReplyMessage);
    QTRY_VERIFY(pending.isFinished());
    QCOMPARE(readEvent(pending.reply()).value("event").toString(), QStringLiteral("closed"));
    QCOMPARE(call("portal-client-b", "closeFlow", {"same"}).type(), QDBusMessage::ReplyMessage);
    QCOMPARE(readEvent(call("portal-client-b", "waitForEvent", {"same"})).value("event").toString(), QStringLiteral("closed"));
    server.terminate();
    QVERIFY(server.waitForFinished());
    proxy.terminate();
    QVERIFY(proxy.waitForFinished());
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (app.arguments().contains("--filtered-service")) {
        if (!QDBusConnection::sessionBus().registerService("org.sailfishos.captiveportal")) return 2;
        PortalFlowService service;
        if (!service.registered()) return 3;
        QObject::connect(&service, &PortalFlowService::openRequested, &service,
                         [&service](const QString &id, const QString &url) {
            service.updated(id, url, "");
            service.loaded(id, true);
        });
        QObject::connect(&service, &PortalFlowService::closeRequested, &service,
                         [&service](const QString &id) { service.closed(id, "completed"); });
        service.setReady();
        puts("ready");
        fflush(stdout);
        return app.exec();
    }
    tst_portalflows test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_portalflows.moc"
