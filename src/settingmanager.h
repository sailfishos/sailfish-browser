/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

#ifndef SETTINGMANAGER_H
#define SETTINGMANAGER_H

#include <MGConfItem>

class SettingManager : public QObject
{
    Q_OBJECT

public:
    explicit SettingManager(QObject *parent = 0);

public slots:
    void initialize();

private slots:
    void clearPrivateData();
    void setSearchEngine();

private:
    MGConfItem *m_clearPrivateDataConfItem;
    MGConfItem *m_searchEngineConfItem;
};

#endif
