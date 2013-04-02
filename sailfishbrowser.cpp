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
#include <QInputContext>
#include <QWidget>
#include <QTimer>

#include "qdeclarativemozview.h"
#include "qgraphicsmozview.h"
#include "qmozcontext.h"

#include "sailfishapplication.h"
#include "src/declarativebrowsertab.h"
#include "src/declarativeparameters.h"
#include "src/declarativebookmarkmodel.h"

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_X11InitThreads, true);

    QScopedPointer<QApplication> app(Sailfish::createApplication(argc, argv));
    app->setQuitOnLastWindowClosed(true);

    qmlRegisterType<QGraphicsMozView>("QtMozilla", 1, 0, "QGraphicsMozView");
    qmlRegisterType<QDeclarativeMozView>("QtMozilla", 1, 0, "QmlMozView");
    qmlRegisterType<DeclarativeBookmarkModel>("Sailfish.Browser", 1, 0, "BookmarkModel");

    QString componentPath(DEFAULT_COMPONENTS_PATH);
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/components/EmbedLiteBinComponents.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/components/EmbedLiteJSComponents.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/chrome/EmbedLiteJSScripts.manifest"));

    app->setApplicationName(QString("sailfish-browser"));
    app->setOrganizationName(QString("org.sailfishos"));
    QScopedPointer<QDeclarativeView> view(Sailfish::createView("browser.qml"));

    DeclarativeBrowserTab * tab = new DeclarativeBrowserTab(view.data(), app.data());
    DeclarativeParameters * parameters = new DeclarativeParameters(app->arguments(), view.data(), app.data());

    view->setViewport(new QGLWidget);
    view->rootContext()->setContextProperty("MozContext", QMozContext::GetInstance());

    // Setup embedding
    QObject::connect(app.data(), SIGNAL(lastWindowClosed()),
                     QMozContext::GetInstance(), SLOT(stopEmbedding()));
    QTimer::singleShot(0, QMozContext::GetInstance(), SLOT(runEmbedding()));

    Sailfish::showView(view.data());
    return app->exec();
}
