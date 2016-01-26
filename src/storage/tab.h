/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
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

#include "link.h"

class Tab
{
public:
    explicit Tab(int tabId, Link currentLink);
    explicit Tab();

    int tabId() const;
    void setTabId(int tabId);

    int currentLink() const;
    void setCurrentLink(int currentLinkId);

    QString url() const;
    void setUrl(const QString &url);

    QString thumbnailPath() const;
    void setThumbnailPath(const QString &thumbnailPath);

    QString title() const;
    void setTitle(const QString &title);

    bool isValid() const;

    bool operator==(const Tab &other) const;
    bool operator!=(const Tab &other) const;

private:
    int m_tabId;
    Link m_currentLink;
    int m_nextLinkId;
};

QDebug operator<<(QDebug, const Tab *);

#endif // TAB_H
