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

#include <silicatheme.h>

#include <math.h>
#include "declarativewebutils.h"
#include "qmozcontext.h"
#include "browserpaths.h"

static const QString gSystemComponentsTimeStamp("/var/lib/_MOZEMBED_CACHE_CLEAN_");
static const QString gProfilePath("/.mozilla/mozembed");

static DeclarativeWebUtils *gSingleton = 0;
static const qreal gCssPixelRatioRoundingFactor = 0.5;
static const qreal gCssDefaultPixelRatio = 1.5;

static bool testScreenDimensions(qreal pixelRatio) {
    QScreen *screen = QGuiApplication::primaryScreen();
    qreal w = screen->size().width() / pixelRatio;
    qreal h = screen->size().height() / pixelRatio;

    return fmod(w, 1.0) == 0 && fmod(h, 1.0) == 0;
}

const QSettings &quickSettings()
{
    static const QSettings settings(QSettings::SystemScope, QStringLiteral("QtProject"), QStringLiteral("QtQuick2"));
    return settings;
}

int getPressAndHoldDelay()
{
    return quickSettings().value(QStringLiteral("QuickMouseArea/PressAndHoldDelay"), 800).toInt();
}

const int PressAndHoldDelay(getPressAndHoldDelay());

static bool fileExists(QString fileName)
{
    QFile file(fileName);
    return file.exists();
}

DeclarativeWebUtils::DeclarativeWebUtils()
    : QObject()
    , m_homePage("/apps/sailfish-browser/settings/home_page", this)
    , m_touchSideRadius(32.0)
    , m_touchTopRadius(48.0)
    , m_touchBottomRadius(16.0)
    , m_inputItemSize(28.0)
    , m_zoomMargin(14.0)
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
    // Infer and set Accept-Language header from the current system locale
    QString langs;
    QStringList locale = QLocale::system().name().split("_", QString::SkipEmptyParts);
    if (locale.size() > 1) {
        langs = QString("%1-%2,%3").arg(locale.at(0)).arg(locale.at(1)).arg(locale.at(0));
    } else {
        langs = locale.at(0);
    }

    QMozContext* mozContext = QMozContext::instance();

    mozContext->setPref(QStringLiteral("general.useragent.updates.url"),
                        QStringLiteral("https://browser.sailfishos.org/gecko/%APP_VERSION%/ua-update.json"));
    mozContext->setPref(QStringLiteral("general.useragent.updates.interval"), QVariant(604800)); // 1 week
    mozContext->setPref(QStringLiteral("general.useragent.updates.retry"), QVariant(86400)); // 1 day

    mozContext->setPref(QString("intl.accept_languages"), QVariant(langs));

    // these are magic numbers defining touch radius required to detect <image src=""> touch
    mozContext->setPref(QStringLiteral("browser.ui.touch.left"), QVariant(m_touchSideRadius));
    mozContext->setPref(QStringLiteral("browser.ui.touch.right"), QVariant(m_touchSideRadius));
    mozContext->setPref(QStringLiteral("browser.ui.touch.top"), QVariant(m_touchTopRadius));
    mozContext->setPref(QStringLiteral("browser.ui.touch.bottom"), QVariant(m_touchBottomRadius));

    // Install embedlite handlers for guestures
    mozContext->setPref(QString("embedlite.azpc.handle.singletap"), QVariant(false));
    mozContext->setPref(QString("embedlite.azpc.json.singletap"), QVariant(true));
    mozContext->setPref(QString("embedlite.azpc.handle.longtap"), QVariant(false));
    mozContext->setPref(QString("embedlite.azpc.json.longtap"), QVariant(true));
    mozContext->setPref(QString("embedlite.azpc.json.viewport"), QVariant(true));

    // Without this pref placeholders get cleaned as soon as a character gets committed
    // by VKB and that happens only when Enter is pressed or comma/space/dot is entered.
    mozContext->setPref(QString("dom.placeholder.show_on_focus"), QVariant(false));

    mozContext->setPref(QString("security.alternate_certificate_error_page"), QString("certerror"));

    mozContext->setPref(QString("geo.wifi.scan"), QVariant(false));
    mozContext->setPref(QString("browser.enable_automatic_image_resizing"), QVariant(true));

    // Make long press timeout equal to the one in Qt
    mozContext->setPref(QString("ui.click_hold_context_menus.delay"), QVariant(PressAndHoldDelay));
    mozContext->setPref(QString("apz.fling_stopped_threshold"), QString("0.13f"));

    // TODO: remove this line when the value adjusted for different DPIs makes
    // its way to Gecko's default prefs.
    mozContext->setPref(QString("apz.touch_start_tolerance"), QString("0.027777f"));

    mozContext->setPref(QString("media.resource_handler_disabled"), QVariant(true));

    // subscribe to gecko messages
    mozContext->addObservers(QStringList()
                             << "clipboard:setdata"
                             << "media-decoder-info"
                             << "embed:download"
                             << "embed:allprefs"
                             << "embed:search");

    // Enable internet search
    mozContext->setPref(QString("keyword.enabled"), QVariant(true));

    Silica::Theme *silicaTheme = Silica::Theme::instance();
    setZoomMargin(silicaTheme->paddingMedium());
    setInputItemSize(silicaTheme->fontSizeSmall());

    // Scale up content size
    setContentScaling();
    setRenderingPreferences();

    // Disable SSLv3
    mozContext->setPref(QString("security.tls.version.min"), QVariant(1));

    // Content Security Policy is enabled by default,
    // this enables the spec compliant mode.
    mozContext->setPref(QString("security.csp.speccompliant"), QVariant(true));
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

qreal DeclarativeWebUtils::touchSideRadius() const
{
    return m_touchSideRadius;
}

void DeclarativeWebUtils::setTouchSideRadius(qreal touchSideRadius)
{
    if (m_touchSideRadius != touchSideRadius) {
        m_touchSideRadius = touchSideRadius;
        QMozContext* mozContext = QMozContext::instance();
        mozContext->setPref(QStringLiteral("browser.ui.touch.left"), QVariant(m_touchSideRadius));
        mozContext->setPref(QStringLiteral("browser.ui.touch.right"), QVariant(m_touchSideRadius));
        emit touchSideRadiusChanged();
    }
}

qreal DeclarativeWebUtils::touchTopRadius() const
{
    return m_touchTopRadius;
}

void DeclarativeWebUtils::setTouchTopRadius(qreal touchTopRadius)
{
    if (m_touchTopRadius != touchTopRadius) {
        m_touchTopRadius = touchTopRadius;
        QMozContext::instance()->setPref(QStringLiteral("browser.ui.touch.top"), QVariant(m_touchTopRadius));
        emit touchTopRadiusChanged();
    }
}

qreal DeclarativeWebUtils::touchBottomRadius() const
{
    return m_touchBottomRadius;
}

void DeclarativeWebUtils::setTouchBottomRadius(qreal touchBottomRadius)
{
    if (m_touchBottomRadius != touchBottomRadius) {
        m_touchBottomRadius = touchBottomRadius;
        QMozContext::instance()->setPref(QStringLiteral("browser.ui.touch.bottom"), QVariant(m_touchBottomRadius));
        emit touchBottomRadiusChanged();
    }
}

qreal DeclarativeWebUtils::inputItemSize() const
{
    return m_inputItemSize;
}

void DeclarativeWebUtils::setInputItemSize(qreal inputItemSize)
{
    if (m_inputItemSize != inputItemSize) {
        m_inputItemSize = inputItemSize;
        QMozContext::instance()->setPref(QStringLiteral("embedlite.inputItemSize"), QVariant(m_inputItemSize));
        emit inputItemSizeChanged();
    }
}

qreal DeclarativeWebUtils::zoomMargin() const
{
    return m_zoomMargin;
}

void DeclarativeWebUtils::setZoomMargin(qreal zoomMargin)
{
    if (m_zoomMargin != zoomMargin) {
        m_zoomMargin = zoomMargin;
        QMozContext::instance()->setPref(QStringLiteral("embedlite.zoomMargin"), QVariant(m_zoomMargin));
        emit zoomMarginChanged();
    }
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

void DeclarativeWebUtils::setContentScaling()
{
    QMozContext* mozContext = QMozContext::instance();
    qreal mozCssPixelRatio = gCssDefaultPixelRatio * Silica::Theme::instance()->pixelRatio();
    // Round to nearest even rounding factor
    mozCssPixelRatio = qRound(mozCssPixelRatio / gCssPixelRatioRoundingFactor) * gCssPixelRatioRoundingFactor;

    // If we're on hdpi and calcaluted pixel ratio doesn't result integer dimensions, let's try to floor it.
    if (mozCssPixelRatio >= 2.0 && !testScreenDimensions(mozCssPixelRatio)) {
        qreal tempPixelRatio = qFloor(mozCssPixelRatio);
        if (testScreenDimensions(tempPixelRatio)) {
            mozCssPixelRatio = tempPixelRatio;
        }
    }

    mozContext->setPixelRatio(mozCssPixelRatio);
    emit cssPixelRatioChanged();
}

void DeclarativeWebUtils::setRenderingPreferences()
{
    QMozContext* mozContext = QMozContext::instance();

    // Don't force 16bit color depth
    mozContext->setPref(QString("gfx.qt.rgb16.force"), QVariant(false));

    // We're not relying on async scroll events. So lets fire them
    // less often than the default value (100) or B2G (15). Currently only toolbar
    // is interested about async scroll events and it's not urgent to be
    // updated immediately.
    mozContext->setPref(QString("apz.asyncscroll.throttle"), QVariant(400));
    mozContext->setPref(QString("apz.asyncscroll.timeout"), QVariant(400));

    // Use external Qt window for rendering content
    mozContext->setPref(QString("gfx.compositor.external-window"), QVariant(true));
    mozContext->setPref(QString("gfx.compositor.clear-context"), QVariant(false));
    mozContext->setPref(QString("embedlite.compositor.external_gl_context"), QVariant(true));

    // Enable progressive painting.
    mozContext->setPref(QString("layers.progressive-paint"), QVariant(true));
    mozContext->setPref(QString("layers.low-precision-buffer"), QVariant(true));

    int screenWidth = QGuiApplication::primaryScreen()->size().width();
    int tileSize = screenWidth;
    // With bigger than qHD screen fill with two tiles in row (portrait).
    // Landscape will be filled with same tile size.
    if (screenWidth > 540) {
        tileSize = screenWidth / 2;
    }

    mozContext->setPref(QString("layers.tile-width"), QVariant(tileSize));
    mozContext->setPref(QString("layers.tile-height"), QVariant(tileSize));
    if (mozContext->pixelRatio() >= 2.0) {
        // Don't use too small low precision buffers for high dpi devices. This reduces
        // a bit the blurriness.
        mozContext->setPref(QString("layers.low-precision-resolution"), QString("0.5f"));
    }
}
