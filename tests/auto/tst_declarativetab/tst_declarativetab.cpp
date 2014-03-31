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
        "   tabModel: TabModel {}\n" \
        "   width: 100; height: 100\n" \
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
    void testThumbnail();
    void testUpdateTabData();
    void testUpdateTabTitle();
    void testUpdateTabAll();

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

void tst_declarativetab::testThumbnail()
{
    // Thumbnail setter is used only from C++. Thus, it updates also Tab data.
    tab->setThumbnailPath("");
    QSignalSpy thumbnailChangeSpy(tab, SIGNAL(thumbPathChanged()));
    Tab tabData = tab->tabData();
    QVERIFY(tabData.currentLink().thumbPath().isEmpty());
    QVERIFY(tab->thumbnailPath().isEmpty());
    tab->setThumbnailPath("foobar.png");
    tabData = tab->tabData();
    QCOMPARE(thumbnailChangeSpy.count(), 1);
    QCOMPARE(tabData.currentLink().thumbPath(), QString("foobar.png"));
    QCOMPARE(tab->thumbnailPath(), QString("foobar.png"));
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

void tst_declarativetab::testUpdateTabTitle()
{
    Link link(1, "", "", "");
    Tab tabData(1, link, 0, 0);
    tab->updateTabData(tabData);
    QSignalSpy titleChangeSpy(tab, SIGNAL(titleChanged()));
    tabData = tab->tabData();
    QVERIFY(tabData.currentLink().title().isEmpty());
    QVERIFY(tab->title().isEmpty());
    tab->setTitle("FooBar");
    tabData = tab->tabData();
    QVERIFY(tabData.currentLink().title().isEmpty());
    QCOMPARE(titleChangeSpy.count(), 1);
    QCOMPARE(tab->title(), QString("FooBar"));
    tab->updateTabData("FooBar 2");
    tabData = tab->tabData();
    QCOMPARE(tabData.currentLink().title(), QString("FooBar 2"));
    QCOMPARE(titleChangeSpy.count(), 2);
    QCOMPARE(tab->title(), QString("FooBar 2"));
}

void tst_declarativetab::testUpdateTabAll()
{
    Link link(1, "", "", "");
    Tab tabData(1, link, 0, 0);
    tab->updateTabData(tabData);
    QSignalSpy urlChangeSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy titleChangeSpy(tab, SIGNAL(titleChanged()));
    QSignalSpy thumbnailChangeSpy(tab, SIGNAL(thumbPathChanged()));

    tab->updateTabData("http://foobar.com", "", "");
    QCOMPARE(urlChangeSpy.count(), 1);
    QCOMPARE(titleChangeSpy.count(), 0);
    QCOMPARE(thumbnailChangeSpy.count(), 0);
    tabData = tab->tabData();
    QCOMPARE(tabData.currentLink().url(), QString("http://foobar.com"));
    QVERIFY(tabData.currentLink().title().isEmpty());
    QVERIFY(tabData.currentLink().thumbPath().isEmpty());
    QCOMPARE(tab->url(), QString("http://foobar.com"));
    QVERIFY(tab->title().isEmpty());
    QVERIFY(tab->thumbnailPath().isEmpty());

    tab->updateTabData("http://foobar.com", "FooBar", "");
    QCOMPARE(urlChangeSpy.count(), 1);
    QCOMPARE(titleChangeSpy.count(), 1);
    QCOMPARE(thumbnailChangeSpy.count(), 0);
    tabData = tab->tabData();
    QCOMPARE(tabData.currentLink().url(), QString("http://foobar.com"));
    QCOMPARE(tabData.currentLink().title(), QString("FooBar"));
    QVERIFY(tabData.currentLink().thumbPath().isEmpty());
    QCOMPARE(tab->url(), QString("http://foobar.com"));
    QCOMPARE(tab->title(), QString("FooBar"));
    QVERIFY(tab->thumbnailPath().isEmpty());

    tab->updateTabData("http://foobar.com", "FooBar", "FooBar.png");
    QCOMPARE(urlChangeSpy.count(), 1);
    QCOMPARE(titleChangeSpy.count(), 1);
    QCOMPARE(thumbnailChangeSpy.count(), 1);
    tabData = tab->tabData();
    QCOMPARE(tabData.currentLink().url(), QString("http://foobar.com"));
    QCOMPARE(tabData.currentLink().title(), QString("FooBar"));
    QCOMPARE(tabData.currentLink().thumbPath(), QString("FooBar.png"));
    QCOMPARE(tab->url(), QString("http://foobar.com"));
    QCOMPARE(tab->title(), QString("FooBar"));
    QCOMPARE(tab->thumbnailPath(), QString("FooBar.png"));

    tab->updateTabData("", "", "");
    QCOMPARE(urlChangeSpy.count(), 2);
    QCOMPARE(titleChangeSpy.count(), 2);
    QCOMPARE(thumbnailChangeSpy.count(), 2);
    tabData = tab->tabData();
    QVERIFY(tabData.currentLink().url().isEmpty());
    QVERIFY(tabData.currentLink().title().isEmpty());
    QVERIFY(tabData.currentLink().thumbPath().isEmpty());
    QVERIFY(tab->url().isEmpty());
    QVERIFY(tab->title().isEmpty());
    QVERIFY(tab->thumbnailPath().isEmpty());
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
