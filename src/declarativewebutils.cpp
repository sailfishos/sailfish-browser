/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#include <QLocale>
#include <QStringList>
#include <QVariant>
#include "declarativewebutils.h"
#include "qmozcontext.h"

DeclarativeWebUtils::DeclarativeWebUtils(QObject *parent) :
    QObject(parent)
{
    connect(QMozContext::GetInstance(), SIGNAL(onInitialized()),
            this, SLOT(updateWebEngineSettings()));
}

QUrl DeclarativeWebUtils::getFaviconForUrl(QUrl url)
{
    QUrl faviconUrl(url);
    faviconUrl.setPath("/favicon.ico");
    return faviconUrl;
}

void DeclarativeWebUtils::updateWebEngineSettings()
{
    // Infer and set Accept-Language header from the current system locale
    QString langs;
    QStringList locale = QLocale::system().name().split("_", QString::SkipEmptyParts);
    if (locale.size() > 1) {
        langs = QString("%1-%2,%3").arg(locale.at(0)).arg(locale.at(1)).arg(locale.at(0));
    } else {
        langs = locale.at(0);
    }
    QMozContext::GetInstance()->setPref(QString("intl.accept_languages"), QVariant(langs));
}
