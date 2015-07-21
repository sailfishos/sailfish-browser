/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVEWEBPAGE_H
#define DECLARATIVEWEBPAGE_H

#include <QObject>
#include <QDebug>

class Tab;
class QUrl;
class DeclarativeWebContainer;

class DeclarativeWebPage : public QObject
{
    Q_OBJECT

public:
    explicit DeclarativeWebPage(QObject *parent = 0);

    void setContainer(DeclarativeWebContainer *);

    void setResurrectedContentRect(QVariant);
    void setInitialTab(const Tab&);

    void resetHeight(bool);
    void forceChrome(bool);

    int tabId() const;

    bool initialLoadHasHappened() const;
    void setInitialLoadHasHappened();

    virtual QUrl url() const;

    virtual QString title() const;
    void setTitle(const QString &title);

    int parentId() const;

    Q_INVOKABLE void loadTab(QString newUrl, bool force);

    int m_tabId;

signals:
    void containerChanged();
    void tabIdChanged();
    void titleChanged();
    void forcedChromeChanged();
    void fullscreenChanged();
    void domContentLoadedChanged();
    void resurrectedContentRectChanged();
    void grabResult(QString fileName);
    void thumbnailResult(QString data);

private:
    QString m_title;
};

QDebug operator<<(QDebug, const DeclarativeWebPage *);

#endif
