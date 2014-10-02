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

    QString writtenDesktopFile(QSignalSpy &spy);

private slots:
    void invalidInput_data();
    void invalidInput();
    void writeDesktopFile_data();
    void writeDesktopFile();
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

QString tst_desktopbookmarkwriter::writtenDesktopFile(QSignalSpy &spy)
{
    if (spy.count() == 0) {
        spy.wait();
    }

    if (spy.count() == 1) {
        QList<QVariant> arguments = spy.takeFirst();
        return arguments.at(0).toString();
    } else {
        return "";
    }
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
    QSignalSpy savedSpy(&writer, SIGNAL(saved(QString)));
    writer.save(link, title, "hello.png");
    QString desktopFile = writtenDesktopFile(savedSpy);
    QVERIFY(desktopFile.isEmpty());
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

    QSignalSpy savedSpy(&writer, SIGNAL(saved(QString)));
    writer.save(inputLink, inputTitle, inputIcon);
    QString desktopFile = writtenDesktopFile(savedSpy);

    desktopFiles << desktopFile;
    QCOMPARE(savedDesktopFile, desktopFile);

    MDesktopEntry desktopEntry(desktopFile);
    QCOMPARE(desktopEntry.name(), outputTitle);
    QCOMPARE(desktopEntry.url(), outputLink);
    QVERIFY(desktopEntry.icon().startsWith(outputIcon));
    QCOMPARE(desktopEntry.comment(), outputTitle);
}

void tst_desktopbookmarkwriter::cleanupTestCase()
{
    foreach (QString desktopFile, desktopFiles) {
        QFile file(desktopFile);
        file.remove();
    }
}

QTEST_GUILESS_MAIN(tst_desktopbookmarkwriter)

#include "tst_desktopbookmarkwriter.moc"
