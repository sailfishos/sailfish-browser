/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WEBPAGES_H
#define WEBPAGES_H

#include "webpagequeue.h"

#include <QObject>
#include <QPointer>

class QQmlComponent;
class DeclarativeWebContainer;
class DeclarativeWebPage;

struct WebPageActivationData {
    WebPageActivationData(DeclarativeWebPage *webPage, bool activated)
        : webPage(webPage)
        , activated(activated)
    {}

    DeclarativeWebPage *webPage;
    bool activated;
};

class WebPages : public QObject
{
    Q_OBJECT

public:
    explicit WebPages(QObject *parent = 0);
    ~WebPages();

    void initialize(DeclarativeWebContainer *webContainer, QQmlComponent *webPageComponent);
    bool initialized() const;
    int count() const;

    bool setMaxLivePages(int count);
    int maxLivePages() const;

    bool alive(int tabId) const;

    WebPageActivationData page(int tabId, int parentId = 0);
    void release(int tabId, bool virtualize = false);
    void clear();
    int parentTabId(int tabId) const;
    void dumpPages() const;

private slots:
    void handleMemNotify(const QString &memoryLevel);
    void updateBackgroundTimestamp();

private:
    void updateStates(DeclarativeWebPage *oldActivePage, DeclarativeWebPage *newActivePage);

    QPointer<DeclarativeWebContainer> m_webContainer;
    QPointer<QQmlComponent> m_webPageComponent;
    // Contains both virtual and real
    WebPageQueue m_activePages;
    qint64 m_backgroundTimestamp;

    friend class tst_webview;
};

#endif
