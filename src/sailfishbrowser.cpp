/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QtQml>
#include <QTimer>
#include <QTranslator>
#include <QDir>
#include <QScreen>

//#include "qdeclarativemozview.h"
#include "quickmozview.h"
#include "qmozcontext.h"

#include "declarativebrowsertab.h"
#include "declarativebookmarkmodel.h"
#include "declarativewebutils.h"
#include "browserservice.h"
#include "declarativewebthumbnail.h"
#include "downloadmanager.h"

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    setenv("QML_BAD_GUI_RENDER_LOOP", "1", 1);
    QScopedPointer<QGuiApplication> app(new QGuiApplication(argc, argv));
    QScopedPointer<QQuickView> view(new QQuickView);

    app->setQuitOnLastWindowClosed(true);

    BrowserService *service = new BrowserService(app.data());

    QString translationPath("/usr/share/translations/");
    QTranslator engineeringEnglish;
    engineeringEnglish.load("sailfish-browser_eng_en", translationPath);
    qApp->installTranslator(&engineeringEnglish);

    QTranslator translator;
    translator.load(QLocale(), "sailfish-browser", "-", translationPath);
    qApp->installTranslator(&translator);

    qmlRegisterType<DeclarativeBookmarkModel>("Sailfish.Browser", 1, 0, "BookmarkModel");
    qmlRegisterType<DeclarativeWebThumbnail>("Sailfish.Browser", 1, 0, "WebThumbnail");

    QString componentPath(DEFAULT_COMPONENTS_PATH);
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/components/EmbedLiteBinComponents.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/components/EmbedLiteJSComponents.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/chrome/EmbedLiteJSScripts.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/chrome/EmbedLiteOverrides.manifest"));

    app->setApplicationName(QString("sailfish-browser"));
    app->setOrganizationName(QString("org.sailfishos"));

    DeclarativeBrowserTab * tab = new DeclarativeBrowserTab(view.data(), app.data());
    DeclarativeWebUtils * utils = new DeclarativeWebUtils(app->arguments(), service, app.data());
    view->rootContext()->setContextProperty("WebUtils", utils);
    view->rootContext()->setContextProperty("MozContext", QMozContext::GetInstance());

    DownloadManager dlMgr(service);
    QObject::connect(app.data(), SIGNAL(lastWindowClosed()),
                     &dlMgr, SLOT(cancelActiveTransfers()));

    bool isDesktop = qApp->arguments().contains("-desktop");

    QString path;
    if (isDesktop) {
        path = qApp->applicationDirPath() + QDir::separator();
    } else {
        path = QString(DEPLOYMENT_PATH);
    }
    view->setSource(QUrl::fromLocalFile(path+"browser.qml"));
  // QRect r = QGuiApplication::primaryScreen()->geometry();
  //         view->resize(r.width(), r.height());
    view->showFullScreen();

    // Setup embedding
    QObject::connect(app.data(), SIGNAL(lastWindowClosed()),
                     QMozContext::GetInstance(), SLOT(stopEmbedding()));
    QTimer::singleShot(0, QMozContext::GetInstance(), SLOT(runEmbedding()));

    return app->exec();
}
