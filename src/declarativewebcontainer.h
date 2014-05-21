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

#include "settingmanager.h"
#include "tab.h"
#include "webpages.h"

#include <QQuickItem>
#include <QPointer>
#include <QImage>
#include <QFutureWatcher>

class QTimerEvent;
class DeclarativeTabModel;
class DeclarativeWebPage;

class DeclarativeWebContainer : public QQuickItem {
    Q_OBJECT

    Q_PROPERTY(DeclarativeWebPage *contentItem READ webPage NOTIFY contentItemChanged FINAL)
    Q_PROPERTY(DeclarativeTabModel *tabModel READ tabModel WRITE setTabModel NOTIFY tabModelChanged FINAL)
    Q_PROPERTY(bool foreground READ foreground WRITE setForeground NOTIFY foregroundChanged FINAL)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(int maxLiveTabCount MEMBER m_maxLiveTabCount NOTIFY maxLiveTabCountChanged FINAL)
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
    Q_PROPERTY(QString initialUrl MEMBER m_initialUrl NOTIFY initialUrlChanged FINAL)
    Q_PROPERTY(QString thumbnailPath READ thumbnailPath NOTIFY thumbnailPathChanged FINAL)

    Q_PROPERTY(QQmlComponent* webPageComponent MEMBER m_webPageComponent NOTIFY webPageComponentChanged FINAL)

    Q_PROPERTY(bool deferredReload MEMBER m_deferredReload NOTIFY deferredReloadChanged FINAL)
    Q_PROPERTY(QVariant deferredLoad MEMBER m_deferredLoad NOTIFY deferredLoadChanged FINAL)

    // "private" properties.
    Q_PROPERTY(bool _readyToLoad READ readyToLoad WRITE setReadyToLoad NOTIFY _readyToLoadChanged FINAL)

public:
    DeclarativeWebContainer(QQuickItem *parent = 0);
    ~DeclarativeWebContainer();

    DeclarativeWebPage *webPage() const;

    DeclarativeTabModel *tabModel() const;
    void setTabModel(DeclarativeTabModel *model);

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
    QString thumbnailPath() const;

    bool readyToLoad() const;
    void setReadyToLoad(bool readyToLoad);

    bool isActiveTab(int tabId);

    Q_INVOKABLE void goForward();
    Q_INVOKABLE void goBack();
    Q_INVOKABLE bool activatePage(int tabId, bool force = false);
    Q_INVOKABLE void loadNewTab(QString url, QString title, int parentId);

    Q_INVOKABLE void captureScreen();
    Q_INVOKABLE void dumpPages() const;

signals:
    void contentItemChanged();
    void tabModelChanged();
    void pageStackChanged();
    void foregroundChanged();
    void backgroundChanged();
    void activeChanged();
    void maxLiveTabCountChanged();
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
    void initialUrlChanged();
    void thumbnailPathChanged();

    void deferredReloadChanged();
    void deferredLoadChanged();

    void _readyToLoadChanged();

    void webPageComponentChanged();
    void triggerLoad(QString url, QString title);

public slots:
    void resetHeight(bool respectContentHeight = true);

private slots:
    void imeNotificationChanged(int state, bool open, int cause, int focusChange, const QString& type);
    void windowVisibleChanged(bool visible);
    void handleWindowChanged(QQuickWindow *window);
    void screenCaptureReady();
    void onActiveTabChanged(int oldTabId, int activeTabId);
    void onModelLoaded();
    void onDownloadStarted();
    void onNewTabRequested(QString url, QString title);
    void onReadyToLoad();
    void onTabsCleared();
    void manageMaxTabCount();
    void releasePage(int tabId, bool virtualize = false);
    void closeWindow();
    void onPageUrlChanged();
    void onPageTitleChanged();
    void onPageThumbnailChanged(int tabId, QString path);
    void updateThumbnail();

    // These are here to inform embedlite-components that keyboard is open or close
    // matching composition metrics.
    void sendVkbOpenCompositionMetrics();

protected:
    void timerEvent(QTimerEvent *event);

private:
    void setWebPage(DeclarativeWebPage *webPage);
    void setThumbnailPath(QString thumbnailPath);
    qreal contentHeight() const;
    void captureScreen(int size, qreal rotate);
    int parentTabId(int tabId) const;
    void updateNavigationStatus(const Tab &tab);
    void updateVkbHeight();

    struct ScreenCapture {
        int tabId;
        QString path;
    };

    ScreenCapture saveToFile(QImage image, QRect cropBounds, int tabId, qreal rotate);

    QPointer<DeclarativeWebPage> m_webPage;
    QPointer<DeclarativeTabModel> m_model;
    QPointer<QQmlComponent> m_webPageComponent;
    QScopedPointer<WebPages> m_webPages;
    QScopedPointer<SettingManager> m_settingManager;
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
    QString m_thumbnailPath;
    QString m_initialUrl;

    bool m_loading;
    int m_loadProgress;
    bool m_canGoForward;
    bool m_canGoBack;
    bool m_realNavigation;
    bool m_readyToLoad;
    int m_maxLiveTabCount;

    QFutureWatcher<ScreenCapture> m_screenCapturer;

    bool m_deferredReload;
    QVariant m_deferredLoad;

    friend class tst_webview;
};

QML_DECLARE_TYPE(DeclarativeWebContainer)

#endif // DECLARATIVEWEBCONTAINER_H
