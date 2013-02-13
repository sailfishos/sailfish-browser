#include "declarativeparameters.h"
#include <QDeclarativeEngine>
#include <QDeclarativeContext>

DeclarativeParameters::DeclarativeParameters(QStringList arguments, QDeclarativeView* view, QObject *parent) :
    QObject(parent),
    m_homePage("file:///usr/share/sailfish-browser/pages/demo.html"),
    m_arguments(arguments)
{
    view->engine()->rootContext()->setContextProperty("Parameters",this);
}


QString DeclarativeParameters::initialPage() {

    if (m_arguments.count()>1) {
        return m_arguments.last();
    } else {
        return homePage();
    }
}

QString DeclarativeParameters::homePage() {
    return m_homePage;
}
