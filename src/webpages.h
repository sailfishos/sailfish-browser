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

#include <QObject>
#include <QMap>
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

    WebPageActivationData page(int tabId, int parentId = 0);
    void release(int tabId, bool virtualize = false);
    void clear();
    int parentTabId(int tabId) const;
    void dumpPages() const;

private:
    struct WebPageEntry {
        WebPageEntry(DeclarativeWebPage *webPage, QRectF *cssContentRect);
        ~WebPageEntry();

        DeclarativeWebPage *webPage;
        QRectF *cssContentRect;
        bool allowPageDelete;
    };

    void updateActivePage(WebPageEntry *webPageEntry, bool resurrect);

    QPointer<DeclarativeWebContainer> m_webContainer;
    QPointer<QQmlComponent> m_webPageComponent;
    // Contains both virtual and real
    QMap<int, WebPageEntry*> m_activePages;
    WebPageEntry *m_activePage;
    int m_count;

    friend class tst_webview;
};

#endif
