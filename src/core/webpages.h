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
class WebPageFactory;
class QDBusPendingCallWatcher;
class DeclarativeWebContainer;
class DeclarativeWebPage;
class Tab;

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
    explicit WebPages(WebPageFactory *pageFactory, QObject *parent = 0);
    ~WebPages();

    void initialize(DeclarativeWebContainer *webContainer);
    bool initialized() const;
    int count() const;

    bool setMaxLivePages(int count);
    int maxLivePages() const;

    bool alive(int tabId) const;

    WebPageActivationData page(const Tab& tab, int parentId = 0);
    void release(int tabId);
    void clear();
    int parentTabId(int tabId) const;
    void dumpPages() const;

private slots:
    void handleMemNotify(const QString &memoryLevel);
    void updateBackgroundTimestamp();
    void initialMemoryLevel(QDBusPendingCallWatcher *watcher);
    void delayVirtualization();

private:
    void updateStates(DeclarativeWebPage *oldActivePage, DeclarativeWebPage *newActivePage);

    QPointer<DeclarativeWebContainer> m_webContainer;
    QPointer<WebPageFactory> m_pageFactory;
    // Contains both virtual and real
    WebPageQueue m_activePages;
    qint64 m_backgroundTimestamp;
    QString m_memoryLevel;

    friend class tst_webview;
    friend class tst_webpages;
};

#endif
