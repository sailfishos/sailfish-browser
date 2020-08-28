/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// QtCore
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringList>
#include <QtCore/QtMath>
#include <QtCore/QVariant>
#include <QtCore/QVariantMap>

// QtGui
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

#include <webengine.h>
#include <webenginesettings.h>

#include <math.h>
#include "declarativewebutils.h"
#include "browserpaths.h"

static const auto defaultUserAgentUpdateUrl = QStringLiteral("https://browser.sailfishos.org/gecko/%APP_VERSION%/ua-update.json");

static DeclarativeWebUtils *gSingleton = 0;

static bool fileExists(QString fileName)
{
    QFile file(fileName);
    return file.exists();
}

DeclarativeWebUtils::DeclarativeWebUtils()
    : QObject()
    , m_homePage("/apps/sailfish-browser/settings/home_page", this)
{
    updateWebEngineSettings();
    connect(SailfishOS::WebEngine::instance(), &SailfishOS::WebEngine::recvObserve,
            this, &DeclarativeWebUtils::handleObserve);

    QString path = BrowserPaths::dataLocation() + QStringLiteral("/.firstUseDone");
    m_firstUseDone = fileExists(path);

    connect(&m_homePage, &MGConfItem::valueChanged,
            this, &DeclarativeWebUtils::homePageChanged);
}

DeclarativeWebUtils::~DeclarativeWebUtils()
{
    gSingleton = 0;
}

int DeclarativeWebUtils::getLightness(const QColor &color) const
{
    return color.lightness();
}

void DeclarativeWebUtils::handleDumpMemoryInfoRequest(const QString &fileName)
{
    if (qApp->arguments().contains("-debugMode")) {
        emit dumpMemoryInfo(fileName);
    }
}

void DeclarativeWebUtils::openUrl(const QString &url)
{

    QFileInfo fileInfo(url);
    QUrl targetUrl(url);

    if (!url.isEmpty() && targetUrl.scheme().isEmpty()) {
        QUrl tmpUrl;
        if (fileInfo.isAbsolute()) {
            tmpUrl = QUrl::fromLocalFile(url);
        } else {
            QUrl baseUrl = QUrl::fromLocalFile(QDir::currentPath() + QDir::separator());
            tmpUrl = baseUrl.resolved(url);
        }

        if (QFileInfo::exists(tmpUrl.path())) {
            targetUrl = tmpUrl;
        }
    }

    QString tmpUrl = targetUrl.toEncoded();
    emit openUrlRequested(tmpUrl);
}

void DeclarativeWebUtils::updateWebEngineSettings()
{
    SailfishOS::WebEngineSettings *webEngineSettings = SailfishOS::WebEngineSettings::instance();
    SailfishOS::WebEngine *webEngine = SailfishOS::WebEngine::instance();

    webEngineSettings->setPreference(QStringLiteral("general.useragent.updates.enabled"),
                        MGConfItem(QStringLiteral("/apps/sailfish-browser/settings/useragent_update_enabled")).value(QVariant(true)));
    webEngineSettings->setPreference(QStringLiteral("general.useragent.updates.url"),
                        MGConfItem(QStringLiteral("/apps/sailfish-browser/settings/useragent_update_url")).value(defaultUserAgentUpdateUrl));
    webEngineSettings->setPreference(QStringLiteral("general.useragent.updates.interval"),
                        MGConfItem(QStringLiteral("/apps/sailfish-browser/settings/useragent_update_interval")).value(QVariant(172800))); // every 2nd day
    webEngineSettings->setPreference(QStringLiteral("general.useragent.updates.retry"),
                        MGConfItem(QStringLiteral("/apps/sailfish-browser/settings/useragent_update_retry")).value(QVariant(86400))); // 1 day

    // Without this pref placeholders get cleaned as soon as a character gets committed
    // by VKB and that happens only when Enter is pressed or comma/space/dot is entered.
    webEngineSettings->setPreference(QString("dom.placeholder.show_on_focus"), QVariant(false));
    webEngineSettings->setPreference(QString("geo.wifi.scan"), QVariant(false));
    webEngineSettings->setPreference(QString("media.resource_handler_disabled"), QVariant(true));

    // subscribe to gecko messages
    std::vector<std::string> messages = { "clipboard:setdata",
                                          "media-decoder-info",
                                          "embed:download",
                                          "embed:allprefs",
                                          "embed:search" };
    webEngine->addObservers(messages);

    // Enable internet search
    webEngineSettings->setPreference(QString("keyword.enabled"), QVariant(true));

    setRenderingPreferences();
}

void DeclarativeWebUtils::setFirstUseDone(bool firstUseDone) {
    QString path = BrowserPaths::dataLocation() + QStringLiteral("/.firstUseDone");
    if (m_firstUseDone != firstUseDone) {
        m_firstUseDone = firstUseDone;
        if (!firstUseDone) {
            QFile f(path);
            f.remove();
        } else {
            QProcess process;
            process.startDetached("touch", QStringList() << path);
        }
        emit firstUseDoneChanged();
    }
}

qreal DeclarativeWebUtils::cssPixelRatio() const
{
    SailfishOS::WebEngineSettings *webEngineSettings = SailfishOS::WebEngineSettings::instance();
    if (webEngineSettings) {
        return webEngineSettings->pixelRatio();
    }
    return 1.0;
}

bool DeclarativeWebUtils::firstUseDone() const {
    return m_firstUseDone;
}

QString DeclarativeWebUtils::homePage() const
{
    return m_homePage.value("http://jolla.com").value<QString>();
}

DeclarativeWebUtils *DeclarativeWebUtils::instance()
{
    if (!gSingleton) {
        gSingleton = new DeclarativeWebUtils();
    }
    return gSingleton;
}

QString DeclarativeWebUtils::displayableUrl(const QString &fullUrl) const
{
    QUrl url(fullUrl);
    // Leaving only the scheme, host address, and port (if present).
    QString returnUrl = url.toDisplayString(QUrl::RemoveUserInfo |
                                         QUrl::RemovePath |
                                         QUrl::RemoveQuery |
                                         QUrl::RemoveFragment |
                                         QUrl::StripTrailingSlash);
    returnUrl.remove(0, returnUrl.lastIndexOf("/") + 1);
    if (returnUrl.indexOf("www.") == 0) {
        return returnUrl.remove(0, 4);
    } else if (returnUrl.indexOf("m.") == 0 && returnUrl.length() > 2) {
        return returnUrl.remove(0, 2);
    } else if (returnUrl.indexOf("mobile.") == 0 && returnUrl.length() > 7) {
        return returnUrl.remove(0, 7);
    }

    return !returnUrl.isEmpty() ? returnUrl : fullUrl;
}

void DeclarativeWebUtils::handleObserve(const QString &message, const QVariant &data)
{
    const QVariantMap dataMap = data.toMap();
    if (message == "clipboard:setdata") {
        QClipboard *clipboard = QGuiApplication::clipboard();

        // check if we copied password
        if (!dataMap.value("private").toBool()) {
            clipboard->setText(dataMap.value("data").toString());
        }
    }
}

void DeclarativeWebUtils::setRenderingPreferences()
{
    SailfishOS::WebEngineSettings *webEngineSettings = SailfishOS::WebEngineSettings::instance();

    // Use external Qt window for rendering content
    webEngineSettings->setPreference(QString("gfx.compositor.external-window"), QVariant(true));
    webEngineSettings->setPreference(QString("gfx.compositor.clear-context"), QVariant(false));
    webEngineSettings->setPreference(QString("embedlite.compositor.external_gl_context"), QVariant(true));
}
