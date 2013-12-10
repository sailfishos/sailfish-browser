/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

#ifndef BROWSERSETTINGS_H
#define BROWSERSETTINGS_H

#include <QObject>
#include <QString>

class BrowserSettings: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString testProp READ testProp CONSTANT)

public:
    explicit BrowserSettings(QObject *parent = 0);
    virtual ~BrowserSettings();

    const QString testProp() const;
};

#endif
