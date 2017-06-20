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
#include <QFutureWatcher>
#include <QPointer>
#include <QRgb>
#include <qopenglwebpage.h>
#include <qmozgrabresult.h>

#include "tab.h"

class DeclarativeWebContainer;
class Link;

class DeclarativeWebPage : public QOpenGLWebPage {
    Q_OBJECT
    Q_PROPERTY(DeclarativeWebContainer* container READ container NOTIFY containerChanged FINAL)
    Q_PROPERTY(int tabId READ tabId NOTIFY tabIdChanged FINAL)
    Q_PROPERTY(bool userHasDraggedWhileLoading MEMBER m_userHasDraggedWhileLoading NOTIFY userHasDraggedWhileLoadingChanged FINAL)
    Q_PROPERTY(bool fullscreen READ fullscreen NOTIFY fullscreenChanged FINAL)
    Q_PROPERTY(bool forcedChrome READ forcedChrome NOTIFY forcedChromeChanged FINAL)
    Q_PROPERTY(bool domContentLoaded READ domContentLoaded NOTIFY domContentLoadedChanged FINAL)
    Q_PROPERTY(QString favicon MEMBER m_favicon NOTIFY faviconChanged FINAL)
    Q_PROPERTY(QVariant resurrectedContentRect READ resurrectedContentRect WRITE setResurrectedContentRect NOTIFY resurrectedContentRectChanged)

    Q_PROPERTY(qreal fullscreenHeight MEMBER m_fullScreenHeight NOTIFY fullscreenHeightChanged FINAL)
    Q_PROPERTY(qreal toolbarHeight READ toolbarHeight WRITE setToolbarHeight NOTIFY toolbarHeightChanged FINAL)
    Q_PROPERTY(qreal virtualKeyboardMargin WRITE setVirtualKeyboardMargin READ virtualKeyboardMargin NOTIFY virtualKeyboardMarginChanged FINAL)

public:
    DeclarativeWebPage(QObject *parent = 0);
    ~DeclarativeWebPage();

    DeclarativeWebContainer* container() const;
    void setContainer(DeclarativeWebContainer *container);

    int tabId() const;
    void setInitialTab(const Tab& tab);

    QVariant resurrectedContentRect() const;
    void setResurrectedContentRect(QVariant resurrectedContentRect);

    qreal toolbarHeight() const;
    void setToolbarHeight(qreal);

    qreal virtualKeyboardMargin() const;
    void setVirtualKeyboardMargin(qreal);

    bool fullscreen() const;
    bool forcedChrome() const;
    bool domContentLoaded() const;

    bool initialLoadHasHappened() const;
    void setInitialLoadHasHappened();

    void timerEvent(QTimerEvent *);

    Q_INVOKABLE void loadTab(QString newUrl, bool force);
    Q_INVOKABLE void grabToFile(const QSize& size);
    Q_INVOKABLE void grabThumbnail(const QSize& size);
    Q_INVOKABLE void forceChrome(bool forcedChrome);

signals:
    void contentOrientationChanged(Qt::ScreenOrientation orientation);
    void containerChanged();
    void tabIdChanged();
    void userHasDraggedWhileLoadingChanged();
    void fullscreenChanged();
    void forcedChromeChanged();
    void domContentLoadedChanged();
    void faviconChanged();
    void resurrectedContentRectChanged();
    void grabResult(QString fileName);
    void thumbnailResult(QString data);

    void fullscreenHeightChanged();
    void toolbarHeightChanged();
    void virtualKeyboardMarginChanged();

private slots:
    void setFullscreen(const bool fullscreen);
    void onRecvAsyncMessage(const QString& message, const QVariant& data);
    void onTabHistoryAvailable(const int& historyTabId, const QList<Link>& links, int currentLinkId);
    void onUrlChanged();
    void grabResultReady();
    void grabWritten();
    void thumbnailReady();
    void updateViewMargins();

    // These are here to inform embedlite-components that keyboard is open or close
    // matching composition metrics.
    void sendVkbOpenCompositionMetrics();

private:
    QString saveToFile(QImage image);
    void restoreHistory();
    void setContentLoaded();

    QPointer<DeclarativeWebContainer> m_container;
    // Tab data fetched upon web page initialization. It never changes afterwards.
    Tab m_initialTab;
    bool m_userHasDraggedWhileLoading;
    bool m_fullscreen;
    bool m_forcedChrome;
    bool m_domContentLoaded;
    bool m_initialLoadHasHappened;
    bool m_tabHistoryReady;
    bool m_urlReady;
    QString m_favicon;
    QVariant m_resurrectedContentRect;
    QSharedPointer<QMozGrabResult> m_grabResult;
    QSharedPointer<QMozGrabResult> m_thumbnailResult;
    QFutureWatcher<QString> m_grabWritter;
    QList<Link> m_restoredTabHistory;
    int m_restoredCurrentLinkId;

    qreal m_fullScreenHeight;
    qreal m_toolbarHeight;
    qreal m_virtualKeyboardMargin;

    int m_marginChangeThrottleTimer;
};

QDebug operator<<(QDebug, const DeclarativeWebPage *);

QML_DECLARE_TYPE(DeclarativeWebPage)

#endif
