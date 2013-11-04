/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

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
#include "declarativewebutils.h"
#include "qmozcontext.h"

static const QString system_components_time_stamp("/var/lib/_MOZEMBED_CACHE_CLEAN_");
static const QString profilePath("/.mozilla/mozembed");

DeclarativeWebUtils::DeclarativeWebUtils(QStringList arguments,
                                         BrowserService *service,
                                         QObject *parent) :
    QObject(parent),
    m_homePage("http://www.jolla.com"),
    m_arguments(arguments),
    m_service(service)
{
    connect(QMozContext::GetInstance(), SIGNAL(onInitialized()),
            this, SLOT(updateWebEngineSettings()));
    connect(QMozContext::GetInstance(), SIGNAL(recvObserve(QString, QVariant)),
            this, SLOT(handleObserve(QString, QVariant)));

    connect(service, SIGNAL(openUrlRequested(QString)),
            this, SLOT(openUrl(QString)));

    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QStringLiteral("/.firstUseDone");
    m_firstUseDone = fileExists(path);
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
    // TODO: Enable readpixels based transfer of webgl context image between thread
    // Remove me when 10364 fixed and drivers issue solved
    mozContext->setPref(QString("webgl.force-layers-readback"), QVariant(true));
    mozContext->setPref(QString("geo.wifi.scan"), QVariant(false));
    mozContext->setPref(QString("browser.enable_automatic_image_resizing"), QVariant(true));

    // Default value for embedlite is 0.55f which is a bit slower than needed.
    mozContext->setPref(QString("gfx.axis.velocity_multiplier"), QString("1.0f"));

    // subscribe to gecko messages
    mozContext->addObservers(QStringList()
                             << "clipboard:setdata"
                             << "media-decoder-info"
                             << "embed:download"
                             << "embed:search"
                             << "embedlite-before-first-paint");

    // Enable internet search
    mozContext->setPref(QString("keyword.enabled"), QVariant(true));

    // Scale up content size
    mozContext->setPixelRatio(1.5);

    mozContext->setPref(QString("embedlite.inputItemSize"), QVariant(38));
    mozContext->setPref(QString("embedlite.zoomMargin"), QVariant(14));
}

void DeclarativeWebUtils::openUrl(QString url)
{
    m_arguments << url;

    emit openUrlRequested(url);
}

QString DeclarativeWebUtils::initialPage()
{
    if (m_arguments.count() > 1) {
        return m_arguments.last();
    } else {
        return "";
    }
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

bool DeclarativeWebUtils::firstUseDone() const {
    return m_firstUseDone;
}

QString DeclarativeWebUtils::homePage()
{
    return m_homePage;
}

QString DeclarativeWebUtils::downloadDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

QString DeclarativeWebUtils::picturesDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
}

void DeclarativeWebUtils::deleteThumbnail(QString path) const
{
    QFile f(path);
    if (f.exists()) {
        f.remove();
    }
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
            if (!dataMap.value("defaultEngine").isValid()) {
                QMozContext *mozContext = QMozContext::GetInstance();
                QVariantMap loadsearch;

                // load opensearch descriptions
                loadsearch.insert(QString("msg"), QVariant(QString("loadxml")));
                loadsearch.insert(QString("uri"), QVariant(QString("chrome://embedlite/content/google.xml")));
                loadsearch.insert(QString("confirm"), QVariant(false));
                mozContext->sendObserve("embedui:search", QVariant(loadsearch));
                loadsearch.insert(QString("uri"), QVariant(QString("chrome://embedlite/content/bing.xml")));
                mozContext->sendObserve("embedui:search", QVariant(loadsearch));
                loadsearch.insert(QString("uri"), QVariant(QString("chrome://embedlite/content/yahoo.xml")));
                mozContext->sendObserve("embedui:search", QVariant(loadsearch));
            }
        }
    }
}
