/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#ifndef DECLARATIVEWEBUTILS_H
#define DECLARATIVEWEBUTILS_H

#include <QObject>
#include <QUrl>
#include <QDeclarativeView>
#include "browserservice.h"

class DeclarativeWebUtils : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString initialPage READ initialPage CONSTANT FINAL)
    Q_PROPERTY(QString homePage READ homePage NOTIFY homePageChanged FINAL)

public:
    explicit DeclarativeWebUtils(QStringList arguments, BrowserService *service, QDeclarativeView *view, QObject *parent = 0);

    Q_INVOKABLE QUrl getFaviconForUrl(QUrl url);
    // TODO: get rid of this method: declarative QML code shouldn't touch Qt event loops.
    Q_INVOKABLE void processEvents();

public slots:
    void updateWebEngineSettings();   
    void openUrl(QString url);
    QString homePage();
    QString initialPage();

signals:
    void homePageChanged();
    void openUrlRequested(QString url);

private:
    QString m_homePage;
    QStringList m_arguments;
    BrowserService *m_service;
};
#endif // DECLARATIVEWEBUTILS_H
