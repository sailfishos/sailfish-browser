/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVETAB_H
#define DECLARATIVETAB_H

#include <QObject>
#include <QString>
#include <QDebug>
#include <qqml.h>

#include "tab.h"
#include "link.h"

class DeclarativeTab : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool valid READ valid NOTIFY validChanged FINAL)
    Q_PROPERTY(QString thumbnailPath READ thumbnailPath NOTIFY thumbPathChanged FINAL)
    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged FINAL)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)

public:
    DeclarativeTab(QObject *parent = 0);
    ~DeclarativeTab();

    QString thumbnailPath() const;
    void setThumbnailPath(QString thumbnailPath);

    QString url() const;
    void setUrl(QString url);

    QString title() const;
    void setTitle(QString title);

    int tabId() const;

    bool valid() const;
    void setInvalid();
    void invalidateTabData();

    Tab tabData() const;
    int linkId() const;

signals:
    void thumbPathChanged(QString path, int tabId);
    void urlChanged();
    void validChanged();
    void titleChanged();

private:
    void updateTabData(const Tab &tab);
    void activatePreviousLink();
    void activateNextLink();

    Tab m_tab;
    Link m_link;
    friend class DeclarativeWebContainer;
    friend class tst_declarativetab;
};

QDebug operator<<(QDebug, const DeclarativeTab *);

QML_DECLARE_TYPE(DeclarativeTab)

#endif // DECLARATIVETAB_H
