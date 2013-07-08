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
#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QStandardPaths>
#include "declarativewebutils.h"
#include "qmozcontext.h"

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
}

QUrl DeclarativeWebUtils::getFaviconForUrl(QUrl url)
{
    QUrl faviconUrl(url);
    faviconUrl.setPath("/favicon.ico");
    return faviconUrl;
}

void DeclarativeWebUtils::processEvents()
{
    QCoreApplication::processEvents();
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
    mozContext->addObserver(QString("embed:download"));

    mozContext->addObserver(QString("clipboard:setdata"));
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

void DeclarativeWebUtils::handleObserve(const QString message, const QVariant data)
{
    const QVariantMap dataMap = data.toMap();

    if (message == "clipboard:setdata") {
        QClipboard *clipboard = QApplication::clipboard();

        // check if we copied password
        if (!dataMap.value("private").toBool()) {
            clipboard->setText(data.toMap().value("data").toString());
        }
    }
}
