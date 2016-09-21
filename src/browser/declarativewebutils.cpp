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
#include "qmozcontext.h"
#include "browserpaths.h"

static const QString gSystemComponentsTimeStamp("/var/lib/_MOZEMBED_CACHE_CLEAN_");
static const QString gProfilePath("/.mozilla/mozembed");

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
    connect(QMozContext::instance(), SIGNAL(recvObserve(QString, QVariant)),
            this, SLOT(handleObserve(QString, QVariant)));

    QString path = BrowserPaths::dataLocation() + QStringLiteral("/.firstUseDone");
    m_firstUseDone = fileExists(path);

    connect(&m_homePage, SIGNAL(valueChanged()), this, SIGNAL(homePageChanged()));
}

DeclarativeWebUtils::~DeclarativeWebUtils()
{
    gSingleton = 0;
}

int DeclarativeWebUtils::getLightness(QColor color) const
{
    return color.lightness();
}

QString DeclarativeWebUtils::createUniqueFileUrl(const QString &fileName, const QString &path) const
{
    if (path.isEmpty() || fileName.isEmpty()) {
        return QString();
    }

    const QFileInfo fileInfo(fileName);
    const QString newFile("%1/%2(%3)%4%5");
    const QString baseName = fileInfo.baseName();
    const QString suffix = fileInfo.completeSuffix();

    QString result(path + "/" + fileName);
    int collisionCount(1);

    while (QFileInfo::exists(result)) {
        collisionCount++;
        result = newFile.arg(path).arg(baseName).arg(collisionCount).arg(suffix.isEmpty() ? "" : ".").arg(suffix);
    }

    return result;
}

void DeclarativeWebUtils::clearStartupCacheIfNeeded()
{
    QFileInfo systemStamp(gSystemComponentsTimeStamp);
    if (systemStamp.exists()) {
        QString mostProfilePath = QDir::homePath() + gProfilePath;
        QString localStampString(mostProfilePath + QString("/_CACHE_CLEAN_"));
        QFileInfo localStamp(localStampString);
        if (localStamp.exists() && systemStamp.lastModified() > localStamp.lastModified()) {
            QDir cacheDir(mostProfilePath + "/startupCache");
            cacheDir.removeRecursively();
            QFile(localStampString).remove();
        }
    }
}

void DeclarativeWebUtils::handleDumpMemoryInfoRequest(QString fileName)
{
    if (qApp->arguments().contains("-debugMode")) {
        emit dumpMemoryInfo(fileName);
    }
}

void DeclarativeWebUtils::updateWebEngineSettings()
{
    SailfishOS::WebEngineSettings *webEngineSettings = SailfishOS::WebEngineSettings::instance();
    SailfishOS::WebEngine *webEngine = SailfishOS::WebEngine::instance();

    webEngineSettings->setPreference(QStringLiteral("general.useragent.updates.url"),
                        QStringLiteral("https://browser.sailfishos.org/gecko/%APP_VERSION%/ua-update.json"));
    webEngineSettings->setPreference(QStringLiteral("general.useragent.updates.interval"), QVariant(604800)); // 1 week
    webEngineSettings->setPreference(QStringLiteral("general.useragent.updates.retry"), QVariant(86400)); // 1 day

    // Without this pref placeholders get cleaned as soon as a character gets committed
    // by VKB and that happens only when Enter is pressed or comma/space/dot is entered.
    webEngineSettings->setPreference(QString("dom.placeholder.show_on_focus"), QVariant(false));

    webEngineSettings->setPreference(QString("security.alternate_certificate_error_page"), QString("certerror"));

    webEngineSettings->setPreference(QString("geo.wifi.scan"), QVariant(false));

    // TODO: remove this line when the value adjusted for different DPIs makes
    // its way to Gecko's default prefs.
    webEngineSettings->setPreference(QString("apz.touch_start_tolerance"), QString("0.027777f"));

    webEngineSettings->setPreference(QString("media.resource_handler_disabled"), QVariant(true));

    // subscribe to gecko messages
    webEngine->addObservers(QStringList()
                            << "clipboard:setdata"
                            << "media-decoder-info"
                            << "embed:download"
                            << "embed:allprefs"
                            << "embed:search");

    // Enable internet search
    webEngineSettings->setPreference(QString("keyword.enabled"), QVariant(true));

    setRenderingPreferences();

    // Disable SSLv3
    webEngineSettings->setPreference(QString("security.tls.version.min"), QVariant(1));

    // Content Security Policy is enabled by default,
    // this enables the spec compliant mode.
    webEngineSettings->setPreference(QString("security.csp.speccompliant"), QVariant(true));
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
    QMozContext* mozContext = QMozContext::instance();
    if (mozContext) {
        return mozContext->pixelRatio();
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

QString DeclarativeWebUtils::displayableUrl(QString fullUrl) const
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

void DeclarativeWebUtils::handleObserve(const QString message, const QVariant data)
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

    if (webEngineSettings->pixelRatio() >= 2.0) {
        // Don't use too small low precision buffers for high dpi devices. This reduces
        // a bit the blurriness.
        webEngineSettings->setPreference(QString("layers.low-precision-resolution"), QString("0.5f"));
    }
}
