/****************************************************************************
**
** Copyright (c) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVEWEBPAGECREATOR_H
#define DECLARATIVEWEBPAGECREATOR_H

#include <QPointer>
#include <qmozviewcreator.h>

class DeclarativeWebPage;

class DeclarativeWebPageCreator : public QMozViewCreator {
    Q_OBJECT

    Q_PROPERTY(DeclarativeWebPage *activeWebPage READ activeWebPage WRITE setActiveWebPage NOTIFY activeWebPageChanged FINAL)

public:
    DeclarativeWebPageCreator(QObject *parent = 0);
    ~DeclarativeWebPageCreator();

    DeclarativeWebPage *activeWebPage() const;
    void setActiveWebPage(DeclarativeWebPage *activeWebPage);

    virtual quint32 createView(const quint32 &parentId);

signals:
    void activeWebPageChanged();
    void newWindowRequested(const quint32 &parentId);

private:
    QPointer<DeclarativeWebPage> m_activeWebPage;
};

#endif // DECLARATIVEWEBPAGECREATOR_H
