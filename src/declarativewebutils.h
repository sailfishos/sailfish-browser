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
    Q_PROPERTY(QString downloadDir READ downloadDir CONSTANT FINAL)
    Q_PROPERTY(QString picturesDir READ picturesDir CONSTANT FINAL)
    Q_PROPERTY(bool firstUseDone READ firstUseDone WRITE setFirstUseDone NOTIFY firstUseDoneChanged)
    Q_PROPERTY(bool debugMode READ debugMode CONSTANT FINAL)
    Q_PROPERTY(qreal cssPixelRatio READ cssPixelRatio NOTIFY cssPixelRatioChanged)
    Q_PROPERTY(qreal silicaPixelRatio READ silicaPixelRatio WRITE setSilicaPixelRatio NOTIFY silicaPixelRatioChanged)
    Q_PROPERTY(qreal touchSideRadius READ touchSideRadius WRITE setTouchSideRadius NOTIFY touchSideRadiusChanged)
    Q_PROPERTY(qreal touchTopRadius READ touchTopRadius WRITE setTouchTopRadius NOTIFY touchTopRadiusChanged)
    Q_PROPERTY(qreal touchBottomRadius READ touchBottomRadius WRITE setTouchBottomRadius NOTIFY touchBottomRadiusChanged)
    Q_PROPERTY(qreal inputItemSize READ inputItemSize WRITE setInputItemSize NOTIFY inputItemSizeChanged)
    Q_PROPERTY(qreal zoomMargin READ zoomMargin WRITE setZoomMargin NOTIFY zoomMarginChanged)

public:
    static DeclarativeWebUtils *instance();

    QString downloadDir() const;
    QString picturesDir() const;
    bool firstUseDone() const;
    void setFirstUseDone(bool firstUseDone);
    bool debugMode() const;
    qreal cssPixelRatio() const;
    qreal silicaPixelRatio() const;
    void setSilicaPixelRatio(qreal silicaPixelRatio);
    qreal touchSideRadius() const;
    void setTouchSideRadius(qreal touchSideRadius);
    qreal touchTopRadius() const;
    void setTouchTopRadius(qreal touchTopRadius);
    qreal touchBottomRadius() const;
    void setTouchBottomRadius(qreal touchBottomRadius);
    qreal inputItemSize() const;
    void setInputItemSize(qreal inputItemSize);
    qreal zoomMargin() const;
    void setZoomMargin(qreal zoomMargin);

    Q_INVOKABLE int getLightness(QColor color) const;
    Q_INVOKABLE bool fileExists(QString fileName) const;
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
    void beforeShutdown();
    void cssPixelRatioChanged();
    void silicaPixelRatioChanged();
    void touchSideRadiusChanged();
    void touchTopRadiusChanged();
    void touchBottomRadiusChanged();
    void inputItemSizeChanged();
    void zoomMarginChanged();

private slots:
    void updateWebEngineSettings();
    void handleObserve(const QString message, const QVariant data);

private:
    explicit DeclarativeWebUtils();
    ~DeclarativeWebUtils();
    void setContentScaling();

    MGConfItem m_homePage;
    bool m_firstUseDone;
    bool m_debugMode;
    qreal m_silicaPixelRatio;
    qreal m_touchSideRadius;
    qreal m_touchTopRadius;
    qreal m_touchBottomRadius;
    qreal m_inputItemSize;
    qreal m_zoomMargin;
};
#endif // DECLARATIVEWEBUTILS_H
