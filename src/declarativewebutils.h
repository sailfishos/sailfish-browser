/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVEWEBUTILS_H
#define DECLARATIVEWEBUTILS_H

#include <QObject>
#include <QUrl>
#include <QColor>
#include <QVariant>
#include "browserservice.h"
#include <QProcess>

class DeclarativeWebUtils : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString homePage READ homePage NOTIFY homePageChanged FINAL)
    Q_PROPERTY(QString downloadDir READ downloadDir CONSTANT FINAL)
    Q_PROPERTY(QString picturesDir READ picturesDir CONSTANT FINAL)
    Q_PROPERTY(bool firstUseDone READ firstUseDone WRITE setFirstUseDone NOTIFY firstUseDoneChanged)

public:
    explicit DeclarativeWebUtils(BrowserService *service, QObject *parent = 0);

    QString downloadDir() const;
    QString picturesDir() const;
    bool firstUseDone() const;
    void setFirstUseDone(bool firstUseDone);

    Q_INVOKABLE QUrl getFaviconForUrl(QUrl url);
    Q_INVOKABLE int getLightness(QColor color) const;
    Q_INVOKABLE bool fileExists(QString fileName) const;
    Q_INVOKABLE QString displayableUrl(QString fullUrl) const;

public slots:
    QString homePage();
    void clearStartupCacheIfNeeded();

signals:
    void homePageChanged();
    void openUrlRequested(QString url, bool launch = false);
    void firstUseDoneChanged();

private slots:
    void updateWebEngineSettings();
    void handleObserve(const QString message, const QVariant data);

private:
    QString m_homePage;
    BrowserService *m_service;
    bool m_firstUseDone;
};
#endif // DECLARATIVEWEBUTILS_H
