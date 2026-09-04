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
    explicit DeclarativeWebPage(QObject *parent = nullptr) : QObject(parent) {}

    MOCK_METHOD(QRectF, contentRect, (), (const));
    MOCK_METHOD(void, setWindow, (QWindow *));
    MOCK_METHOD(bool, completed, (), (const));
    MOCK_METHOD(quint32, uniqueId, (), (const));
    MOCK_METHOD(bool, isPainted, (), (const));
    MOCK_METHOD(int, loadProgress, (), (const));
    MOCK_METHOD(bool, loading, (), (const));
    MOCK_METHOD(bool, canGoForward, (), (const));
    MOCK_METHOD(bool, canGoBack, (), (const));
    MOCK_METHOD(void, reload, ());
    MOCK_METHOD(void, goForward, ());
    MOCK_METHOD(void, goBack, ());
    MOCK_METHOD(void, setChrome, (bool));
    MOCK_METHOD(void, suspendView, ());
    MOCK_METHOD(void, resumeView, ());
    MOCK_METHOD(void, update, ());
    MOCK_METHOD(void, initialize, ());
    MOCK_METHOD(void, stop, ());
    MOCK_METHOD(QColor, bgcolor, ());
    MOCK_METHOD(void, touchEvent, (QTouchEvent *));
    MOCK_METHOD(void, keyPressEvent, (QKeyEvent *));
    MOCK_METHOD(void, keyReleaseEvent, (QKeyEvent *));
    MOCK_METHOD(void, focusInEvent, (QFocusEvent *));
    MOCK_METHOD(void, focusOutEvent, (QFocusEvent *));
    MOCK_METHOD(void, timerEvent, (QTimerEvent *), (override));
    MOCK_METHOD(void, updateContentOrientation, (Qt::ScreenOrientation));
    MOCK_METHOD(qreal, contentHeight, (), (const));
    MOCK_METHOD(float, resolution, (), (const));
    MOCK_METHOD(void, sendAsyncMessage, (const QString&, const QVariant&));
    MOCK_METHOD(void, setParentId, (unsigned));
    MOCK_METHOD(void, setParentBrowsingContext, (uintptr_t));

    MOCK_METHOD(void, grabThumbnail, (const QSize&));

    MOCK_METHOD(bool, active, (), (const));
    MOCK_METHOD(void, setActive, (bool));

    MOCK_METHOD(void, setContainer, (DeclarativeWebContainer *));

    MOCK_METHOD(void, setResurrectedContentRect, (QVariant));
    MOCK_METHOD(void, setInitialState, (const Tab&, bool privateMode));

    MOCK_METHOD(void, forceChrome, (bool));
    MOCK_METHOD(bool, domContentLoaded, (), (const));

    MOCK_METHOD(int, tabId, (), (const));

    MOCK_METHOD(QUrl, url, (), (const));

    MOCK_METHOD(bool, desktopMode, (), (const));

    MOCK_METHOD(QString, title, (), (const));
    MOCK_METHOD(void, setTitle, (const QString &title));

    MOCK_METHOD(int, parentId, (), (const));

    MOCK_METHOD(void, setPrivateMode, (bool));

    MOCK_METHOD(void, Q_INVOKABLE loadTab, (const QString &newUrl, bool force, bool fromExternal));

    MOCK_METHOD(uint, securityState, (), (const));
    MOCK_METHOD(QString, securityStatus, (), (const));
    MOCK_METHOD(QMozSecurity*, security, (), (const));

signals:
    void canGoBackChanged();
    void canGoForwardChanged();
    void imeNotification(int, bool, int, int, const QString&);
    void windowCloseRequested();
    void loadingChanged();
    void loadProgressChanged();
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
