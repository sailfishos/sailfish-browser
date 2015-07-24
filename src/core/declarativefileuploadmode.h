/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVEFILEUPLOADMODE_H
#define DECLARATIVEFILEUPLOADMODE_H

#include <QObject>

class DeclarativeFileUploadMode : public QObject
{
    Q_OBJECT
    Q_ENUMS(UploadMode)

public:
    enum UploadMode {
        Open = 0,        // nsIFilePicker::modeOpen,
        OpenMultiple = 3 // nsIFilePicker::modeOpenMultiple
    };
};

#endif // DECLARATIVEFILEUPLOADMODE_H
