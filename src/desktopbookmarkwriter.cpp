/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "desktopbookmarkwriter.h"

#include <QDir>
#include <QFileInfo>
#include <QNetworkReply>
#include <QImage>
#include <QStandardPaths>
#include <QUrl>

#define BASE64_IMAGE_PREFIX "data:image/png;base64,"
#define BASE64_IMAGE "data:image/png;base64,%1"

static bool dbw_testMode = false;

DesktopBookmarkWriter::DesktopBookmarkWriter(QObject *parent)
    : QObject(parent)
    , m_minimumIconSize(64) // Initial value that matches theme iconSizeMedium.
{
}

void DesktopBookmarkWriter::setTestModeEnabled(bool testMode)
{
    dbw_testMode = testMode;
}

bool DesktopBookmarkWriter::isTestModeEnabled()
{
    return dbw_testMode;
}

bool DesktopBookmarkWriter::save()
{
    if (m_link.trimmed().isEmpty() || m_title.trimmed().isEmpty()) {
        return false;
    }

    QString icon = m_icon;
    if (icon.startsWith("/")) {
        icon = "file://" + icon;
    }

    if (m_icon.isEmpty()) {
        m_iconData = defaultIcon();
        write();
    } else if (!QUrl(icon).isLocalFile()){
        QUrl url(icon);
        QString path = url.path();
        if (path.endsWith(".ico")) {
            m_iconData = defaultIcon();
            write();
        } else {
            QNetworkRequest request(url);
            QNetworkReply *reply = m_networkAccessManager.get(request);
            connect(reply, SIGNAL(finished()), this, SLOT(iconReady()));
        }
    } else {
        QFile localIcon(QUrl(icon).toLocalFile());
        QByteArray iconData;
        if (localIcon.open(QFile::ReadOnly)) {
            iconData = localIcon.readAll();
        }
        setIconData(iconData);
        write();
    }
    return true;
}


bool DesktopBookmarkWriter::exists(const QString &file)
{
    return QFileInfo(file).exists();
}

void DesktopBookmarkWriter::clear()
{
    if (!m_icon.isEmpty()) {
        m_icon = "";
        emit iconChanged();
    }

    if (!m_link.isEmpty()) {
        m_link = "";
        emit linkChanged();
    }

    if (!m_title.isEmpty()) {
        m_title = "";
        emit titleChanged();
    }

    if (!m_iconData.isEmpty()) {
        m_iconData = "";
    }
}

void DesktopBookmarkWriter::iconReady()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        QByteArray iconData = reply->readAll();
        setIconData(iconData);
        reply->deleteLater();
        write();
    }
}

void DesktopBookmarkWriter::setIconData(const QByteArray &data)
{
    QImage image;
    image.loadFromData(data);
    if (image.width() < m_minimumIconSize || image.height() < m_minimumIconSize) {
        m_iconData = defaultIcon();
    } else {
        m_iconData = QString(BASE64_IMAGE).arg(QString(data.toBase64()));
    }
}

QString DesktopBookmarkWriter::defaultIcon()
{
    return QString(DEFAULT_DESKTOP_BOOKMARK_ICON);
}

QString DesktopBookmarkWriter::uniqueDesktopFileName(QString title)
{
    QString filePath;
    if (!isTestModeEnabled()) {
        filePath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    } else {
        filePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    }
    title = title.simplified().replace(QString(" "), QString("-"));

    QDir dir(filePath);
    dir.setNameFilters(QStringList() << QString("sailfish-browser-%2*").arg(title));
    QStringList similarlyNamedFiles = dir.entryList();
    int count = similarlyNamedFiles.count();

    QString fileName = QString(DESKTOP_FILE).arg(title).arg(count);
    while (similarlyNamedFiles.contains(fileName)) {
        ++count;
        fileName = QString(DESKTOP_FILE).arg(title).arg(count);
    }

    return QString(DESKTOP_FILE_PATTERN).arg(filePath, title).arg(count);
}

void DesktopBookmarkWriter::write()
{
    QString fileName = uniqueDesktopFileName(m_title);
    QString desktopFileData = QString("[Desktop Entry]\n" \
                                      "Type=Link\n" \
                                      "Name=%1\n" \
                                      "Icon=%2\n" \
                                      "URL=%3\n" \
                                      "Comment=%4\n").arg(m_title.trimmed(), m_iconData,
                                                          m_link.trimmed(), m_title.trimmed());
    QFile desktopFile(fileName);
    if (desktopFile.open(QFile::WriteOnly)) {
        desktopFile.write(desktopFileData.toUtf8());
        desktopFile.flush();
        desktopFile.close();
        clear();
        emit saved(fileName);
    }
}
