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
#include <MGConfItem>

class DeclarativeWebUtils : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString homePage READ homePage NOTIFY homePageChanged FINAL)
    Q_PROPERTY(bool firstUseDone READ firstUseDone WRITE setFirstUseDone NOTIFY firstUseDoneChanged)
    Q_PROPERTY(qreal cssPixelRatio READ cssPixelRatio NOTIFY cssPixelRatioChanged)

public:
    static DeclarativeWebUtils *instance();

    bool firstUseDone() const;
    void setFirstUseDone(bool firstUseDone);
    qreal cssPixelRatio() const;

    Q_INVOKABLE int getLightness(QColor color) const;
    Q_INVOKABLE QString createUniqueFileUrl(const QString &fileName, const QString &path) const;
    Q_INVOKABLE QString displayableUrl(QString fullUrl) const;

public slots:
    QString homePage() const;
    void clearStartupCacheIfNeeded();
    void handleDumpMemoryInfoRequest(QString fileName);

signals:
    void homePageChanged();
    void openUrlRequested(QString url);
    void activateNewTabViewRequested();
    void firstUseDoneChanged();
    void dumpMemoryInfo(QString fileName);
    void cssPixelRatioChanged();
    void touchTopRadiusChanged();
    void touchBottomRadiusChanged();

private slots:
    void updateWebEngineSettings();
    void handleObserve(const QString message, const QVariant data);

private:
    explicit DeclarativeWebUtils();
    ~DeclarativeWebUtils();
    void setRenderingPreferences();

    MGConfItem m_homePage;
    bool m_firstUseDone;
};
#endif // DECLARATIVEWEBUTILS_H
