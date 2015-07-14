/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QUrl>
#include "declarativewebpage.h"

DeclarativeWebPage::DeclarativeWebPage(QObject *parent)
    : QObject(parent)
{
}

void DeclarativeWebPage::setContainer(DeclarativeWebContainer *)
{
}


void DeclarativeWebPage::setResurrectedContentRect(QVariant)
{

}

void DeclarativeWebPage::setInitialTab(const Tab&)
{
}

void DeclarativeWebPage::resetHeight(bool)
{
}

void DeclarativeWebPage::forceChrome(bool)
{
}

int DeclarativeWebPage::tabId() const
{
    return m_tabId;
}

bool DeclarativeWebPage::initialLoadHasHappened() const
{
    return false;
}

void DeclarativeWebPage::setInitialLoadHasHappened()
{
}

QUrl DeclarativeWebPage::url() const
{
    QUrl url;
    return url;
}

QString DeclarativeWebPage::title() const
{
    return m_title;
}

void DeclarativeWebPage::setTitle(const QString &title)
{
    if (title != m_title) {
        m_title = title;
        emit titleChanged();
    }
}

int DeclarativeWebPage::parentId() const
{
    return 0;
}

void DeclarativeWebPage::loadTab(QString newUrl, bool force)
{
    Q_UNUSED(newUrl)
    Q_UNUSED(force);
}

QDebug operator<<(QDebug dbg, const DeclarativeWebPage *page)
{
    Q_UNUSED(page);

    return dbg;
}
