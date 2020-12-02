/****************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtGlobal>
#include <QtQml>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QTranslator>

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


class Q_DECL_EXPORT BrowserSettingsPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "org.sailfishos.browser.settings")

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
        Q_ASSERT(QLatin1String(uri) == QLatin1String("org.sailfishos.browser.settings"));
        qmlRegisterUncreatableType<AppTranslator>(uri, 1, 0, "BrowserSettingsTranslations", "Browser settings translations loaded by import");

    }
};

#include "plugin.moc"
