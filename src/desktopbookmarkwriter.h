/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DESKTOPBOOKMARKWRITER_H
#define DESKTOPBOOKMARKWRITER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QUrl>

class DesktopBookmarkWriter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title MEMBER m_title NOTIFY titleChanged FINAL)
    Q_PROPERTY(QString link MEMBER m_link NOTIFY linkChanged FINAL)
    Q_PROPERTY(QString icon MEMBER m_icon NOTIFY iconChanged FINAL)
    Q_PROPERTY(qreal minimumIconSize MEMBER m_minimumIconSize NOTIFY minimumIconSizeChanged)

public:
    explicit DesktopBookmarkWriter(QObject *parent = 0);

    static void setTestModeEnabled(bool testMode);
    static bool isTestModeEnabled();

signals:
    void titleChanged();
    void linkChanged();
    void iconChanged();
    void minimumIconSizeChanged();
    void saved(QString desktopFile);

public slots:
    bool save();
    bool exists(const QString &file);

private slots:
    void iconReady();
    void setIconData(const QByteArray &data);

private:
    QString defaultIcon();
    QString uniqueDesktopFileName(QString title);
    void write();
    void clear();

    QString m_title;
    QString m_link;
    QString m_icon;
    QString m_iconData;
    qreal m_minimumIconSize;

    QNetworkAccessManager m_networkAccessManager;

    friend class tst_desktopbookmarkwriter;
};

#endif // DESKTOPBOOKMARKWRITER_H
