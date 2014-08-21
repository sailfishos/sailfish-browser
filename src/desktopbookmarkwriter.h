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

#include <QQuickItem>
#include <QNetworkAccessManager>
#include <QUrl>

class DesktopBookmarkWriter : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QString title MEMBER m_title NOTIFY titleChanged FINAL)
    Q_PROPERTY(QString link MEMBER m_link NOTIFY linkChanged FINAL)
    Q_PROPERTY(QString icon MEMBER m_icon NOTIFY iconChanged FINAL)
    Q_PROPERTY(qreal minimumIconSize MEMBER m_minimumIconSize NOTIFY minimumIconSizeChanged)
    Q_PROPERTY(bool allowCapture MEMBER m_allowCapture NOTIFY allowCaptureChanged)
    Q_PROPERTY(int captureSize MEMBER m_captureSize NOTIFY captureSizeChanged)

public:
    explicit DesktopBookmarkWriter(QQuickItem *parent = 0);

    static void setTestModeEnabled(bool testMode);
    static bool isTestModeEnabled();

signals:
    void titleChanged();
    void linkChanged();
    void iconChanged();
    void minimumIconSizeChanged();
    void allowCaptureChanged();
    void captureSizeChanged();

    void saved(QString desktopFile);
    void iconFetched(const QString &data);

public slots:
    bool save();
    void fetchIcon(const QString &iconUrl);
    bool exists(const QString &file);

private slots:
    void iconReady();
    void writeWhenReadyIcon();
    void setIconData(const QByteArray &data, QImage &image);

private:
    void fetchIcon(const QString &iconUrl, bool allowWrite);
    void readIconData(QNetworkReply *reply, bool allowWrite);
    QString defaultIcon();
    QString uniqueDesktopFileName(QString title);
    void write();
    void clear();

    QString m_title;
    QString m_link;
    QString m_icon;
    QString m_iconData;
    qreal m_minimumIconSize;
    bool m_allowCapture;
    int m_captureSize;

    QNetworkAccessManager m_networkAccessManager;

    friend class tst_desktopbookmarkwriter;
};

#endif // DESKTOPBOOKMARKWRITER_H
