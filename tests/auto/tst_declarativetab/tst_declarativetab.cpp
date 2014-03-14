/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickView>
#include <QHash>
#include <QStandardPaths>

#include "declarativetab.h"
#include "declarativetabmodel.h"
#include "declarativewebcontainer.h"

static const QByteArray QML_SNIPPET = \
        "import QtQuick 2.0\n" \
        "import Sailfish.Browser 1.0\n" \
        "WebContainer {\n" \
        "   property alias tabModel: model\n" \
        "   width: 100; height: 100\n" \
        "   TabModel { id: model }\n" \
        "}\n";

class tst_declarativetab : public QObject
{
    Q_OBJECT

public:
    tst_declarativetab(QObject *parent = 0);

private slots:
    void initTestCase();
    void testTitle();
    void testUrl();
    void testUpdateTabData();

    void cleanupTestCase();

private:
    DeclarativeTabModel *tabModel;
    DeclarativeTab *tab;
    QQuickView view;
};


tst_declarativetab::tst_declarativetab(QObject *parent)
    : QObject(parent)
    , tabModel(0)
    , tab(0)
{
}

void tst_declarativetab::initTestCase()
{
    QQmlComponent component(view.engine());
    component.setData(QML_SNIPPET, QUrl());
    QObject *obj = component.create(view.engine()->rootContext());

    view.setContent(QUrl(""), 0, obj);
    view.show();
    QTest::qWaitForWindowExposed(&view);
    QVariant var = obj->property("tabModel");
    tabModel = qobject_cast<DeclarativeTabModel *>(qvariant_cast<QObject*>(var));
    QVERIFY(tabModel);

    DeclarativeWebContainer *webContainer = qobject_cast<DeclarativeWebContainer *>(obj);
    tab = webContainer->currentTab();
    QVERIFY(tab);

    if (!tabModel->loaded()) {
        QSignalSpy loadedSpy(tabModel, SIGNAL(loadedChanged()));
        // Tabs must be loaded with in 500ms
        QVERIFY(loadedSpy.wait());
        QCOMPARE(loadedSpy.count(), 1);
    }

    QSignalSpy currentUrlChangedSpy(tab, SIGNAL(urlChanged()));
    tabModel->addTab("http://www.jolla.com", "");
    QCOMPARE(currentUrlChangedSpy.count(), 1);
}

void tst_declarativetab::testTitle()
{
    QSignalSpy titleChangeSpy(tab, SIGNAL(titleChanged()));
    tab->setTitle("FooBar");
    QCOMPARE(titleChangeSpy.count(), 1);
    QCOMPARE(tab->title(), QString("FooBar"));
}

void tst_declarativetab::testUrl()
{
    QSignalSpy urlChangeSpy(tab, SIGNAL(urlChanged()));
    tab->setUrl("http://foobar.com");
    QCOMPARE(urlChangeSpy.count(), 1);
    QCOMPARE(tab->url(), QString("http://foobar.com"));
}

void tst_declarativetab::testUpdateTabData()
{
    QSignalSpy urlChangeSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy titleChangeSpy(tab, SIGNAL(titleChanged()));

    Tab newTab;
    // Update only linkId.
    int linkId = tab->linkId() + 1;
    Link link(linkId, "http://foobar.com", "", "FooBar");
    newTab.setCurrentLink(link);
    tab->updateTabData(newTab);

    QCOMPARE(urlChangeSpy.count(), 0);
    QCOMPARE(titleChangeSpy.count(), 0);
    QCOMPARE(tab->linkId(), linkId);

    // Update title
    linkId = tab->linkId();
    link.setTitle("");
    newTab.setCurrentLink(link);
    tab->updateTabData(newTab);
    QCOMPARE(urlChangeSpy.count(), 0);
    QCOMPARE(titleChangeSpy.count(), 1);
    QVERIFY(tab->title().isEmpty());
    QCOMPARE(tab->linkId(), linkId);

    // Update url
    link.setUrl("http://foobar.com/page2");
    newTab.setCurrentLink(link);
    tab->updateTabData(newTab);

    QCOMPARE(urlChangeSpy.count(), 1);
    QCOMPARE(titleChangeSpy.count(), 1);
    QCOMPARE(tab->url(), QString("http://foobar.com/page2"));
    QVERIFY(tab->title().isEmpty());
    QCOMPARE(tab->linkId(), linkId);

    // Update url, title, link (navigation)
    linkId = tab->linkId() + 1;
    link = Link(linkId, "http://foobar.com/page3", "", "FooBar");
    newTab.setCurrentLink(link);
    tab->updateTabData(newTab);

    QCOMPARE(urlChangeSpy.count(), 2);
    QCOMPARE(titleChangeSpy.count(), 2);
    QCOMPARE(tab->url(), QString("http://foobar.com/page3"));
    QCOMPARE(tab->title(), QString("FooBar"));
    QCOMPARE(tab->linkId(), linkId);
}

void tst_declarativetab::cleanupTestCase()
{
    tabModel->clear();
    QVERIFY(tabModel->count() == 0);

    // Wait for event loop of db manager
    QTest::qWait(500);
    QString dbFileName = QString("%1/%2")
            .arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation))
            .arg(QLatin1String(DB_NAME));
    QFile dbFile(dbFileName);
    QVERIFY(dbFile.remove());
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    tst_declarativetab testcase;
    qmlRegisterUncreatableType<DeclarativeTab>("Sailfish.Browser", 1, 0, "Tab", "");
    qmlRegisterType<DeclarativeTabModel>("Sailfish.Browser", 1, 0, "TabModel");
    qmlRegisterType<DeclarativeWebContainer>("Sailfish.Browser", 1, 0, "WebContainer");
    return QTest::qExec(&testcase, argc, argv); \
}
#include "tst_declarativetab.moc"
