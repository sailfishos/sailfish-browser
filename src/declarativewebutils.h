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
#include <QColor>
#include <QVariant>
#include "browserservice.h"

class DeclarativeWebUtils : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString initialPage READ initialPage CONSTANT FINAL)
    Q_PROPERTY(QString homePage READ homePage NOTIFY homePageChanged FINAL)
    Q_PROPERTY(QString downloadDir READ downloadDir CONSTANT FINAL)
    Q_PROPERTY(QString picturesDir READ picturesDir CONSTANT FINAL)

public:
    explicit DeclarativeWebUtils(QStringList arguments, BrowserService *service, QObject *parent = 0);

    QString downloadDir() const;
    QString picturesDir() const;

    Q_INVOKABLE QUrl getFaviconForUrl(QUrl url);
    Q_INVOKABLE int getLightness(QColor color) const;
    Q_INVOKABLE bool fileExists(QString fileName) const;
    Q_INVOKABLE void deleteThumbnail(QString path) const;

public slots:
    void openUrl(QString url);
    QString homePage();
    QString initialPage();
    void clearStartupCacheIfNeeded();

signals:
    void homePageChanged();
    void openUrlRequested(QString url);

private slots:
    void updateWebEngineSettings();
    void handleObserve(const QString message, const QVariant data);

private:
    QString m_homePage;
    QStringList m_arguments;
    BrowserService *m_service;
};
#endif // DECLARATIVEWEBUTILS_H
