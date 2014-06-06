/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "desktopbookmarkwriter.h"

#include <MDesktopEntry>
#include <QtTest>
#include <QStringList>

class tst_desktopbookmarkwriter : public QObject
{
    Q_OBJECT

public:
    tst_desktopbookmarkwriter(QObject *parent = 0);

private slots:
    void invalidInput_data();
    void invalidInput();
    void writeDesktopFile_data();
    void writeDesktopFile();
    void clear();
    void exists();
    void cleanupTestCase();

private:
    QStringList desktopFiles;
    DesktopBookmarkWriter writer;
};


tst_desktopbookmarkwriter::tst_desktopbookmarkwriter(QObject *parent)
    : QObject(parent)
{
    DesktopBookmarkWriter::setTestModeEnabled(true);
}

void tst_desktopbookmarkwriter::invalidInput_data()
{
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("link");
    QTest::newRow("empty title") << "" << "http://www.test1.jolla.com";
    QTest::newRow("empty link") << "Test1" << "";
    QTest::newRow("link and title empty") << "" << "";
}

void tst_desktopbookmarkwriter::invalidInput()
{
    QFETCH(QString, title);
    QFETCH(QString, link);
    writer.setProperty("title", title);
    writer.setProperty("link", link);
    QVERIFY(!writer.save());

    writer.clear();
    QVERIFY(writer.property("title").toString().isEmpty());
    QVERIFY(writer.property("link").toString().isEmpty());
}

void tst_desktopbookmarkwriter::writeDesktopFile_data()
{
    QTest::addColumn<QString>("inputTitle");
    QTest::addColumn<QString>("outputTitle");
    QTest::addColumn<QString>("inputLink");
    QTest::addColumn<QString>("outputLink");
    QTest::addColumn<QString>("inputIcon");
    QTest::addColumn<QString>("outputIcon");
    QTest::addColumn<QString>("savedDesktopFile");

    QString testPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QTest::newRow("basic default icon")  << "Jolla" << "Jolla"
                            << "http://www.test1.jolla.com" << "http://www.test1.jolla.com"
                            << "" << QString(DEFAULT_DESKTOP_BOOKMARK_ICON)
                            << QString(DESKTOP_FILE_PATTERN).arg(testPath, "Jolla", "0");
    QTest::newRow("duplicate basic default icon")  << "Jolla" << "Jolla"
                            << "http://www.test1.jolla.com" << "http://www.test1.jolla.com"
                            << "" << QString(DEFAULT_DESKTOP_BOOKMARK_ICON)
                            << QString(DESKTOP_FILE_PATTERN).arg(testPath, "Jolla", "1");
    QTest::newRow("title surrounded with spaces")  << "   HelloWorld   " << "HelloWorld"
                            << "http://www.test1.jolla.com" << "http://www.test1.jolla.com"
                            << "" << QString(DEFAULT_DESKTOP_BOOKMARK_ICON)
                            << QString(DESKTOP_FILE_PATTERN).arg(testPath, "HelloWorld", "0");
    QTest::newRow("title spaces inside word")  << "   W  o   r  l  d   " << "W  o   r  l  d"
                            << "http://www.test1.jolla.com" << "http://www.test1.jolla.com"
                            << "" << QString(DEFAULT_DESKTOP_BOOKMARK_ICON)
                            << QString(DESKTOP_FILE_PATTERN).arg(testPath, "W-o-r-l-d", "0");
    QTest::newRow("link surrounded with spaces")  << "World" << "World"
                            << "   http://www.test1.jolla.com    " << "http://www.test1.jolla.com"
                            << "" << QString(DEFAULT_DESKTOP_BOOKMARK_ICON)
                            << QString(DESKTOP_FILE_PATTERN).arg(testPath, "World", "0");
    QTest::newRow("too small icon")  << "Test" << "Test"
                            << "http://www.test1.jolla.com" << "http://www.test1.jolla.com"
                            << QString("%1/%2").arg(TEST_DATA, "too-small-icon-size.png") << QString(DEFAULT_DESKTOP_BOOKMARK_ICON)
                            << QString(DESKTOP_FILE_PATTERN).arg(testPath, "Test", "0");
    QTest::newRow("large icon")  << "Test2" << "Test2"
                            << "http://www.test1.jolla.com" << "http://www.test1.jolla.com"
                            << QString("%1/%2").arg(TEST_DATA, "graphic-browsertutorial.png") << QString("data:image/png;base64")
                            << QString(DESKTOP_FILE_PATTERN).arg(testPath, "Test2", "0");
}

void tst_desktopbookmarkwriter::writeDesktopFile()
{
    QFETCH(QString, inputTitle);
    QFETCH(QString, outputTitle);
    QFETCH(QString, inputLink);
    QFETCH(QString, outputLink);
    QFETCH(QString, inputIcon);
    QFETCH(QString, outputIcon);
    QFETCH(QString, savedDesktopFile);

    writer.setProperty("title", inputTitle);
    writer.setProperty("link", inputLink);
    writer.setProperty("icon", inputIcon);

    QSignalSpy savedSpy(&writer, SIGNAL(saved(QString)));
    QVERIFY(writer.save());

    if (savedSpy.count() == 0) {
        savedSpy.wait();
    }

    QVERIFY(savedSpy.count() == 1);

    QList<QVariant> arguments = savedSpy.takeFirst();
    QString desktopFile = arguments.at(0).toString();
    desktopFiles << desktopFile;
    QCOMPARE(savedDesktopFile, desktopFile);

    MDesktopEntry desktopEntry(desktopFile);
    QCOMPARE(desktopEntry.name(), outputTitle);
    QCOMPARE(desktopEntry.url(), outputLink);
    QVERIFY(desktopEntry.icon().startsWith(outputIcon));
    QCOMPARE(desktopEntry.comment(), outputTitle);
}

void tst_desktopbookmarkwriter::clear()
{
    writer.setProperty("title", "Foo");
    writer.setProperty("link", "Bar");
    writer.setProperty("icon", "FooBar");
    writer.clear();

    QVERIFY(writer.property("title").toString().isEmpty());
    QVERIFY(writer.property("icon").toString().isEmpty());
    QVERIFY(writer.property("link").toString().isEmpty());
}

void tst_desktopbookmarkwriter::exists()
{
    QVERIFY(writer.exists(QString("%1/%2").arg(TEST_DATA, "graphic-browsertutorial.png")));
}

void tst_desktopbookmarkwriter::cleanupTestCase()
{
    foreach (QString desktopFile, desktopFiles) {
        QFile file(desktopFile);
        file.remove();
    }
}

QTEST_APPLESS_MAIN(tst_desktopbookmarkwriter)

#include "tst_desktopbookmarkwriter.moc"
