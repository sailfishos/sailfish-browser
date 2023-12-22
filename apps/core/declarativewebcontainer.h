/*
 * Copyright (c) 2013 - 2021 Jolla Ltd.
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DECLARATIVEWEBCONTAINER_H
#define DECLARATIVEWEBCONTAINER_H

#include <qmozcontext.h>
#include <qmozsecurity.h>
#include <QtGui/QWindow>
#include <QtGui/QOpenGLFunctions>
#include <QPointer>
#include <QFutureWatcher>
#include <QQmlComponent>
#include <QQuickView>
#include <QQuickItem>
#include <QMutex>
#include <QWaitCondition>

class QInputMethodEvent;
class QMozWindow;
class QTimerEvent;
class DeclarativeTabModel;
class DeclarativeWebPage;
class WebPages;
class Tab;
class DeclarativeHistoryModel;
class CloseEventFilter;

class DeclarativeWebContainer : public QWindow, public QQmlParserStatus, protected QOpenGLFunctions
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QQuickItem *rotationHandler MEMBER m_rotationHandler NOTIFY rotationHandlerChanged FINAL)
    Q_PROPERTY(DeclarativeWebPage *contentItem READ webPage NOTIFY contentItemChanged FINAL)
    Q_PROPERTY(DeclarativeTabModel *tabModel READ tabModel NOTIFY tabModelChanged FINAL)
    Q_PROPERTY(DeclarativeTabModel *persistentTabModel READ persistentTabModel CONSTANT)
    Q_PROPERTY(DeclarativeTabModel *privateTabModel READ privateTabModel CONSTANT)
    Q_PROPERTY(bool completed READ completed NOTIFY completedChanged FINAL)
    Q_PROPERTY(bool enabled MEMBER m_enabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool foreground READ foreground WRITE setForeground NOTIFY foregroundChanged FINAL)
    Q_PROPERTY(int maxLiveTabCount READ maxLiveTabCount WRITE setMaxLiveTabCount NOTIFY maxLiveTabCountChanged FINAL)
    // This property should cover all possible popus
    Q_PROPERTY(bool touchBlocked MEMBER m_touchBlocked NOTIFY touchBlockedChanged FINAL)
    Q_PROPERTY(bool selectionActive MEMBER m_selectionActive NOTIFY selectionActiveChanged FINAL)
    Q_PROPERTY(bool portrait READ portrait WRITE setPortrait NOTIFY portraitChanged FINAL)
    Q_PROPERTY(bool fullscreenMode MEMBER m_fullScreenMode NOTIFY fullscreenModeChanged FINAL)
    Q_PROPERTY(qreal fullscreenHeight MEMBER m_fullScreenHeight NOTIFY fullscreenHeightChanged FINAL)
    Q_PROPERTY(bool imOpened MEMBER m_imOpened NOTIFY imOpenedChanged FINAL)
    Q_PROPERTY(qreal toolbarHeight MEMBER m_toolbarHeight NOTIFY toolbarHeightChanged FINAL)
    Q_PROPERTY(bool allowHiding MEMBER m_allowHiding NOTIFY allowHidingChanged FINAL)

    Q_PROPERTY(QString favicon MEMBER m_favicon NOTIFY faviconChanged)

    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged FINAL)
    Q_PROPERTY(int loadProgress READ loadProgress NOTIFY loadProgressChanged FINAL)

    // Navigation related properties
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY canGoForwardChanged FINAL)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY canGoBackChanged FINAL)

    Q_PROPERTY(int tabId READ tabId NOTIFY tabIdChanged FINAL)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged FINAL)
    Q_PROPERTY(QString url READ url NOTIFY urlChanged FINAL)

    Q_PROPERTY(bool privateMode READ privateMode WRITE setPrivateMode NOTIFY privateModeChanged FINAL)
    Q_PROPERTY(bool activeTabRendered READ activeTabRendered NOTIFY activeTabRenderedChanged FINAL)

    Q_PROPERTY(QQmlComponent* webPageComponent READ webPageComponent WRITE setWebPageComponent NOTIFY webPageComponentChanged FINAL)
    Q_PROPERTY(QObject *chromeWindow READ chromeWindow WRITE setChromeWindow NOTIFY chromeWindowChanged FINAL)
    Q_PROPERTY(bool readyToPaint READ readyToPaint WRITE setReadyToPaint NOTIFY readyToPaintChanged FINAL)

    Q_PROPERTY(Qt::ScreenOrientation pendingWebContentOrientation READ pendingWebContentOrientation NOTIFY pendingWebContentOrientationChanged FINAL)

    Q_PROPERTY(QMozSecurity *security READ security NOTIFY securityChanged)
    Q_PROPERTY(DeclarativeHistoryModel* historyModel READ historyModel WRITE setHistoryModel NOTIFY historyModelChanged)

    Q_PROPERTY(bool hasInitialUrl READ hasInitialUrl NOTIFY hasInitialUrlChanged)

public:
    DeclarativeWebContainer(QWindow *parent = 0);
    ~DeclarativeWebContainer();

    static DeclarativeWebContainer *instance();

    DeclarativeWebPage *webPage() const;
    QMozWindow *mozWindow() const;

    DeclarativeTabModel *tabModel() const;
    DeclarativeTabModel *persistentTabModel() const;
    DeclarativeTabModel *privateTabModel() const;

    bool completed() const;

    bool foreground() const;
    void setForeground(bool active);

    int maxLiveTabCount() const;
    void setMaxLiveTabCount(int count);

    bool portrait() const;
    void setPortrait(bool portrait);

    QQmlComponent* webPageComponent() const;
    void setWebPageComponent(QQmlComponent* qmlComponent);

    bool privateMode() const;
    void setPrivateMode(bool);

    bool activeTabRendered() const;

    bool loading() const;

    int loadProgress() const;
    void setLoadProgress(int loadProgress);

    bool imOpened() const;

    bool canGoForward() const;
    bool canGoBack() const;

    QObject *chromeWindow() const;
    void setChromeWindow(QObject *chromeWindow);

    bool readyToPaint() const;
    void setReadyToPaint(bool ready);

    Qt::ScreenOrientation pendingWebContentOrientation() const;

    QMozSecurity *security() const;

    int tabId() const;
    QString title() const;
    QString url() const;
    QString thumbnailPath() const;

    bool isActiveTab(int tabId);
    bool activatePage(const Tab& tab, bool force = false);
    int tabId(uint32_t uniqueId) const;
    int previouslyUsedTabId() const;
    // For D-Bus interfaces
    uint tabOwner(int tabId) const;
    int requestTabWithOwner(int tabId, const QString &url, uint ownerPid);
    void requestTabWithOwnerAsync(int tabId, const QString &url, uint ownerPid, void *context);
    Q_INVOKABLE void releaseActiveTabOwnership();

    Q_INVOKABLE void load(const QString &url = QString(), bool force = false);
    Q_INVOKABLE void reload(bool force = true);
    Q_INVOKABLE void goForward();
    Q_INVOKABLE void goBack();

    Q_INVOKABLE int activateTab(int tabId, const QString &url);
    Q_INVOKABLE void closeTab(int tabId);

    Q_INVOKABLE void updatePageFocus(bool focus);
    Q_INVOKABLE void dumpPages() const;

    QObject *focusObject() const;

    bool event(QEvent *event);

    DeclarativeHistoryModel *historyModel() const;
    void setHistoryModel(DeclarativeHistoryModel *model);

    bool hasInitialUrl() const;

signals:
    void rotationHandlerChanged();
    void contentItemChanged();
    void tabModelChanged();
    void completedChanged();
    void enabledChanged();
    void foregroundChanged();
    void allowHidingChanged();
    void maxLiveTabCountChanged();
    void touchBlockedChanged();
    void selectionActiveChanged();
    void portraitChanged();
    void fullscreenModeChanged();
    void fullscreenHeightChanged();
    void imOpenedChanged();
    void toolbarHeightChanged();

    void faviconChanged();
    void loadingChanged();
    void loadProgressChanged();

    void canGoForwardChanged();
    void canGoBackChanged();

    void tabIdChanged();
    void titleChanged();
    void urlChanged();
    void thumbnailPathChanged();
    void privateModeChanged();
    void activeTabRenderedChanged();

    void webPageComponentChanged(QQmlComponent *newComponent);
    void chromeWindowChanged();
    void chromeExposed();
    void readyToPaintChanged();

    void pendingWebContentOrientationChanged();
    void webContentOrientationChanged(Qt::ScreenOrientation orientation);
    void securityChanged();
    void historyModelChanged();
    void applicationClosing();

    void hasInitialUrlChanged();
    void requestTabWithOwnerAsyncResult(int tabId, void *context);

protected:
    bool eventFilter(QObject *obj, QEvent *event);
    virtual void exposeEvent(QExposeEvent *event);
    virtual void touchEvent(QTouchEvent *event);
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery property) const;
    virtual void inputMethodEvent(QInputMethodEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
    virtual void focusInEvent(QFocusEvent *event);
    virtual void focusOutEvent(QFocusEvent *event);
    virtual void timerEvent(QTimerEvent *event);
    virtual void classBegin();
    virtual void componentComplete();

public slots:
    void updateContentOrientation(Qt::ScreenOrientation orientation);
    void clearSurface();
    void dsmeStateChange(const QString &state);

private slots:
    void initialize();
    void onActiveTabChanged(int activeTabId);
    void onDownloadStarted();
    void onNewTabRequested(const Tab &tab);
    void releasePage(int tabId);
    void closeWindow();
    void updateLoadProgress();
    void updateLoading();
    void updateActiveTabRendered();
    void onLastViewDestroyed();

    void onLastWindowDestroyed();
    void updateWindowFlags();

    // QMozWindow related slots:
    void createGLContext();

    void handleContentOrientationChanged(Qt::ScreenOrientation orientation);
    // Clears window surface on the compositor thread. Can be called even when there are
    // no active views. In case this function is called too early during gecko initialization,
    // before compositor thread has actually been started the function returns false.
    bool postClearWindowSurfaceTask();

private:
    void setWebPage(DeclarativeWebPage *webPage, bool triggerSignals = false);
    void setTabModel(DeclarativeTabModel *model);
    qreal contentHeight() const;
    bool canInitialize() const;
    void loadTab(const Tab& tab, bool force);
    void updateMode();
    void setActiveTabRendered(bool rendered);
    bool browserEnabled() const;

    void destroyWindow();
    static void clearWindowSurfaceTask(void* data);
    void clearWindowSurface();

    QPointer<QMozWindow> m_mozWindow;
    QPointer<QQuickItem> m_rotationHandler;
    QPointer<DeclarativeWebPage> m_webPage;
    QPointer<QQuickView> m_chromeWindow;
    QOpenGLContext *m_context;
    QMutex m_contextMutex;

    QPointer<DeclarativeTabModel> m_model;
    QPointer<QQmlComponent> m_webPageComponent;
    QPointer<WebPages> m_webPages;
    QPointer<DeclarativeTabModel> m_persistentTabModel;
    QPointer<DeclarativeTabModel> m_privateTabModel;

    bool m_enabled;
    bool m_foreground;
    bool m_allowHiding;
    bool m_touchBlocked;
    bool m_selectionActive;
    bool m_portrait;
    bool m_fullScreenMode;
    qreal m_fullScreenHeight;
    bool m_imOpened;
    qreal m_toolbarHeight;

    QString m_favicon;

    // See DeclarativeWebContainer::load (line 283) as load need to "work" even if engine, model,
    // or qml component is not yet completed (completed property is still false). So cache url/title for later use.
    // Problem is visible with a download url as it does not trigger urlChange for the loaded page (correct behavior).
    // Once downloading has been started and if we have existing tabs we reset
    // back to the active tab and load it. In case we did not have tabs open when downloading was
    // triggered we just clear these.
    QString m_initialUrl;

    bool m_loading;
    int m_loadProgress;

    bool m_completed;
    bool m_initialized;

    bool m_privateMode;
    bool m_activeTabRendered;

    QMutex m_clearSurfaceTaskMutex;
    QMozContext::TaskHandle m_clearSurfaceTask;

    bool m_closing;

    QHash<int, uint> m_tabOwners;
    DeclarativeHistoryModel *m_historyModel;

    CloseEventFilter *m_closeEventFilter;

    friend class tst_webview;
    friend class tst_declarativewebcontainer;
};

QML_DECLARE_TYPE(DeclarativeWebContainer)

#endif // DECLARATIVEWEBCONTAINER_H
