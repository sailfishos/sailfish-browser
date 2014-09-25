/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QLocale>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QGuiApplication>
#include <QClipboard>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QXmlStreamReader>
#include "declarativewebutils.h"
#include "qmozcontext.h"

static const QString system_components_time_stamp("/var/lib/_MOZEMBED_CACHE_CLEAN_");
static const QString profilePath("/.mozilla/mozembed");
static const QString openSearchPath("/usr/lib/mozembedlite/chrome/embedlite/content/");
static DeclarativeWebUtils *gSingleton = 0;

typedef QMap<QString, QString> StringMap;

const StringMap getAvailableOpenSearchConfigs()
{
    StringMap configs;
    QDir configDir(openSearchPath);
    configDir.setSorting(QDir::Name);

    foreach (QString fileName, configDir.entryList(QStringList("*.xml"))) {
        QFile xmlFile(openSearchPath + fileName);
        xmlFile.open(QIODevice::ReadOnly | QIODevice::Text);
        QXmlStreamReader xml(&xmlFile);
        QString searchEngine;

        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement() && xml.name() == "ShortName") {
                xml.readNext();
                if (xml.isCharacters()) {
                    searchEngine = xml.text().toString();
                }
            }
        }

        if (!xml.hasError()) {
            configs.insert(searchEngine, fileName);
        }

        xmlFile.close();
    }

    return configs;
}

DeclarativeWebUtils::DeclarativeWebUtils()
    : QObject()
    , m_homePage("/apps/sailfish-browser/settings/home_page", this)
    , m_debugMode(qApp->arguments().contains("-debugMode"))
{
    connect(QMozContext::GetInstance(), SIGNAL(onInitialized()),
            this, SLOT(updateWebEngineSettings()));
    connect(QMozContext::GetInstance(), SIGNAL(recvObserve(QString, QVariant)),
            this, SLOT(handleObserve(QString, QVariant)));

    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QStringLiteral("/.firstUseDone");
    m_firstUseDone = fileExists(path);

    connect(&m_homePage, SIGNAL(valueChanged()), this, SIGNAL(homePageChanged()));
}

DeclarativeWebUtils::~DeclarativeWebUtils()
{
    gSingleton = 0;
}

QUrl DeclarativeWebUtils::getFaviconForUrl(QUrl url)
{
    QUrl faviconUrl(url);
    faviconUrl.setPath("/favicon.ico");
    return faviconUrl;
}

int DeclarativeWebUtils::getLightness(QColor color) const
{
    return color.lightness();
}

bool DeclarativeWebUtils::fileExists(QString fileName) const
{
    QFile file(fileName);
    return file.exists();
}

void DeclarativeWebUtils::clearStartupCacheIfNeeded()
{
    QFileInfo systemStamp(system_components_time_stamp);
    if (systemStamp.exists()) {
        QString mostProfilePath = QDir::homePath() + profilePath;
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
    if (m_debugMode) {
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

    QMozContext* mozContext = QMozContext::GetInstance();
    mozContext->setPref(QString("intl.accept_languages"), QVariant(langs));

    // these are magic numbers defining touch radius required to detect <image src=""> touch
    mozContext->setPref(QString("browser.ui.touch.left"), QVariant(32));
    mozContext->setPref(QString("browser.ui.touch.right"), QVariant(32));
    mozContext->setPref(QString("browser.ui.touch.top"), QVariant(48));
    mozContext->setPref(QString("browser.ui.touch.bottom"), QVariant(16));

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

    // Use autodownload, never ask
    mozContext->setPref(QString("browser.download.useDownloadDir"), QVariant(true));
    // see https://developer.mozilla.org/en-US/docs/Download_Manager_preferences
    // Use custom downloads location defined in browser.download.dir
    mozContext->setPref(QString("browser.download.folderList"), QVariant(2));
    mozContext->setPref(QString("browser.download.dir"), downloadDir());
    // Downloads should never be removed automatically
    mozContext->setPref(QString("browser.download.manager.retention"), QVariant(2));
    // Downloads will be canceled on quit
    // TODO: this doesn't really work. Instead the incomplete downloads get restarted
    //       on browser launch.
    mozContext->setPref(QString("browser.download.manager.quitBehavior"), QVariant(2));
    // TODO: this doesn't really work too
    mozContext->setPref(QString("browser.helperApps.deleteTempFileOnExit"), QVariant(true));
    mozContext->setPref(QString("geo.wifi.scan"), QVariant(false));
    mozContext->setPref(QString("browser.enable_automatic_image_resizing"), QVariant(true));

    // Make long press timeout equal to the one in Qt
    mozContext->setPref(QString("ui.click_hold_context_menus.delay"), QVariant(800));
    mozContext->setPref(QString("apz.fling_stopped_threshold"), QString("0.13f"));

    // Don't force 16bit color depth
    mozContext->setPref(QString("gfx.qt.rgb16.force"), QVariant(false));

    mozContext->setPref(QString("media.resource_handler_disabled"), QVariant(true));

    // Disable asmjs
    mozContext->setPref(QString("javascript.options.asmjs"), QVariant(false));

    // subscribe to gecko messages
    mozContext->addObservers(QStringList()
                             << "clipboard:setdata"
                             << "media-decoder-info"
                             << "embed:download"
                             << "embed:allprefs"
                             << "embed:search");

    // Enable internet search
    mozContext->setPref(QString("keyword.enabled"), QVariant(true));

    // Scale up content size
    mozContext->setPixelRatio(1.5);

    // Theme.fontSizeSmall
    mozContext->setPref(QString("embedlite.inputItemSize"), QVariant(28));
    mozContext->setPref(QString("embedlite.zoomMargin"), QVariant(14));

    // Memory management related preferences.
    // We're sending "memory-pressure" when browser is on background (cover by another application)
    // and when the browser page is inactivated.
    mozContext->setPref(QString("javascript.options.gc_on_memory_pressure"), true);
}

void DeclarativeWebUtils::setFirstUseDone(bool firstUseDone) {
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QStringLiteral("/.firstUseDone");
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

bool DeclarativeWebUtils::debugMode() const
{
    return m_debugMode;
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

QString DeclarativeWebUtils::downloadDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

QString DeclarativeWebUtils::picturesDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
}

QString DeclarativeWebUtils::displayableUrl(QString fullUrl) const
{
    QUrl url(fullUrl);
    // Leaving only the scheme, host address, and port (if present).
    return url.toDisplayString(QUrl::RemoveUserInfo |
                               QUrl::RemovePath |
                               QUrl::RemoveQuery |
                               QUrl::RemoveFragment |
                               QUrl::StripTrailingSlash);
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
    } else if (message == "embed:search") {
        QString msg = dataMap.value("msg").toString();
        if (msg == "init") {
            const StringMap configs(getAvailableOpenSearchConfigs());
            QStringList registeredSearches(dataMap.value("engines").toStringList());
            QMozContext *mozContext = QMozContext::GetInstance();

            // Add newly installed configs
            foreach (QString searchName, configs.keys()) {
                if (registeredSearches.contains(searchName)) {
                    registeredSearches.removeAll(searchName);
                } else {
                    QVariantMap loadsearch;

                    // load opensearch descriptions
                    loadsearch.insert(QString("msg"), QVariant(QString("loadxml")));
                    loadsearch.insert(QString("uri"), QVariant(QString("chrome://embedlite/content/") +
                                                               configs.value(searchName)));
                    loadsearch.insert(QString("confirm"), QVariant(false));
                    mozContext->sendObserve("embedui:search", QVariant(loadsearch));
                }
            }

            // Remove uninstalled OpenSearch configs
            foreach(QString searchName, registeredSearches) {
                QVariantMap removeMsg;
                removeMsg.insert(QString("msg"), QVariant(QString("remove")));
                removeMsg.insert(QString("name"), QVariant(searchName));
                mozContext->sendObserve("embedui:search", QVariant(removeMsg));
            }
        }
    }
}
