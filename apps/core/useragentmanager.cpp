/****************************************************************************
**
** Copyright (c) 2022 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "useragentmanager.h"
#include <QFile>
#include <webengine.h>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "qmozcontext.h"
#include "dbmanager.h"
#include "declarativewebcontainer.h"
#include "faviconmanager.h"

const auto USER_AGENTS_CONFIG = QStringLiteral("/usr/share/sailfish-browser/data/useragents.json");

static UserAgentManager *gSingleton = nullptr;

UserAgentManager::UserAgentManager(QObject *parent)
    : QObject(parent)
{
    initialize();
}

UserAgentManager::~UserAgentManager()
{
    gSingleton = nullptr;
}

void UserAgentManager::initialize()
{
    QScopedPointer<QFile> file(new QFile(USER_AGENTS_CONFIG));

    if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file->readAll(), &error);

    if (doc.isNull()) {
        return;
    }

    if (!doc.isArray()) {
        return;
    }

    QJsonArray array = doc.array();

    m_browserList.clear();
    for (const QJsonValue &value : array) {
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            QVariantMap val;
            val["key"] = obj.value("key").toString();
            val["name"] = obj.value("name").toString();
            m_browserList.push_back(val);
        }
    }

    QVariant conf = doc.toVariant();
    QVariantMap overrides = getUserAgentOverrides();

    QMozContext *mozContext = QMozContext::instance();
    if (mozContext->isInitialized()) {
        SailfishOS::WebEngine::instance()->notifyObservers(QLatin1String("useragentlistchanged"), conf);
        SailfishOS::WebEngine::instance()->notifyObservers(QLatin1String("useragentoverrideschanged"), overrides);
    } else {
        QObject *context = new QObject(this);
        connect(mozContext, &QMozContext::initialized, context, [conf, overrides, context]() {
            SailfishOS::WebEngine::instance()->notifyObservers(QLatin1String("useragentlistchanged"), conf);
            SailfishOS::WebEngine::instance()->notifyObservers(QLatin1String("useragentoverrideschanged"), overrides);
            context->deleteLater();
        });
    }
}

QVariantMap UserAgentManager::getUserAgentOverrides() const
{
    return DBManager::instance()->getUserAgentOverrides();
}

void UserAgentManager::setUserAgentOverride(const QString &host, const QString &userAgent, const bool isKey)
{
    DBManager::instance()->setUserAgentOverride(host, isKey, userAgent);
    SailfishOS::WebEngine::instance()->notifyObservers(QLatin1String("useragentoverrideschanged"),
                                                       QVariantMap{
                                                           std::pair<QString, QVariant>(
                                                           host,
                                                           QVariantList{isKey, userAgent}
                                                           )
                                                       });

    DeclarativeWebContainer *webContainer = DeclarativeWebContainer::instance();
    if (webContainer->webPage() && QUrl(webContainer->url()).host() == host) {
        FaviconManager::instance()->grabIcon("userAgents", webContainer->webPage(), QSize(64, 64));
    }
}

void UserAgentManager::unsetUserAgentOverride(const QString &host) {
    DBManager::instance()->unsetUserAgentOverride(host);
    qWarning() << "#### UserAgentManager::setUserAgentOverride";
    SailfishOS::WebEngine::instance()->notifyObservers(QLatin1String("useragentoverrideschanged"),
                                                       QVariantMap{
                                                           std::pair<QString, QVariant>(
                                                           host,
                                                           QVariantList{true, ""}
                                                           )
                                                       });
    FaviconManager::instance()->remove("userAgents", "https://" + host);
    FaviconManager::instance()->remove("userAgents", "http://" + host);
}

void UserAgentManager::clearUserAgentOverrides()
{
    qWarning() << "UserAgentManager::clearUserAgentOverrides";
    DBManager::instance()->clearUserAgentOverrides();
    SailfishOS::WebEngine::instance()->notifyObservers(QLatin1String("useragentoverrideschanged"),
                                                       QVariantMap{});
}

QVariantList UserAgentManager::getBrowserList() const
{
    return m_browserList;
}

UserAgentManager *UserAgentManager::instance()
{
    if (!gSingleton) {
        gSingleton = new UserAgentManager();
    }
    return gSingleton;
}
