#include "chromeview.h"

#include <QRegion>
#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>

ChromeView::ChromeView(QWindow *parent)
    : QQuickView(parent)
{
    // TODO: Figure out how to control size and mouse region.
    resize(1536, 2048);
    installEventFilter(this);

    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    create();
    native->setWindowProperty(handle(), QLatin1String("MOUSE_REGION"), QVariant(QRegion(0, 2048-100, 1536, 100)));
}
