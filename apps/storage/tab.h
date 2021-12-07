/****************************************************************************
**
** Copyright (c) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef TAB_H
#define TAB_H

#include <QString>
#include <QDebug>

class Tab
{
public:
    explicit Tab();
    explicit Tab(int tabId, const QString &url, const QString &title, const QString &thumbPath);

    int tabId() const;
    void setTabId(int tabId);

    void setRequestedUrl(const QString &url);
    QString requestedUrl() const;

    QString url() const;
    void setUrl(const QString &url);

    bool hasResolvedUrl() const;

    QString thumbnailPath() const;
    void setThumbnailPath(const QString &thumbnailPath);

    QString title() const;
    void setTitle(const QString &title);

    bool desktopMode() const;
    void setDesktopMode(bool desktopMode);

    bool isValid() const;

    bool operator==(const Tab &other) const;
    bool operator!=(const Tab &other) const;

private:
    int m_tabId;
    QString m_requestedUrl;
    QString m_url;
    QString m_title;
    QString m_thumbPath;
    bool m_desktopMode;
};

Q_DECLARE_METATYPE(Tab)

QDebug operator<<(QDebug, const Tab *);

#endif // TAB_H
