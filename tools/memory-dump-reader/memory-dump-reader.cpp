/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtGui/QGuiApplication>
#include <QFile>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QQmlContext>
#include <QDebug>
#include "qtquick2applicationviewer.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QString fileName;
    if (app.arguments().length() > 1) {
        fileName = app.arguments().at(1);
    } else {
        qWarning() << "Pass extracted json memory info dump as argument";
        return 0;
    }

    QtQuick2ApplicationViewer viewer;

    QFile jsonFile(fileName);
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file:" << fileName;
        return 0;
    }

    viewer.setTitle(fileName);
    viewer.engine()->rootContext()->setContextProperty("jsonFile", QString(jsonFile.readAll()));
    viewer.setMainQmlFile(QStringLiteral("qml/memory-dump-reader.qml"));
    viewer.showExpanded();

    return app.exec();
}
