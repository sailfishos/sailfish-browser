/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#include <QApplication>
#include <QDeclarativeView>
#include <QGLWidget>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QWidget>
#include <QTimer>
#include <QTranslator>
#include <QDir>

#include "qdeclarativemozview.h"
#include "qgraphicsmozview.h"
#include "qmozcontext.h"

#include "sailfishapplication.h"
#include "declarativebrowsertab.h"
#include "declarativebookmarkmodel.h"
#include "declarativewebutils.h"
#include "browserservice.h"
#include "declarativewebthumbnail.h"
#include "downloadmanager.h"

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_X11InitThreads, true);

    QScopedPointer<QApplication> app(Sailfish::createApplication(argc, argv));
    app->setQuitOnLastWindowClosed(true);

    BrowserService *service = new BrowserService(app.data());

    QString translationPath("/usr/share/translations/");
    QTranslator engineeringEnglish;
    engineeringEnglish.load("sailfish-browser_eng_en", translationPath);
    qApp->installTranslator(&engineeringEnglish);

    QTranslator translator;
    translator.load(QLocale(), "sailfish-browser", "-", translationPath);
    qApp->installTranslator(&translator);

    qmlRegisterType<QGraphicsMozView>("QtMozilla", 1, 0, "QGraphicsMozView");
    qmlRegisterType<QDeclarativeMozView>("QtMozilla", 1, 0, "QmlMozView");
    qmlRegisterType<DeclarativeBookmarkModel>("Sailfish.Browser", 1, 0, "BookmarkModel");
    qmlRegisterType<DeclarativeWebThumbnail>("Sailfish.Browser", 1, 0, "WebThumbnail");

    QString componentPath(DEFAULT_COMPONENTS_PATH);
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/components/EmbedLiteBinComponents.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/components/EmbedLiteJSComponents.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/chrome/EmbedLiteJSScripts.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/chrome/EmbedLiteOverrides.manifest"));

    app->setApplicationName(QString("sailfish-browser"));
    app->setOrganizationName(QString("org.sailfishos"));
    QScopedPointer<QDeclarativeView> view(Sailfish::createView());

    DeclarativeBrowserTab * tab = new DeclarativeBrowserTab(view.data(), app.data());
    DeclarativeWebUtils * utils = new DeclarativeWebUtils(app->arguments(), service, view.data(), app.data());
    view->engine()->rootContext()->setContextProperty("WebUtils", utils);

    view->setViewport(new QGLWidget);
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

    // Setup embedding
    QObject::connect(app.data(), SIGNAL(lastWindowClosed()),
                     QMozContext::GetInstance(), SLOT(stopEmbedding()));
    QTimer::singleShot(0, QMozContext::GetInstance(), SLOT(runEmbedding()));

    Sailfish::showView(view.data());
    return app->exec();
}
