/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

#include <QtTest>

#include <QFile>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QJSValue>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlContext>
#include <QRegularExpression>
#include <QScopedPointer>
#include <QPointer>
#include "mockportalbus.h"

class tst_hostedqml : public QObject
{
    Q_OBJECT

private slots:
    void contextMenuRouting();
    void datePickerRouting();
    void hostedViewSuspension();
    void mediaPlayerRegistration();
    void navigation();
    void newTabPresentation();
    void privateThumbnailCapture();
    void thumbnailScrollDeferral();
    void selectCancellation();
    void promptCancellation();
    void tabCloseCancellation();
    void tabSwipe();
    void sessionOwnership();
    void snapshotApplicationTriggers();
    void externalFlows();
    void portalClient();
    void portalClientReplies();
    void portalClientTeardown();

private:
    static QString readResource(const QString &path);
    static QString functionSource(const QString &path, const QString &name);
    static void runJavaScriptTest(const QString &scriptPath,
                                  const QStringList &functions);
};

void tst_hostedqml::externalFlows()
{
    runJavaScriptTest(QStringLiteral(":/external-flows.js"), QStringList()
                      << functionSource(QStringLiteral(":/ExternalUrlFlow.qml"), QStringLiteral("interceptCallback"))
                      << functionSource(QStringLiteral(":/ExternalUrlFlow.qml"), QStringLiteral("current"))
                      << functionSource(QStringLiteral(":/ExternalUrlFlow.qml"), QStringLiteral("closeFlow"))
                      << functionSource(QStringLiteral(":/ExternalUrlFlow.qml"), QStringLiteral("respond"))
                      << functionSource(QStringLiteral(":/ExternalUrlFlow.qml"), QStringLiteral("reject"))
                      << functionSource(QStringLiteral(":/ExternalUrlFlow.qml"), QStringLiteral("nextPrompt")));
}

void tst_hostedqml::portalClient()
{
    qmlRegisterType<MockPortalBus>("Test.PortalBus", 1, 0, "DBusInterface");
    QQmlEngine engine;
    QQmlComponent windowComponent(&engine);
    windowComponent.setData("import QtQuick 2.6; QtObject { property int activations: 0; function activate() { activations += 1 } }", QUrl());
    QScopedPointer<QObject> window(windowComponent.create());
    QVERIFY2(window, qPrintable(windowComponent.errorString()));
    QString source = readResource(QStringLiteral(":/PortalFlow.qml"));
    source.replace(QStringLiteral("import Nemo.DBus 2.0"), QStringLiteral("import Test.PortalBus 1.0"));
    source.replace(QStringLiteral("id: flow"), QStringLiteral(
                       "id: flow\n    property int activationAtClose\n"
                       "    property int closedCount\n    property int updatedCount\n"
                       "    property int failedCount\n    onUpdated: updatedCount += 1\n"
                       "    onFailed: failedCount += 1\n    onClosed: {\n"
                       "        closedCount += 1\n        activationAtClose = returnWindow.activations\n    }"));
    QQmlComponent component(&engine);
    component.setData(source.toUtf8(), QUrl(QStringLiteral("qrc:/PortalFlow.qml")));
    QScopedPointer<QObject> flow(component.create());
    QVERIFY2(flow, qPrintable(component.errorString()));
    QVERIFY(flow->setProperty("returnWindow", QVariant::fromValue(window.data())));
    auto *bus = flow->findChild<MockPortalBus *>();
    QVERIFY(bus);
    QVariant opened;
    const auto open = [&]() {
        return QMetaObject::invokeMethod(flow.data(), "open", Q_RETURN_ARG(QVariant, opened),
                    Q_ARG(QVariant, "https://provider.example"), Q_ARG(QVariant, "authentication"),
                    Q_ARG(QVariant, "account:/done"));
    };
    QVERIFY(open());
    QVERIFY(opened.toBool());
    const QVariant id = flow->property("_requestId");
    QCOMPARE(bus->calls.count(), 1);
    QVERIFY(open());
    QVERIFY(!opened.toBool());
    QCOMPARE(bus->calls.count(), 1);
    auto openReply = bus->calls[0].success;
    openReply.call();
    QCOMPARE(bus->calls[1].method, QStringLiteral("waitForEvent"));
    QVERIFY(QMetaObject::invokeMethod(flow.data(), "close", Q_ARG(QVariant, "completed")));
    QVERIFY(QMetaObject::invokeMethod(flow.data(), "close", Q_ARG(QVariant, "cancelled")));
    QCOMPARE(bus->calls.count(), 3);
    auto updateReply = bus->calls[1].success;
    updateReply.call({engine.toScriptValue(QVariantMap{{"event", "updated"}, {"url", "account:/done?code=late"}, {"title", ""}})});
    QCOMPARE(flow->property("updatedCount").toInt(), 0);
    QCOMPARE(bus->calls[3].method, QStringLiteral("waitForEvent"));
    auto closedReply = bus->calls[3].success;
    closedReply.call({engine.toScriptValue(QVariantMap{{"event", "closed"}, {"reason", "completed"}})});
    QCOMPARE(flow->property("closedCount").toInt(), 1);
    QCOMPARE(window->property("activations").toInt(), 1);
    QCOMPARE(flow->property("activationAtClose").toInt(), 1);
    closedReply.call({engine.toScriptValue(QVariantMap{{"event", "closed"}, {"reason", "cancelled"}})});
    QCOMPARE(flow->property("closedCount").toInt(), 1);
    QCOMPARE(window->property("activations").toInt(), 1);
    // Owner loss can arrive before the delayed open reply. The late reply
    // must not revive a failed flow or activate the caller twice.
    QVERIFY(open());
    bus->setStatus(MockPortalBus::Unavailable);
    QCOMPARE(flow->property("failedCount").toInt(), 1);
    QCOMPARE(flow->property("closedCount").toInt(), 2);
    QVERIFY(!flow->property("active").toBool());
    auto lateOpenReply = bus->calls[4].success;
    lateOpenReply.call();
    QCOMPARE(flow->property("closedCount").toInt(), 2);
    QCOMPARE(window->property("activations").toInt(), 2);
    QVERIFY(open());
    auto failedOpenReply = bus->calls[5].failure;
    failedOpenReply.call();
    QCOMPARE(flow->property("failedCount").toInt(), 2);
    QCOMPARE(flow->property("closedCount").toInt(), 3);
    QCOMPARE(window->property("activations").toInt(), 3);
}

void tst_hostedqml::portalClientReplies()
{
    qmlRegisterType<MockPortalBus>("Test.PortalBus", 1, 0, "DBusInterface");
    QQmlEngine engine;
    QString source = readResource(QStringLiteral(":/PortalFlow.qml"));
    source.replace("import Nemo.DBus 2.0", "import Test.PortalBus 1.0");
    source.replace("id: flow", "id: flow\n    property string callbackUrl\n"
                   "    onUpdated: { callbackUrl = url; close(\"completed\") }");
    QQmlComponent component(&engine);
    component.setData(source.toUtf8(), QUrl(QStringLiteral("qrc:/PortalFlow.qml")));
    QScopedPointer<QObject> flow(component.create());
    QVERIFY2(flow, qPrintable(component.errorString()));
    auto *bus = flow->findChild<MockPortalBus *>();
    QVERIFY(bus);
    const auto open = [&]() {
        return QMetaObject::invokeMethod(flow.data(), "open", Q_ARG(QVariant, "https://provider.example"),
                    Q_ARG(QVariant, "authentication"), Q_ARG(QVariant, "account:/done"));
    };
    QVERIFY(open());
    auto opened = bus->calls[0].success;
    opened.call();
    // An idle response re-arms without completing or failing the flow.
    auto idle = bus->calls[1].success;
    idle.call({engine.toScriptValue(QVariantMap())});
    QCOMPARE(bus->calls[2].method, QStringLiteral("waitForEvent"));
    QVERIFY(flow->property("active").toBool());
    auto update = bus->calls[2].success;
    update.call({engine.toScriptValue(QVariantMap{{"event", "updated"}, {"url", "account:/done?code=fixture"}, {"title", ""}})});
    QCOMPARE(flow->property("callbackUrl").toString(), QStringLiteral("account:/done?code=fixture"));
    // The callback handler closes the flow, with its final wait already queued.
    QCOMPARE(bus->calls[3].method, QStringLiteral("waitForEvent"));
    QCOMPARE(bus->calls[4].method, QStringLiteral("closeFlow"));
    auto stale = bus->calls[3].failure;
    auto closed = bus->calls[3].success;
    closed.call({engine.toScriptValue(QVariantMap{{"event", "closed"}, {"reason", "completed"}})});
    QVERIFY(!flow->property("active").toBool());
    QVERIFY(open());
    const QString id = flow->property("_requestId").toString();
    stale.call();
    QCOMPARE(flow->property("_requestId").toString(), id);
    auto reopened = bus->calls[5].success;
    reopened.call();
    auto error = bus->calls[6].failure;
    error.call();
    QVERIFY(!flow->property("active").toBool());
    QCOMPARE(bus->calls[7].method, QStringLiteral("closeFlow"));
}

void tst_hostedqml::portalClientTeardown()
{
    qmlRegisterType<MockPortalBus>("Nemo.DBus", 2, 0, "DBusInterface");
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/"));
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQuick 2.6
        import Sailfish.CaptivePortal 1.0 as Portal
        Item {
            id: caller

            property int activations
            property var client
            property var ownedFlow
            function activate() { activations += 1 }
            function complete() {
                ownedFlow.close("completed")
                client.destroy()
                client = null
            }
            Component {
                id: clientComponent

                Item {
                    id: client

                    Component.onCompleted: {
                        caller.ownedFlow = Portal.Flows.create(caller, caller)
                        caller.ownedFlow.open("https://provider.example", "authentication", "account:/done")
                    }
                }
            }
            Component.onCompleted: client = clientComponent.createObject(caller)
        }
    )", QUrl(QStringLiteral("qrc:/ClientTeardown.qml")));
    QScopedPointer<QObject> caller(component.create());
    QVERIFY2(caller, qPrintable(component.errorString()));
    QPointer<QObject> flow = caller->property("ownedFlow").value<QObject *>();
    QVERIFY(flow);
    auto *bus = flow->findChild<MockPortalBus *>();
    QVERIFY(bus);
    const QVariant id = flow->property("_requestId");
    auto openReply = bus->calls[0].success;
    openReply.call();
    QCOMPARE(bus->calls[1].method, QStringLiteral("waitForEvent"));
    QVERIFY(QMetaObject::invokeMethod(caller.data(), "complete"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCOMPARE(bus->calls.count(), 3);
    auto closeReply = bus->calls[1].success;
    closeReply.call({engine.toScriptValue(QVariantMap{{"event", "closed"}, {"reason", "completed"}})});
    QCOMPARE(caller->property("activations").toInt(), 1);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(flow.isNull());
}

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

void tst_hostedqml::datePickerRouting()
{
    runJavaScriptTest(QStringLiteral(":/date-picker-routing.js"),
                      QStringList()
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("openHostedPicker"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("rejectHostedModalRequest")));
}

void tst_hostedqml::hostedViewSuspension()
{
    runJavaScriptTest(QStringLiteral(":/view-suspension.js"),
                      QStringList() << functionSource(
                          QStringLiteral(":/BrowserPage.qml"),
                          QStringLiteral("updateHostedViewSuspension")));
}

void tst_hostedqml::mediaPlayerRegistration()
{
    QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface();
    if (!bus) QSKIP("A session bus is required for MPRIS registration");

    QQmlEngine engine;
    QQmlComponent mockComponent(&engine, QUrl(QStringLiteral("qrc:/MockMediaController.qml")));
    QScopedPointer<QObject> controller(mockComponent.create());
    QVERIFY2(controller, qPrintable(mockComponent.errorString()));
    engine.rootContext()->setContextProperty(QStringLiteral("testController"), controller.data());
    QString source = readResource(QStringLiteral(":/BrowserMediaPlayer.qml"));
    source.remove(QStringLiteral("import Sailfish.WebEngine 1.0"));
    source.replace(QStringLiteral("WebEngine.mediaController"), QStringLiteral("testController"));
    QQmlComponent component(&engine);
    component.setData(source.toUtf8(), QUrl(QStringLiteral("qrc:/BrowserMediaPlayer.qml")));
    if (component.errorString().contains(QStringLiteral("module \"Amber.Mpris\" is not installed"))) {
        QSKIP("Amber MPRIS QML plugin is required for this integration test");
    }
    QScopedPointer<QObject> player(component.create());
    QVERIFY2(player, qPrintable(component.errorString()));
    const QString service = QStringLiteral("org.mpris.MediaPlayer2.sailfish_browser.instance%1")
            .arg(QCoreApplication::applicationPid());
    QTRY_VERIFY(bus->isServiceRegistered(service).value());

    QVERIFY(controller->setProperty("available", false));
    QVERIFY(!bus->isServiceRegistered(service).value());
    QVERIFY(controller->setProperty("available", true));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QTRY_VERIFY(bus->isServiceRegistered(service).value());

    QVERIFY(controller->setProperty("privateBrowsing", true));
    QVERIFY(!bus->isServiceRegistered(service).value());
    QVERIFY(controller->setProperty("privateBrowsing", false));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QTRY_VERIFY(bus->isServiceRegistered(service).value());
    player.reset();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!bus->isServiceRegistered(service).value());
}

void tst_hostedqml::navigation()
{
    const QString page = readResource(QStringLiteral(":/BrowserPage.qml"));
    const QRegularExpression handlerExpression(
                QStringLiteral("^        onRuntimeTraversalReady: (\\{[\\s\\S]*?^        \\})"),
                QRegularExpression::MultilineOption);
    const QString handler = handlerExpression.match(page).captured(1);
    QVERIFY(!handler.isEmpty());
    runJavaScriptTest(QStringLiteral(":/navigation.js"),
                      QStringList()
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("goBack"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("goForward"))
                      << QStringLiteral("function runtimeTraversalReady(persistentId, direction) ")
                         + handler);
}

void tst_hostedqml::newTabPresentation()
{
    runJavaScriptTest(QStringLiteral(":/new-tab-presentation.js"),
                      QStringList()
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("beginForegroundNewTabWait"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("scheduleForegroundNewTabDispatch"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("dispatchForegroundNewTab"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("noteForegroundNewTabSelection"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("noteForegroundNewTabKeyboardSettled"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("noteForegroundNewTabFrame"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("finishForegroundNewTabWait"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("rejectForegroundNewTab"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("newTab")));
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

void tst_hostedqml::thumbnailScrollDeferral()
{
    QStringList functions;
    const QStringList names = QStringList()
            << QStringLiteral("resetHostedThumbnailCapture")
            << QStringLiteral("updateHostedThumbnailScrollState")
            << QStringLiteral("scheduleHostedThumbnailCapture")
            << QStringLiteral("continueHostedThumbnailCapture")
            << QStringLiteral("requestHostedThumbnail")
            << QStringLiteral("captureHostedThumbnail")
            << QStringLiteral("requestPrivateCover")
            << QStringLiteral("captureTabSwipeFrame")
            << QStringLiteral("capturePendingHostedThumbnail")
            << QStringLiteral("cancelHostedThumbnailCaptureForBackground");
    for (const QString &name : names) {
        functions.append(functionSource(QStringLiteral(":/BrowserPage.qml"), name));
    }
    runJavaScriptTest(QStringLiteral(":/thumbnail-scroll.js"), functions);
}

void tst_hostedqml::selectCancellation()
{
    runJavaScriptTest(QStringLiteral(":/select-cancellation.js"),
                      QStringList() << functionSource(
                          QStringLiteral(":/BrowserPage.qml"),
                          QStringLiteral("cancelHostedSelect")));
}

void tst_hostedqml::tabSwipe()
{
    QString constants = readResource(QStringLiteral(":/TabTransition.js"));
    QVERIFY(!constants.isEmpty());
    constants.remove(QStringLiteral(".pragma library"));
    runJavaScriptTest(QStringLiteral(":/tab-swipe.js"),
                      QStringList()
                      << (QStringLiteral("var TabTransition = (function() {\n")
                          + constants
                          + QStringLiteral("\nreturn { Phase: Phase, Operation: Operation }; })();"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("tabSwipeTabAt"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("saveTabSwipeCapture"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("closeTabWithTransition"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("noteTabCloseResult"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("tabSwipeTargetForDistance"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("beginTabSwipe"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("updateTabSwipe"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("endTabSwipe"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("finishTabSwipeSettle"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("noteTabSwipeSelection"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("noteTabSwipeFrame"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("startTabSwipeFade"))
                      << functionSource(QStringLiteral(":/BrowserPage.qml"),
                                        QStringLiteral("finishTabSwipe")));
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

void tst_hostedqml::promptCancellation()
{
    runJavaScriptTest(QStringLiteral(":/prompt-cancellation.js"),
                      QStringList() << functionSource(
                          QStringLiteral(":/BrowserPage.qml"),
                          QStringLiteral("cancelHostedPrompt")));
}

void tst_hostedqml::tabCloseCancellation()
{
    runJavaScriptTest(QStringLiteral(":/tab-close-cancellation.js"),
                      QStringList() << functionSource(
                          QStringLiteral(":/TabItem.qml"),
                          QStringLiteral("restoreRejectedClose")));
}
