/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#include "declarativeparameters.h"
#include <QDeclarativeEngine>
#include <QDeclarativeContext>

DeclarativeParameters::DeclarativeParameters(QStringList arguments, BrowserService *service, QDeclarativeView* view, QObject *parent) :
    QObject(parent),
    m_homePage("file:///usr/share/sailfish-browser/pages/demo.html"),
    m_arguments(arguments),
    m_service(service)

{
    view->engine()->rootContext()->setContextProperty("Parameters",this);
    connect(service, SIGNAL(openUrlRequested(QString)), this, SLOT(openUrl(QString)));
}


void DeclarativeParameters::openUrl(QString url)
{
    m_arguments << url;

    emit openUrlRequested(url);
}

QString DeclarativeParameters::initialPage()
{
    if (m_arguments.count() > 1) {
        return m_arguments.last();
    } else {
        return "";
    }
}

QString DeclarativeParameters::homePage()
{
    return m_homePage;
}
