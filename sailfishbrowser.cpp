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

#include "qdeclarativemozview.h"
#include "qgraphicsmozview.h"
#include "qmozcontext.h"

#include "sailfishapplication.h"
#include "src/declarativebrowsertab.h"
#include "src/declarativeparameters.h"

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_X11InitThreads, true);

    QScopedPointer<QApplication> app(Sailfish::createApplication(argc, argv));
    app->setQuitOnLastWindowClosed(true);

    QString componentPath(DEFAULT_COMPONENTS_PATH);
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/EmbedLiteBinComponents.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/EmbedLiteJSComponents.manifest"));

    qmlRegisterType<QGraphicsMozView>("QtMozilla", 1, 0, "QGraphicsMozView");
    qmlRegisterType<QDeclarativeMozView>("QtMozilla", 1, 0, "QmlMozView");

    QScopedPointer<QDeclarativeView> view(Sailfish::createView("browser.qml"));
    app->setApplicationName(QString("sailfish-browser"));
    app->setOrganizationName(QString("org.sailfishos"));

    DeclarativeBrowserTab * tab = new DeclarativeBrowserTab(view.data(), app.data());
    DeclarativeParameters * parameters = new DeclarativeParameters(app->arguments(), view.data(), app.data());

    view->setViewport(new QGLWidget);
    Sailfish::showView(view.data());
    return app->exec();
}
