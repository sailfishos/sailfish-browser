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

DeclarativeWebUtils::DeclarativeWebUtils(QStringList arguments,
                                         BrowserService *service,
                                         QDeclarativeView* view,
                                         QObject *parent) :
    QObject(parent),
    m_homePage("file:///usr/share/sailfish-browser/pages/demo.html"),
    m_arguments(arguments),
    m_service(service)
{
    connect(QMozContext::GetInstance(), SIGNAL(onInitialized()),
            this, SLOT(updateWebEngineSettings()));

    connect(service, SIGNAL(openUrlRequested(QString)),
            this, SLOT(openUrl(QString)));
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

void DeclarativeWebUtils::openUrl(QString url)
{
    m_arguments << url;

    emit openUrlRequested(url);
}

QString DeclarativeWebUtils::initialPage()
{
    if (m_arguments.count() > 1) {
        return m_arguments.last();
    } else {
        return "";
    }
}

QString DeclarativeWebUtils::homePage()
{
    return m_homePage;
}
