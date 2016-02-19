/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "browserpaths.h"
#include "downloadmimetypehandler.h"

#include <QObject>
#include <QByteArray>
#include <QFile>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>
#include <QtTest>

#define APTOIDE_MYAPP "application/vnd.cm.aptoide.pt myapp"

static const QByteArray COMMENTED_OUT = "foobar app\n" \
        "#" APTOIDE_MYAPP "\n";

static const QByteArray COMMENTED_OUT_WITH_SPACE = "foobar app\n" \
        "#\t\n" APTOIDE_MYAPP "\t\t\n";

static const QByteArray BROKEN_MIME_TYPE = "foobar app\n" \
        "application / vnd.cm.aptoide.pt myapp\n";

static const QByteArray HAS_CONTENT_TYPE = "foobar app\n" \
        APTOIDE_MYAPP "\n";

static const QByteArray NO_CONTENT_TYPE = "foobar app\n" \
        "hello world\n";


class tst_downloadmimetypehandler : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void defaultMimeTypes_data();
    void defaultMimeTypes();

private:
    void writeTestFile(const QString &path, const QByteArray &data);

    QMap<QString, QString> mTestFiles;
};


void tst_downloadmimetypehandler::initTestCase()
{
    QString fileName = QString("%1/commented_out").arg(BrowserPaths::dataLocation());
    writeTestFile(fileName, COMMENTED_OUT);
    mTestFiles.insert("commented_out", fileName);

    fileName = QString("%1/commented_out_with_spaces").arg(BrowserPaths::dataLocation());
    writeTestFile(fileName, COMMENTED_OUT_WITH_SPACE);
    mTestFiles.insert("commented_out_with_spaces", fileName);

    fileName = QString("%1/broken_mime_type").arg(BrowserPaths::dataLocation());
    writeTestFile(fileName, BROKEN_MIME_TYPE);
    mTestFiles.insert("broken_mime_type", fileName);

    fileName = QString("%1/has_content_type").arg(BrowserPaths::dataLocation());
    writeTestFile(fileName, HAS_CONTENT_TYPE);
    mTestFiles.insert("has_content_type", fileName);

    fileName = QString("%1/no_content_type").arg(BrowserPaths::dataLocation());
    writeTestFile(fileName, NO_CONTENT_TYPE);
    mTestFiles.insert("no_content_type", fileName);

    mTestFiles.insert("unknown_file", QString("%1/unknown_file").arg(BrowserPaths::dataLocation()));
}

void tst_downloadmimetypehandler::cleanupTestCase()
{
    QDir dir(BrowserPaths::dataLocation());
    dir.removeRecursively();
}

void tst_downloadmimetypehandler::defaultMimeTypes_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<int>("hasMimeTypes");

    QTest::newRow("commented_out") << mTestFiles.value("commented_out") << 1;
    QTest::newRow("commented_out_with_spaces") << mTestFiles.value("commented_out_with_spaces") << 1;
    QTest::newRow("has_content_type") << mTestFiles.value("has_content_type") << 1;
    QTest::newRow("no_content_type") << mTestFiles.value("no_content_type") << 0;
    QTest::newRow("broken_mime_type") << mTestFiles.value("broken_mime_type") << 0;
    QTest::newRow("unknown_file") << mTestFiles.value("unknown_file") << -1;
}

void tst_downloadmimetypehandler::defaultMimeTypes()
{
    QFETCH(QString, fileName);
    QFETCH(int, hasMimeTypes);
    QCOMPARE(DownloadMimetypeHandler::hasDefaults(fileName), hasMimeTypes);

    if (hasMimeTypes < 1) {
        DownloadMimetypeHandler::appendDefaults(fileName);
        QCOMPARE(DownloadMimetypeHandler::hasDefaults(fileName), 1);
    }
}

void tst_downloadmimetypehandler::writeTestFile(const QString &path, const QByteArray &data)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << data;
    endl(out);
    file.close();
}

QTEST_GUILESS_MAIN(tst_downloadmimetypehandler)

#include "tst_downloadmimetypehandler.moc"
