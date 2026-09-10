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

#include <QQuickItem>
#include <QQuickView>
#include <QPointer>
#include <qmozsecurity.h>

class DeclarativeTabModel;
class DeclarativeHistoryModel;
class CloseEventFilter;

// Application state and D-Bus controller, with an optional native content
// window below the translucent QML controls window.
class DeclarativeWebContainer : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QWindow *nativeWindow READ nativeWindow CONSTANT)
    Q_PROPERTY(QQuickItem *rotationHandler MEMBER m_rotationHandler NOTIFY rotationHandlerChanged FINAL)
    Q_PROPERTY(DeclarativeTabModel *tabModel READ tabModel NOTIFY tabModelChanged FINAL)
    Q_PROPERTY(DeclarativeTabModel *persistentTabModel READ persistentTabModel CONSTANT)
    Q_PROPERTY(DeclarativeTabModel *privateTabModel READ privateTabModel CONSTANT)
    Q_PROPERTY(bool completed READ completed NOTIFY completedChanged FINAL)
    Q_PROPERTY(bool foreground READ foreground WRITE setForeground NOTIFY foregroundChanged FINAL)
    Q_PROPERTY(bool touchBlocked MEMBER m_touchBlocked NOTIFY touchBlockedChanged FINAL)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged FINAL)
    Q_PROPERTY(int loadProgress READ loadProgress NOTIFY loadProgressChanged FINAL)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY canGoForwardChanged FINAL)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY canGoBackChanged FINAL)
    Q_PROPERTY(int tabId READ tabId NOTIFY tabIdChanged FINAL)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged FINAL)
    Q_PROPERTY(QString url READ url NOTIFY urlChanged FINAL)
    Q_PROPERTY(bool privateMode READ privateMode WRITE setPrivateMode NOTIFY privateModeChanged FINAL)
    Q_PROPERTY(QObject *chromeWindow READ chromeWindow WRITE setChromeWindow NOTIFY chromeWindowChanged FINAL)
    Q_PROPERTY(QMozSecurity *security READ security NOTIFY securityChanged)
    Q_PROPERTY(DeclarativeHistoryModel* historyModel READ historyModel WRITE setHistoryModel NOTIFY historyModelChanged)
    Q_PROPERTY(bool hasInitialUrl READ hasInitialUrl NOTIFY hasInitialUrlChanged)

public:
    DeclarativeWebContainer(QQuickItem *parent = 0);
    ~DeclarativeWebContainer();
    static DeclarativeWebContainer *instance();
    static bool nativePresentationEnabled();
    QWindow *nativeWindow() const;
    DeclarativeTabModel *tabModel() const;
    DeclarativeTabModel *persistentTabModel() const;
    DeclarativeTabModel *privateTabModel() const;
    bool completed() const;
    bool foreground() const;
    void setForeground(bool active);
    bool privateMode() const;
    void setPrivateMode(bool);
    bool loading() const;
    int loadProgress() const;
    bool canGoForward() const;
    bool canGoBack() const;
    QObject *chromeWindow() const;
    void setChromeWindow(QObject *chromeWindow);
    QMozSecurity *security() const;
    int tabId() const;
    QString title() const;
    QString url() const;
    bool isActiveTab(int tabId);
    uint tabOwner(int tabId) const;
    int requestTabWithOwner(int tabId, const QString &url, uint ownerPid);
    void requestTabWithOwnerAsync(int tabId, const QString &url, uint ownerPid, void *context);
    Q_INVOKABLE void releaseActiveTabOwnership();
    Q_INVOKABLE void load(const QString &url, bool force = false, bool fromExternal = false);
    Q_INVOKABLE void reload(bool force = true);
    Q_INVOKABLE void goForward();
    Q_INVOKABLE void goBack();
    Q_INVOKABLE int activateTab(int tabId, const QString &url);
    Q_INVOKABLE void closeTab(int tabId);
    Q_INVOKABLE void updateHostedState(const QString &url, const QString &title,
                                       bool loading, int loadProgress,
                                       bool canGoBack, bool canGoForward,
                                       QMozSecurity *security,
                                       bool notifySecurity);
    Q_INVOKABLE void clearHostedState();
    Q_INVOKABLE void reportWindowOrientation(Qt::ScreenOrientation orientation);
    DeclarativeHistoryModel *historyModel() const;
    void setHistoryModel(DeclarativeHistoryModel *model);
    bool hasInitialUrl() const;

signals:
    void rotationHandlerChanged();
    void tabModelChanged();
    void completedChanged();
    void foregroundChanged();
    void touchBlockedChanged();
    void loadingChanged();
    void loadProgressChanged();
    void canGoForwardChanged();
    void canGoBackChanged();
    void tabIdChanged();
    void titleChanged();
    void urlChanged();
    void privateModeChanged();
    void chromeWindowChanged();
    void securityChanged();
    void historyModelChanged();
    void hasInitialUrlChanged();
    void applicationClosing();
    void requestTabWithOwnerAsyncResult(int tabId, void *context);
    void hostedLoadRequested(const QString &url, bool fromExternal);
    void hostedReloadRequested();
    void hostedGoBackRequested();
    void hostedGoForwardRequested();

protected:
    void componentComplete() override;
    bool eventFilter(QObject *object, QEvent *event) override;

private slots:
    void initialize();
    void dsmeStateChange(const QString &state);

private:
    void setTabModel(DeclarativeTabModel *model);
    void updateMode();
    bool canInitialize() const;
    bool browserEnabled() const;

    QWindow *m_nativeWindow = nullptr;
    QPointer<QQuickItem> m_rotationHandler;
    QPointer<QQuickView> m_chromeWindow;
    QPointer<DeclarativeTabModel> m_model;
    QPointer<DeclarativeTabModel> m_persistentTabModel;
    QPointer<DeclarativeTabModel> m_privateTabModel;
    bool m_nativeInitialized = false;
    bool m_foreground = true;
    bool m_touchBlocked = false;
    bool m_privateMode = false;
    bool m_completed = false;
    bool m_initialized = false;
    bool m_closing = false;
    QString m_initialUrl;
    bool m_fromExternal = false;
    QString m_hostedUrl;
    QString m_hostedTitle;
    int m_hostedLoadProgress = 0;
    bool m_hostedLoading = false;
    bool m_hostedCanGoBack = false;
    bool m_hostedCanGoForward = false;
    bool m_hostedStateActive = false;
    QPointer<QMozSecurity> m_hostedSecurity;
    QHash<int, uint> m_tabOwners;
    DeclarativeHistoryModel *m_historyModel = nullptr;
    CloseEventFilter *m_closeEventFilter = nullptr;
};

QML_DECLARE_TYPE(DeclarativeWebContainer)
#endif
