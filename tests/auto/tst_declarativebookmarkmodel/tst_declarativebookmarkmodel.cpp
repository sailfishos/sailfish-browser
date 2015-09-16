/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtTest>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>

#include "declarativebookmarkmodel.h"
#include "bookmarkmanager.h"

static const QByteArray BOOKMARKS_JSON = \
    "[{" \
    "  \"favicon\": \"image://theme/icon-launcher-jollacom\"," \
    "  \"hasTouchIcon\": true," \
    "  \"title\": \"Jolla\"," \
    "  \"url\": \"http://jolla.com/\"" \
    "}]";

static const QString JOLLA_URL = "http://jolla.com/";
static const QString TEST_URL = "http://www.test1.jolla.com";

class tst_declarativebookmarkmodel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void add();
    void remove();
    void removeByIndex();
    void contains();
    void edit();
    void setActiveUrl();
    void clearBookmarks();
    void updateFavoriteIcon();
    void data();

private:
    QPointer<DeclarativeBookmarkModel> m_model;
    QString m_bookmarksFile;
};

void tst_declarativebookmarkmodel::initTestCase()
{
    QString settingsLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir dir(settingsLocation);
    if (!dir.exists()) {
        if (!dir.mkpath(settingsLocation)) {
            qWarning() << "Can't create directory " << settingsLocation;
            return;
        }
    }
    m_bookmarksFile = settingsLocation + "/bookmarks.json";
}

void tst_declarativebookmarkmodel::init()
{
    QFile file(m_bookmarksFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Can't create file " << m_bookmarksFile;
    }

    QTextStream out(&file);
    out << BOOKMARKS_JSON;
    file.close();

    m_model = new DeclarativeBookmarkModel(this);
    QCOMPARE(m_model->rowCount(), 1);
}

void tst_declarativebookmarkmodel::cleanup()
{
    delete m_model;
    QFile file(m_bookmarksFile);
    QVERIFY(file.remove());
}

void tst_declarativebookmarkmodel::add()
{
    QSignalSpy countChangeSpy(m_model, SIGNAL(countChanged()));
    QSignalSpy activeUrlBookmarkedChangedSpy(m_model, SIGNAL(activeUrlBookmarkedChanged()));
    m_model->add(TEST_URL, "test", "");

    QCOMPARE(countChangeSpy.count(), 1);
    QCOMPARE(activeUrlBookmarkedChangedSpy.count(), 1);
    QCOMPARE(m_model->rowCount(), 2);
    QVERIFY(m_model->contains(TEST_URL));
}

void tst_declarativebookmarkmodel::remove()
{
    QSignalSpy countChangeSpy(m_model, SIGNAL(countChanged()));
    QSignalSpy activeUrlBookmarkedChangedSpy(m_model, SIGNAL(activeUrlBookmarkedChanged()));

    m_model->remove(JOLLA_URL);
    QCOMPARE(countChangeSpy.count(), 1);
    QCOMPARE(activeUrlBookmarkedChangedSpy.count(), 1);
    QCOMPARE(m_model->rowCount(), 0);
}

void tst_declarativebookmarkmodel::removeByIndex()
{
    add();

    QSignalSpy countChangeSpy(m_model, SIGNAL(countChanged()));
    QSignalSpy activeUrlBookmarkedChangedSpy(m_model, SIGNAL(activeUrlBookmarkedChanged()));

    m_model->remove(0);
    QCOMPARE(countChangeSpy.count(), 1);
    QCOMPARE(activeUrlBookmarkedChangedSpy.count(), 1);
    // Make sure the first entry (index == 0) got removed.
    QCOMPARE(m_model->rowCount(), 1);
    QVERIFY(m_model->contains(TEST_URL));
}

void tst_declarativebookmarkmodel::contains()
{
    QVERIFY(!m_model->contains(TEST_URL));
    add();
    QVERIFY(m_model->contains(TEST_URL));
}

void tst_declarativebookmarkmodel::edit()
{
    add();
    m_model->add("http://www.test5.jolla.com/2", "jolla2", "");
    QSignalSpy activeUrlBookmarkedChangedSpy(m_model, SIGNAL(activeUrlBookmarkedChanged()));
    QSignalSpy dataChangedSpy(m_model, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)));

    QModelIndex index = m_model->index(0);
    // Let's make sure that first bookmark stays untouched.
    QString constUrl = m_model->data(index, DeclarativeBookmarkModel::UrlRole).toString();
    QString constTitle = m_model->data(index, DeclarativeBookmarkModel::TitleRole).toString();

    // Edit only the last entry
    index = m_model->index(2);
    QString originalUrl = m_model->data(index, DeclarativeBookmarkModel::UrlRole).toString();
    QString newTitle("New title");
    QString newUrl("http://www.test5.jolla.com/edited");

    m_model->edit(index.row(), originalUrl, newTitle);
    QString title = m_model->data(index, DeclarativeBookmarkModel::TitleRole).toString();
    QString url = m_model->data(index, DeclarativeBookmarkModel::UrlRole).toString();

    QCOMPARE(activeUrlBookmarkedChangedSpy.count(), 0);
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(title, newTitle);
    QCOMPARE(url, originalUrl);

    m_model->edit(index.row(), newUrl, title);
    title = m_model->data(index, DeclarativeBookmarkModel::TitleRole).toString();
    url = m_model->data(index, DeclarativeBookmarkModel::UrlRole).toString();

    QCOMPARE(activeUrlBookmarkedChangedSpy.count(), 1);
    QCOMPARE(dataChangedSpy.count(), 2);
    QCOMPARE(title, newTitle);
    QCOMPARE(url, newUrl);

    index = m_model->index(0);
    url = m_model->data(index, DeclarativeBookmarkModel::UrlRole).toString();
    title = m_model->data(index, DeclarativeBookmarkModel::TitleRole).toString();

    QCOMPARE(title, constTitle);
    QCOMPARE(url, constUrl);

    QCOMPARE(m_model->rowCount(), 3);
}

void tst_declarativebookmarkmodel::setActiveUrl()
{
    QSignalSpy activeUrlBookmarkedChangedSpy(m_model, SIGNAL(activeUrlBookmarkedChanged()));
    QSignalSpy activeUrlChangedSpy(m_model, SIGNAL(activeUrlChanged()));

    QVERIFY(m_model->activeUrl() != QString(TEST_URL));
    m_model->setActiveUrl(TEST_URL);
    QCOMPARE(activeUrlBookmarkedChangedSpy.count(), 1);
    QCOMPARE(activeUrlChangedSpy.count(), 1);
    QCOMPARE(m_model->activeUrl(), QString(TEST_URL));

    QVERIFY(!m_model->activeUrlBookmarked());
    m_model->setActiveUrl(JOLLA_URL);
    QVERIFY(m_model->activeUrlBookmarked());
    QCOMPARE(activeUrlBookmarkedChangedSpy.count(), 2);
    QCOMPARE(activeUrlChangedSpy.count(), 2);
}

void tst_declarativebookmarkmodel::clearBookmarks()
{
    QSignalSpy countChangeSpy(m_model, SIGNAL(countChanged()));

    BookmarkManager::instance()->clear();

    QCOMPARE(countChangeSpy.count(), 1);
    QCOMPARE(m_model->rowCount(), 0);
}

void tst_declarativebookmarkmodel::updateFavoriteIcon()
{
    QSignalSpy dataChangedSpy(m_model, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)));

    m_model->updateFavoriteIcon(JOLLA_URL, "image://theme/icon-launcher-test1", false);
    QCOMPARE(dataChangedSpy.count(), 1);
}

void tst_declarativebookmarkmodel::data()
{
    QModelIndex index = m_model->index(0);

    foreach (const int role, m_model->roleNames().keys()) {
        QVERIFY(m_model->data(index, role).isValid());
    }

    // check non-existing role
    QVERIFY(!m_model->data(index, 10000).isValid());

    // check non-existing index
    index = m_model->index(1000);
    QVERIFY(!m_model->data(index, DeclarativeBookmarkModel::UrlRole).isValid());
}

QTEST_MAIN(tst_declarativebookmarkmodel)
#include "tst_declarativebookmarkmodel.moc"
