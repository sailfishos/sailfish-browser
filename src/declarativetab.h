/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVETAB_H
#define DECLARATIVETAB_H

#include <QImage>
#include <QObject>
#include <QQuickItem>
#include <QStringList>
#include <QFutureWatcher>

#include "tab.h"
#include "link.h"

class DeclarativeTab : public QQuickItem {
    Q_OBJECT

    Q_PROPERTY(bool valid READ valid NOTIFY validChanged FINAL)
    Q_PROPERTY(QString thumbnailPath READ thumbnailPath WRITE setThumbnailPath NOTIFY thumbPathChanged FINAL)
    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged FINAL)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY canGoForwardChanged FINAL)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY canGoBackChanged FINAL)
    Q_PROPERTY(bool backForwardNavigation READ backForwardNavigation WRITE setBackForwardNavigation NOTIFY backForwardNavigationChanged FINAL)

public:
    DeclarativeTab(QQuickItem *parent = 0);
    ~DeclarativeTab();

    QString thumbnailPath() const;
    void setThumbnailPath(QString thumbnailPath);

    QString url() const;
    void setUrl(QString url);

    QString title() const;
    void setTitle(QString title);

    int tabId() const;

    bool valid() const;
    void invalidate();

    bool canGoForward() const;
    bool canGoBack() const;

    bool backForwardNavigation() const;
    void setBackForwardNavigation(bool backForwardNavigation);

    Q_INVOKABLE void goForward();
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void captureScreen(QString url, int x, int y, int width, int height, qreal rotate);

public slots:
    void tabChanged(Tab tab);
    void updateThumbPath(QString url, QString path, int tabId);

private slots:
    void screenCaptureReady();

signals:
    void thumbPathChanged(QString path, int tabId);
    void urlChanged();
    void validChanged();
    void titleChanged();
    void canGoForwardChanged();
    void canGoBackChanged();
    void backForwardNavigationChanged();

private:
    void setTabId(int tabId);

    struct ScreenCapture {
        int tabId;
        QString path;
        QString url;
    };

    void init();
    // Grep following todos
    // TODO: Remove url parameter from this, worker, and manager.
    ScreenCapture saveToFile(QString url, QImage image, QRect cropBounds, int tabId, qreal rotate);

    int m_tabId;
    bool m_valid;
    Link m_link;
    bool m_canGoForward, m_canGoBack;
    bool m_backForwardNavigation;
    QFutureWatcher<ScreenCapture> m_screenCapturer;
};

#endif // DECLARATIVETAB_H
