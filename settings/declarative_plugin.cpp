#include <QtGlobal>
#include <QtQml>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QTranslator>

#include "browsersettings.h"

// using custom translator so it gets properly removed from qApp when engine is deleted
class AppTranslator: public QTranslator
{
    Q_OBJECT
public:
    AppTranslator(QObject *parent)
        : QTranslator(parent)
    {
        qApp->installTranslator(this);
    }

    virtual ~AppTranslator()
    {
        qApp->removeTranslator(this);
    }
};


class BrowserSettingsPlugin : public QQmlExtensionPlugin
{
public:
    void initializeEngine(QQmlEngine *engine, const char *uri)
    {
        Q_UNUSED(uri)

        AppTranslator *engineeringEnglish = new AppTranslator(engine);
        engineeringEnglish->load("settings-sailfish-browser_eng_en", "/usr/share/translations");

        AppTranslator *translator = new AppTranslator(engine);
        translator->load(QLocale(), "settings-sailfish-browser", "-", "/usr/share/translations");
    }

    void registerTypes(const char *uri)
    {
        Q_UNUSED(uri)
        Q_ASSERT(QLatin1String(uri) == QLatin1String("org.sailfishos.browser.settings"));
        qmlRegisterType<BrowserSettings>("org.sailfishos.browser.settings", 1, 0, "BrowserSettings");
    }
};

#include "declarative_plugin.moc"
