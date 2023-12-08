/****************************************************************************
**
** Copyright (c) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LINK_H
#define LINK_H

#include <QString>
#include <QDebug>
#include <QDate>

class Link
{
public:
    explicit Link(int linkId, const QString &url, const QString &thumbPath, const QString &title, const QDate &date = QDate());
    explicit Link();

    int linkId() const;
    void setLinkId(int linkId);

    QString url() const;
    void setUrl(const QString &url);

    QString thumbPath() const;
    void setThumbPath(const QString &thumbPath);

    QString title() const;
    void setTitle(const QString &title);

    bool isValid() const;

    bool operator==(const Link &other) const;
    bool operator!=(const Link &other) const;

    QDate date() const;
    void setDate(const QDate &date);

private:
    int m_linkId;
    QString m_url;
    QString m_thumbPath;
    QString m_title;
    QDate m_date;
};

QDebug operator<<(QDebug, const Link *);

#endif // LINK_H
