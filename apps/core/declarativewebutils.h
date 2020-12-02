/****************************************************************************
**
** Copyright (c) 2013 - 2018 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
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

    Q_INVOKABLE int getLightness(const QColor &color) const;
    Q_INVOKABLE QString displayableUrl(const QString &fullUrl) const;
    Q_INVOKABLE QString pageName(const QString &fullUrl) const;

public slots:
    QString homePage() const;
    void handleDumpMemoryInfoRequest(const QString &fileName);
    void openUrl(const QString &url);
    void openSettings();

signals:
    void homePageChanged();
    void openUrlRequested(const QString &url);
    void openSettingsRequested();
    void activateNewTabViewRequested();
    void showChrome();

    void firstUseDoneChanged();
    void dumpMemoryInfo(const QString &fileName);
    void cssPixelRatioChanged();
    void touchTopRadiusChanged();
    void touchBottomRadiusChanged();

private slots:
    void updateWebEngineSettings();
    void handleObserve(const QString &message, const QVariant &data);

private:
    explicit DeclarativeWebUtils();
    ~DeclarativeWebUtils();
    void setRenderingPreferences();

    MGConfItem m_homePage;
    bool m_firstUseDone;
};
#endif // DECLARATIVEWEBUTILS_H
