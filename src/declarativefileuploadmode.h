/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVEFILEUPLOADOPTIONS_H
#define DECLARATIVEFILEUPLOADOPTIONS_H

#include <QObject>
#include "nsIFilePicker.h"

class DeclarativeFileUploadMode : public QObject
{
    Q_OBJECT

    Q_ENUMS(UploadMode)

public:
    enum UploadMode {
        Open = nsIFilePicker::modeOpen,
        Save = nsIFilePicker::modeSave,
        GetFolder = nsIFilePicker::modeGetFolder,
        OpenMultiple = nsIFilePicker::modeOpenMultiple
    };
};

#endif // DECLARATIVEFILEUPLOADOPTIONS_H
