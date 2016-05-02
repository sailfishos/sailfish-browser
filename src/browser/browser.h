/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BROWSER_H
#define BROWSER_H

#include <QObject>

class QQuickView;
class BrowserPrivate;

class Browser : public QObject
{
    Q_OBJECT

public:
    explicit Browser(QQuickView *view, QObject *parent = 0);

    void load();

    static QString applicationFilePath();

public slots:
    void openUrl(const QString &url);
    void openNewTabView();

    void cancelDownload(int transferId);
    void restartDownload(int transferid);

    // Debug helpers
    void dumpMemoryInfo(const QString &fileName);

private:
    BrowserPrivate *d_ptr;
    Q_DISABLE_COPY(Browser)
    Q_DECLARE_PRIVATE(Browser)
};

#endif // BROWSER_H
