/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WEBPAGEQUEUE_H
#define WEBPAGEQUEUE_H

#include <QQueue>

class QRectF;
class DeclarativeWebPage;

class WebPageQueue {

public :
    explicit WebPageQueue();
    ~WebPageQueue();

    int count() const;
    bool alive(int tabId) const;
    bool active(int tabId) const;
    DeclarativeWebPage *activate(int tabId);
    DeclarativeWebPage *activeWebPage() const;
    void release(int tabId, bool virtualize = false);
    void prepend(int tabId, DeclarativeWebPage *webPage);
    void clear();
    int parentTabId(int tabId) const;

    bool setMaxLivePages(int count);
    int maxLivePages() const;
    void virtualizeInactive();

    void dumpPages() const;

private:
    struct WebPageEntry {
        WebPageEntry(DeclarativeWebPage *webPage, QRectF *cssContentRect);
        ~WebPageEntry();

        DeclarativeWebPage *webPage;
        int tabId;
        QRectF *cssContentRect;
        bool allowPageDelete;
    };

    void updateLivePages();
    WebPageEntry *find(int tabId, int &index) const;

    QList<WebPageEntry *> m_queue;
    int m_maxLiveCount;

    // This flag is set when we prepend a live page to the queue and reset upon
    // virtualization of inactive live pages as only one live page stays in the
    // queue.
    bool m_livePagePrepended;
};

#endif
