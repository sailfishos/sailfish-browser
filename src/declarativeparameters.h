/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/
#ifndef DECLARATIVEPARAMETERS_H
#define DECLARATIVEPARAMETERS_H

#include <QObject>
#include <QStringList>
#include <QDeclarativeView>
#include "browserservice.h"

class DeclarativeParameters : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString initialPage READ initialPage CONSTANT FINAL)
    Q_PROPERTY(QString homePage READ homePage NOTIFY homePageChanged FINAL)

public:
    explicit DeclarativeParameters(QStringList arguments, BrowserService *service, QDeclarativeView *view, QObject *parent = 0);

    QString homePage();
    QString initialPage();

public slots:
    void openUrl(QString url);

signals:
    void homePageChanged();  
    void openUrlRequested(QString url);
    
private:
    QString m_homePage;
    QStringList m_arguments;  
    BrowserService *m_service;
};
#endif // DECLARATIVEPARAMETERS_H
