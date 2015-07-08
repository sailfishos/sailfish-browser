/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVEWEBPAGE_H
#define DECLARATIVEWEBPAGE_H

#include <QObject>
#include <QDebug>

class Tab;
class DeclarativeWebContainer;

class DeclarativeWebPage : public QObject
{
    Q_OBJECT

public:
    explicit DeclarativeWebPage();

    void setContainer(DeclarativeWebContainer *);

    void setResurrectedContentRect(QVariant);
    void setInitialTab(const Tab&);

    void resetHeight(bool);
    void forceChrome(bool);

    int tabId() const;
    bool initialLoadHasHappened() const;
    void setInitialLoadHasHappened();

    Q_INVOKABLE void loadTab(QString newUrl, bool force);
signals:
    void containerChanged();
    void tabIdChanged();
    void forcedChromeChanged();
    void fullscreenChanged();
    void domContentLoadedChanged();
    void resurrectedContentRectChanged();
    void clearGrabResult();
    void grabResult(QString fileName);
    void thumbnailResult(QString data);
};

QDebug operator<<(QDebug, const DeclarativeWebPage *);

#endif
