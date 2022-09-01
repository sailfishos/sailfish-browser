/*
 * Copyright (c) 2015 - 2019 Jolla Ltd.
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DECLARATIVEWEBPAGE_H
#define DECLARATIVEWEBPAGE_H

#include <QObject>
#include <QDebug>
#include <QUrl>
#include <QRectF>
#include <QWindow>
#include <QTouchEvent>
#include <QVariant>
#include <QColor>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "tab.h"

class DeclarativeWebContainer;
class QMozSecurity;

class DeclarativeWebPage : public QObject
{
    Q_OBJECT

public:
    explicit DeclarativeWebPage(QObject *parent = 0) : QObject(parent) {};

    MOCK_CONST_METHOD0(contentRect, QRectF());
    MOCK_METHOD1(setWindow, void(QWindow *));
    MOCK_CONST_METHOD0(completed, bool());
    MOCK_CONST_METHOD0(uniqueId, quint32());
    MOCK_CONST_METHOD0(isPainted, bool());
    MOCK_CONST_METHOD0(loadProgress, int());
    MOCK_CONST_METHOD0(loading, bool());
    MOCK_CONST_METHOD0(canGoForward, bool());
    MOCK_CONST_METHOD0(canGoBack, bool());
    MOCK_METHOD0(reload, void());
    MOCK_METHOD0(goForward, void());
    MOCK_METHOD0(goBack, void());
    MOCK_METHOD1(setChrome, void(bool));
    MOCK_METHOD0(suspendView, void());
    MOCK_METHOD0(resumeView, void());
    MOCK_METHOD0(update, void());
    MOCK_METHOD0(initialize, void());
    MOCK_METHOD0(stop, void());
    MOCK_METHOD0(bgcolor, QColor());
    MOCK_METHOD1(touchEvent, void(QTouchEvent *));
    MOCK_CONST_METHOD1(inputMethodQuery, QVariant(Qt::InputMethodQuery));
    MOCK_METHOD1(inputMethodEvent, void(QInputMethodEvent *));
    MOCK_METHOD1(keyPressEvent, void(QKeyEvent *));
    MOCK_METHOD1(keyReleaseEvent, void(QKeyEvent *));
    MOCK_METHOD1(focusInEvent, void(QFocusEvent *));
    MOCK_METHOD1(focusOutEvent, void(QFocusEvent *));
    MOCK_METHOD1(timerEvent, void(QTimerEvent *));
    MOCK_METHOD1(updateContentOrientation, void(Qt::ScreenOrientation));
    MOCK_CONST_METHOD0(contentHeight, qreal());
    MOCK_CONST_METHOD0(resolution, float());
    MOCK_METHOD2(sendAsyncMessage, void(const QString&, const QVariant&));
    MOCK_METHOD1(setParentId, void(unsigned));
    MOCK_METHOD1(setParentBrowsingContext, void(uintptr_t));

    MOCK_METHOD1(grabThumbnail, void(const QSize&));

    MOCK_CONST_METHOD0(active, bool());
    MOCK_METHOD1(setActive, void(bool));

    MOCK_METHOD1(setContainer, void(DeclarativeWebContainer *));

    MOCK_METHOD1(setResurrectedContentRect, void(QVariant));
    MOCK_METHOD2(setInitialState, void(const Tab&, bool privateMode));

    MOCK_METHOD1(forceChrome, void(bool));
    MOCK_CONST_METHOD0(domContentLoaded, bool());

    MOCK_CONST_METHOD0(tabId, int());

    MOCK_CONST_METHOD0(url, QUrl());

    MOCK_CONST_METHOD0(desktopMode, bool());

    MOCK_CONST_METHOD0(title, QString());
    MOCK_METHOD1(setTitle, void(const QString &title));

    MOCK_CONST_METHOD0(parentId, int());

    MOCK_METHOD1(setPrivateMode, void(bool));

    MOCK_METHOD2(Q_INVOKABLE loadTab, void(const QString &newUrl, bool force));

    MOCK_CONST_METHOD0(securityState, uint());
    MOCK_CONST_METHOD0(securityStatus, QString());
    MOCK_CONST_METHOD0(security, QMozSecurity *());

signals:
    void canGoBackChanged();
    void canGoForwardChanged();
    void imeNotification(int, bool, int, int, const QString&);
    void windowCloseRequested();
    void loadingChanged();
    void loadProgressChanged();
    void requestGLContext();
    void completedChanged();

    void contentOrientationChanged(Qt::ScreenOrientation orientation);
    void containerChanged();
    void tabIdChanged();
    void urlChanged();
    void titleChanged();
    void securityChanged(QString status, uint state);
    void forcedChromeChanged();
    void fullscreenChanged();
    void domContentLoadedChanged();
    void resurrectedContentRectChanged();
    void clearGrabResult();
    void grabResult(const QString &fileName);
    void thumbnailResult(const QString &data);
};

QDebug operator<<(QDebug, const DeclarativeWebPage *);

#endif
