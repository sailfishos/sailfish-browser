/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVEFILEUPLOADFILTER_H
#define DECLARATIVEFILEUPLOADFILTER_H

#include <QObject>

class DeclarativeFileUploadFilter : public QObject
{
    Q_OBJECT
    Q_ENUMS(UploadFilter)

public:
    enum UploadFilter {
        All = 0x001,    // nsIFilePicker::filterAll,
        Image = 0x008, // nsIFilePicker::filterImages,
        Audio = 0x100,  // nsIFilePicker::filterAudio,
        Video = 0x200   // nsIFilePicker::filterVideo
    };
};
#endif // DECLARATIVEFILEUPLOADFILTER_H
