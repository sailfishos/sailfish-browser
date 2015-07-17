/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DESKTOPBOOKMARKWRITER_H
#define DESKTOPBOOKMARKWRITER_H

#include <QObject>
#include <QFutureWatcher>

class DesktopBookmarkWriter : public QObject
{
    Q_OBJECT

public:
    explicit DesktopBookmarkWriter(QObject *parent = 0);
    ~DesktopBookmarkWriter();

    static void setTestModeEnabled(bool testMode);
    static bool isTestModeEnabled();

    Q_INVOKABLE void save(QString url, QString title, QString icon);

signals:
    void saved(QString desktopFile);

private slots:
    void desktopFileWritten();

private:
    QString uniqueDesktopFileName(QString title);
    QString write(QString url, QString title, QString icon);

    QFutureWatcher<QString> m_writter;

    friend class tst_desktopbookmarkwriter;
};

#endif // DESKTOPBOOKMARKWRITER_H
