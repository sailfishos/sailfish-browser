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

class DeclarativeParameters : public QObject
{
    Q_OBJECT

     Q_PROPERTY(QString homePage READ homePage NOTIFY homePageChanged FINAL)

public:
    explicit DeclarativeParameters(QStringList arguments, QDeclarativeView* view, QObject *parent = 0);

    QString homePage();

    Q_INVOKABLE QString initialPage();

signals:
    void homePageChanged();
    
private:
    QString m_homePage;
    QStringList m_arguments;  
};
#endif // DECLARATIVEPARAMETERS_H
