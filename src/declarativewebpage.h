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
    Q_PROPERTY(QString favicon MEMBER m_favicon NOTIFY faviconChanged FINAL)
    Q_PROPERTY(QVariant resurrectedContentRect READ resurrectedContentRect WRITE setResurrectedContentRect NOTIFY resurrectedContentRectChanged)

    Q_PROPERTY(qreal fullscreenHeight MEMBER m_fullScreenHeight NOTIFY fullscreenHeightChanged FINAL)
    Q_PROPERTY(qreal toolbarHeight MEMBER m_toolbarHeight NOTIFY toolbarHeightChanged FINAL)

public:
    DeclarativeWebPage(QObject *parent = 0);
    ~DeclarativeWebPage();

    DeclarativeWebContainer* container() const;
    void setContainer(DeclarativeWebContainer *container);

    int tabId() const;
    void setTab(const Tab& tab);

    QVariant resurrectedContentRect() const;
    void setResurrectedContentRect(QVariant resurrectedContentRect);

    bool fullscreen() const;
    bool forcedChrome() const;
    bool domContentLoaded() const;

    bool initialLoadHasHappened() const;
    void setInitialLoadHasHappened();

    Q_INVOKABLE void loadTab(QString newUrl, bool force);
    Q_INVOKABLE void grabToFile(const QSize& size);
    Q_INVOKABLE void grabThumbnail(const QSize& size);
    Q_INVOKABLE void forceChrome(bool forcedChrome);

public slots:
    void resetHeight(bool respectContentHeight = true);

signals:
    void containerChanged();
    void tabIdChanged();
    void userHasDraggedWhileLoadingChanged();
    void fullscreenChanged();
    void forcedChromeChanged();
    void domContentLoadedChanged();
    void faviconChanged();
    void resurrectedContentRectChanged();
    void clearGrabResult();
    void grabResult(QString fileName);
    void thumbnailResult(QString data);

    void fullscreenHeightChanged();
    void toolbarHeightChanged();

private slots:
    void setFullscreen(const bool fullscreen);
    void onRecvAsyncMessage(const QString& message, const QVariant& data);
    void onTabHistoryAvailable(const int& tabId, const QList<Link>& links);
    void onUrlChanged();
    void grabResultReady();
    void grabWritten();
    void thumbnailReady();

private:
    QString saveToFile(QImage image);
    void restoreHistory();

    QPointer<DeclarativeWebContainer> m_container;
    Tab m_tab;
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

    qreal m_fullScreenHeight;
    qreal m_toolbarHeight;
};

QDebug operator<<(QDebug, const DeclarativeWebPage *);

QML_DECLARE_TYPE(DeclarativeWebPage)

#endif
