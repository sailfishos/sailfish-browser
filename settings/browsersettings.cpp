#include <QDebug>
#include "browsersettings.h"

BrowserSettings::BrowserSettings(QObject *parent)
    : QObject(parent)
{
    qDebug() << "BrowserSettings created";
}

BrowserSettings::~BrowserSettings()
{
}

const QString BrowserSettings::testProp() const
{
    return QString("test property");
}
