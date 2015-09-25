/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WEBPAGEFACTORY_H
#define WEBPAGEFACTORY_H

#include <QPointer>
#include <QObject>

class DeclarativeWebPage;
class DeclarativeWebContainer;
class Tab;
class QQmlComponent;

class WebPageFactory : public QObject
{
    Q_OBJECT
public:
    WebPageFactory(QObject *parent = 0) : QObject(parent) {};

    DeclarativeWebPage* createWebPage(DeclarativeWebContainer *webContainer,
                                      const Tab &initialTab,
                                      int parentId);
public slots:
    void updateQmlComponent(QQmlComponent *newComponent);

private:
    QPointer<QQmlComponent> m_qmlComponent;
};

#endif
