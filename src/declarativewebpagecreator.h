/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVEWEBVIEWCREATOR_H
#define DECLARATIVEWEBVIEWCREATOR_H

#include <QPointer>
#include <qmozviewcreator.h>

class QQuickItem;
class QuickMozView;

class DeclarativeWebViewCreator : public QMozViewCreator {
    Q_OBJECT

    Q_PROPERTY(QuickMozView *activeWebView READ activeWebView WRITE setActiveWebView NOTIFY activeWebViewChanged FINAL)

public:
    DeclarativeWebViewCreator(QObject *parent = 0);
    ~DeclarativeWebViewCreator();

    QuickMozView *activeWebView() const;
    void setActiveWebView(QuickMozView *activeWebView);

    virtual quint32 createView(const QString &url, const quint32 &parentId);

signals:
    void activeWebViewChanged();
    void newWindowRequested(const QString &url, const quint32 &parentId);

private:
    QPointer<QuickMozView> m_activeWebView;
};

#endif // DECLARATIVEWEBVIEWCREATOR_H
