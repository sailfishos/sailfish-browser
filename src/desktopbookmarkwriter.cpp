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

#include <QBuffer>
#include <QDir>
#include <QFileInfo>
#include <QNetworkReply>
#include <QImage>
#include <QStandardPaths>
#include <QUrl>
#include <QQuickWindow>

#define BASE64_IMAGE_PREFIX "data:image/png;base64,"
#define BASE64_IMAGE "data:image/png;base64,%1"

static bool dbw_testMode = false;

DesktopBookmarkWriter::DesktopBookmarkWriter(QQuickItem *parent)
    : QQuickItem(parent)
    , m_minimumIconSize(64) // Initial value that matches theme iconSizeMedium.
    , m_allowCapture(false)
    , m_captureSize(-1)
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

    fetchIcon(m_icon, true);
    return true;
}

void DesktopBookmarkWriter::fetchIcon(const QString &iconUrl)
{
    fetchIcon(iconUrl, false);
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
    readIconData(reply, false);
}

void DesktopBookmarkWriter::writeWhenReadyIcon()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    readIconData(reply, true);
}

void DesktopBookmarkWriter::setIconData(const QByteArray &data, QImage &image)
{
    if (image.width() < m_minimumIconSize || image.height() < m_minimumIconSize) {
        m_iconData = defaultIcon();
    } else {
        m_iconData = QString(BASE64_IMAGE).arg(QString(data.toBase64()));
    }

    emit iconFetched(m_iconData);
}

void DesktopBookmarkWriter::fetchIcon(const QString &iconUrl, bool allowWrite)
{
    if (!iconUrl.isEmpty()){
        QUrl url(iconUrl);
        QString path = url.path();
        if (path.endsWith(".ico")) {
            m_iconData = defaultIcon();
            emit iconFetched(m_iconData);
            if (allowWrite) {
                write();
            } else {
                clear();
            }
        } else {
            QNetworkRequest request(url);
            QNetworkReply *reply = m_networkAccessManager.get(request);
            if (allowWrite) {
                connect(reply, SIGNAL(finished()), this, SLOT(writeWhenReadyIcon()));
            } else {
                connect(reply, SIGNAL(finished()), this, SLOT(iconReady()));
            }
        }
    } else {
        QImage image;
        QByteArray iconData;
        if (m_allowCapture && window() && window()->isActive()) {
            // Avoid I/O.
            image = window()->grabWindow();
            qreal rotation = parentItem() ? parentItem()->rotation() : 0;
            QRect cropBounds(0, 0, m_captureSize, m_captureSize);
            QTransform transform;
            transform.rotate(360 - rotation);
            image = image.transformed(transform);
            image = image.copy(cropBounds);
            image = image.scaledToWidth(m_minimumIconSize * 2);

            QBuffer buffer(&iconData);
            buffer.open(QIODevice::WriteOnly);
            image.save(&buffer, "PNG");
            buffer.close();
        }

        setIconData(iconData, image);
        if (allowWrite) {
            write();
        } else {
            clear();
        }
    }
}

void DesktopBookmarkWriter::readIconData(QNetworkReply *reply, bool allowWrite)
{
    if (reply) {
        QByteArray iconData = reply->readAll();
        QImage image;
        image.loadFromData(iconData);
        setIconData(iconData, image);
        reply->deleteLater();
        if (allowWrite) {
            write();
        } else {
            clear();
        }
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
