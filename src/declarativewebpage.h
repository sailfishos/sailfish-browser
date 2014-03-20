/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVEWEBPAGE_H
#define DECLARATIVEWEBPAGE_H

#include <qqml.h>
#include <QPointer>
#include <quickmozview.h>
#include <declarativewebcontainer.h>

class DeclarativeWebPage : public QuickMozView {
    Q_OBJECT
    Q_PROPERTY(DeclarativeWebContainer* container MEMBER m_container NOTIFY containerChanged FINAL)
    Q_PROPERTY(int tabId MEMBER m_tabId NOTIFY tabIdChanged FINAL)
    Q_PROPERTY(bool viewReady MEMBER m_viewReady NOTIFY viewReadyChanged FINAL)
    Q_PROPERTY(bool loaded MEMBER m_loaded NOTIFY loadedChanged FINAL)
    Q_PROPERTY(bool userHasDraggedWhileLoading MEMBER m_userHasDraggedWhileLoading NOTIFY userHasDraggedWhileLoadingChanged FINAL)
    Q_PROPERTY(QString favicon MEMBER m_favicon NOTIFY faviconChanged FINAL)
    Q_PROPERTY(QVariant resurrectedContentRect MEMBER m_resurrectedContentRect NOTIFY resurrectedContentRectChanged)

    // Private
    Q_PROPERTY(bool _deferredReload MEMBER m_deferredReload NOTIFY _deferredReloadChanged FINAL)
    Q_PROPERTY(QVariant _deferredLoad MEMBER m_deferredLoad NOTIFY _deferredLoadChanged FINAL)

public:
    DeclarativeWebPage(QuickMozView *parent = 0);
    ~DeclarativeWebPage();

signals:
    void containerChanged();
    void tabIdChanged();
    void viewReadyChanged();
    void loadedChanged();
    void userHasDraggedWhileLoadingChanged();
    void faviconChanged();
    void resurrectedContentRectChanged();

    void _deferredReloadChanged();
    void _deferredLoadChanged();

protected:
    void componentComplete();

private:
    QPointer<DeclarativeWebContainer> m_container;
    int m_tabId;
    bool m_viewReady;
    bool m_loaded;
    bool m_userHasDraggedWhileLoading;
    QString m_favicon;
    QVariant m_resurrectedContentRect;

    bool m_deferredReload;
    QVariant m_deferredLoad;
};

QML_DECLARE_TYPE(DeclarativeWebPage)

#endif
