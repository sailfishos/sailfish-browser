/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVEWEBCONTAINER_H
#define DECLARATIVEWEBCONTAINER_H

#include "tab.h"

#include <QQuickItem>
#include <QPointer>
#include <QImage>
#include <QFutureWatcher>

class QTimerEvent;
class DeclarativeTab;
class DeclarativeTabModel;
class DeclarativeWebPage;

class DeclarativeWebContainer : public QQuickItem {
    Q_OBJECT

    Q_PROPERTY(DeclarativeWebPage *contentItem READ webPage WRITE setWebPage NOTIFY contentItemChanged FINAL)
    Q_PROPERTY(bool foreground READ foreground WRITE setForeground NOTIFY foregroundChanged FINAL)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged FINAL)
    // This property should cover all possible popus
    Q_PROPERTY(bool popupActive MEMBER m_popupActive NOTIFY popupActiveChanged FINAL)
    Q_PROPERTY(bool portrait MEMBER m_portrait NOTIFY portraitChanged FINAL)
    Q_PROPERTY(bool fullscreenMode MEMBER m_fullScreenMode NOTIFY fullscreenModeChanged FINAL)
    Q_PROPERTY(bool inputPanelVisible READ inputPanelVisible NOTIFY inputPanelVisibleChanged FINAL)
    Q_PROPERTY(qreal inputPanelHeight READ inputPanelHeight WRITE setInputPanelHeight NOTIFY inputPanelHeightChanged FINAL)
    Q_PROPERTY(qreal inputPanelOpenHeight MEMBER m_inputPanelOpenHeight NOTIFY inputPanelOpenHeightChanged FINAL)
    Q_PROPERTY(qreal toolbarHeight MEMBER m_toolbarHeight NOTIFY toolbarHeightChanged FINAL)
    Q_PROPERTY(bool background READ background NOTIFY backgroundChanged FINAL)

    Q_PROPERTY(QString favicon MEMBER m_favicon NOTIFY faviconChanged)

    Q_PROPERTY(bool loading MEMBER m_loading NOTIFY loadingChanged FINAL)
    Q_PROPERTY(int loadProgress READ loadProgress WRITE setLoadProgress NOTIFY loadProgressChanged FINAL)

    // Navigation related properties
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY canGoForwardChanged FINAL)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY canGoBackChanged FINAL)

    Q_PROPERTY(QString title READ title NOTIFY titleChanged FINAL)
    Q_PROPERTY(QString url READ url NOTIFY urlChanged FINAL)

    Q_PROPERTY(DeclarativeTab *currentTab READ currentTab NOTIFY currentTabChanged FINAL)

    // "private" properties.
    Q_PROPERTY(bool _firstFrameRendered MEMBER m_firstFrameRendered NOTIFY _firstFrameRenderedChanged FINAL)
    Q_PROPERTY(bool _readyToLoad READ readyToLoad WRITE setReadyToLoad NOTIFY _readyToLoadChanged FINAL)

public:
    DeclarativeWebContainer(QQuickItem *parent = 0);
    ~DeclarativeWebContainer();

    DeclarativeWebPage *webPage() const;
    void setWebPage(DeclarativeWebPage *webPage);

    bool foreground() const;
    void setForeground(bool active);

    bool background() const;

    int loadProgress() const;
    void setLoadProgress(int loadProgress);

    bool active() const;
    void setActive(bool active);

    bool inputPanelVisible() const;

    qreal inputPanelHeight() const;
    void setInputPanelHeight(qreal height);

    bool canGoForward() const;
    void setCanGoForward(bool canGoForward);

    bool canGoBack() const;
    void setCanGoBack(bool canGoBack);

    QString title() const;
    QString url() const;

    DeclarativeTab *currentTab() const;

    bool readyToLoad() const;
    void setReadyToLoad(bool readyToLoad);

    Q_INVOKABLE void goForward();
    Q_INVOKABLE void goBack();

    Q_INVOKABLE void captureScreen();

signals:
    void contentItemChanged();
    void pageStackChanged();
    void foregroundChanged();
    void backgroundChanged();
    void activeChanged();
    void popupActiveChanged();
    void portraitChanged();
    void fullscreenModeChanged();
    void inputPanelVisibleChanged();
    void inputPanelHeightChanged();
    void inputPanelOpenHeightChanged();
    void toolbarHeightChanged();

    void faviconChanged();
    void loadingChanged();
    void loadProgressChanged();

    void canGoForwardChanged();
    void canGoBackChanged();

    void titleChanged();
    void urlChanged();

    void _firstFrameRenderedChanged();
    void _readyToLoadChanged();

    void currentTabChanged();

public slots:
    void resetHeight(bool respectContentHeight = true);

private slots:
    void imeNotificationChanged(int state, bool open, int cause, int focusChange, const QString& type);
    void windowVisibleChanged(bool visible);
    void handleWindowChanged(QQuickWindow *window);
    void updateTabData(const Tab &tab);
    void invalidateTabData();
    void screenCaptureReady();
    void triggerLoad();

protected:
    void timerEvent(QTimerEvent *event);
    void componentComplete();

private:
    qreal contentHeight() const;
    void captureScreen(QString url, int size, qreal rotate);

    struct ScreenCapture {
        int tabId;
        QString path;
        QString url;
    };

    // Grep following todos
    // TODO: Remove url parameter from this, worker, and manager.
    ScreenCapture saveToFile(QString url, QImage image, QRect cropBounds, int tabId, qreal rotate);

    QPointer<DeclarativeWebPage> m_webPage;
    QPointer<DeclarativeTabModel> m_model;
    QPointer<DeclarativeTab> m_currentTab;
    bool m_foreground;
    bool m_background;
    bool m_windowVisible;
    int m_backgroundTimer;
    bool m_active;
    bool m_popupActive;
    bool m_portrait;
    bool m_fullScreenMode;
    bool m_inputPanelVisible;
    qreal m_inputPanelHeight;
    qreal m_inputPanelOpenHeight;
    qreal m_toolbarHeight;

    QString m_favicon;

    bool m_loading;
    int m_loadProgress;
    bool m_canGoForward;
    bool m_canGoBack;
    bool m_realNavigation;
    bool m_firstFrameRendered;
    bool m_readyToLoad;

    QFutureWatcher<ScreenCapture> m_screenCapturer;
};

QML_DECLARE_TYPE(DeclarativeWebContainer)

#endif // DECLARATIVEWEBCONTAINER_H
