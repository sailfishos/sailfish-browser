/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "downloadmimetypehandler.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#define APTOIDE_MYAPP "application/vnd.cm.aptoide.pt myapp"

void DownloadMimetypeHandler::update()
{
    QString mimeTypes = QString("%1/.mime.types").arg(QDir::homePath());
    if (!QFileInfo(mimeTypes).exists()) {
        appendDefaults(mimeTypes);
    } else if (hasDefaults(mimeTypes) == 0) {
        appendDefaults(mimeTypes);
    }
}

void DownloadMimetypeHandler::appendDefaults(const QString &targetFile)
{
    QFile file(targetFile);
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Can't create file " << targetFile;
        return;
    }

    QTextStream out(&file);
    out << APTOIDE_MYAPP;
    endl(out);
    file.close();
}

int DownloadMimetypeHandler::hasDefaults(const QString &targetFile)
{
    QFile file(targetFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return -1;
    }

    QTextStream in(&file);

    // Line can be commeted out and have a space as each line
    // will be simplified.
    QRegExp regExp(QString("^#?\\s?%1").arg(APTOIDE_MYAPP));

    // With Qt5.5 change to use QTextStream::readlineInto
    bool mimeTypeDefined = false;

    while (!mimeTypeDefined && !in.atEnd()) {
        // Remove spaces in between
        QString line = in.readLine().simplified();

        if (line.contains(regExp)) {
            mimeTypeDefined = true;
        }
    }
    return mimeTypeDefined ? 1 : 0;
}
