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

#include <QQuickItem>
#include <QPointer>

class QTimerEvent;

class DeclarativeWebContainer : public QQuickItem {
    Q_OBJECT

    Q_PROPERTY(QQuickItem *webView READ webView WRITE setWebView NOTIFY webViewChanged FINAL)
    Q_PROPERTY(bool foreground READ foreground WRITE setForeground NOTIFY foregroundChanged FINAL)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(bool inputPanelVisible READ inputPanelVisible NOTIFY inputPanelVisibleChanged FINAL)
    Q_PROPERTY(qreal inputPanelHeight READ inputPanelHeight WRITE setInputPanelHeight NOTIFY inputPanelHeightChanged FINAL)
    Q_PROPERTY(qreal inputPanelOpenHeight READ inputPanelOpenHeight WRITE setInputPanelOpenHeight NOTIFY inputPanelOpenHeightChanged FINAL)
    Q_PROPERTY(qreal toolbarHeight READ toolbarHeight WRITE setToolbarHeight NOTIFY toolbarHeightChanged FINAL)
    Q_PROPERTY(bool background READ background NOTIFY backgroundChanged FINAL)

public:
    DeclarativeWebContainer(QQuickItem *parent = 0);
    ~DeclarativeWebContainer();

    QQuickItem *webView() const;
    void setWebView(QQuickItem *webView);

    bool foreground() const;
    void setForeground(bool active);

    bool background() const;

    bool active() const;
    void setActive(bool active);

    bool inputPanelVisible() const;

    qreal inputPanelHeight() const;
    void setInputPanelHeight(qreal height);

    qreal inputPanelOpenHeight() const;
    void setInputPanelOpenHeight(qreal height);

    qreal toolbarHeight() const;
    void setToolbarHeight(qreal height);

signals:
    void webViewChanged();
    void pageStackChanged();
    void foregroundChanged();
    void backgroundChanged();
    void activeChanged();
    void inputPanelVisibleChanged();
    void inputPanelHeightChanged();
    void inputPanelOpenHeightChanged();
    void toolbarHeightChanged();

public slots:
    void resetHeight(bool respectContentHeight = true);

private slots:
    void imeNotificationChanged(int state, bool open, int cause, int focusChange, const QString& type);
    void windowVisibleChanged(bool visible);
    void handleWindowChanged(QQuickWindow *window);

protected:
    void timerEvent(QTimerEvent *event);

private:
    qreal contentHeight() const;

private:
    QPointer<QQuickItem> m_webView;
    bool m_foreground;
    bool m_background;
    bool m_windowVisible;
    int m_backgroundTimer;
    bool m_active;
    bool m_inputPanelVisible;
    qreal m_inputPanelHeight;
    qreal m_inputPanelOpenHeight;
    qreal m_toolbarHeight;
};

#endif // DECLARATIVEWEBCONTAINER_H
