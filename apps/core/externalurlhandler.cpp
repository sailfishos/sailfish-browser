/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */
#include "externalurlhandler.h"
#include <intenturl.h>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QUrl>

namespace {
QDBusMessage fileServiceCall(const QString &method, const QString &url)
{
    auto message = QDBusMessage::createMethodCall(QStringLiteral("org.sailfishos.fileservice"),
            QStringLiteral("/"), QStringLiteral("org.sailfishos.fileservice"), method);
    message.setArguments(QVariantList() << url);
    return message;
}
}

ExternalUrlHandler::ExternalUrlHandler(QObject *parent) : QObject(parent) {}

void ExternalUrlHandler::check(const QVariantMap &request)
{
    const QString id = request.value(QStringLiteral("id")).toString();
    if (id.isEmpty() || m_requests.contains(id)) return;
    QVariantMap data = request;
    const QString url = data.value(QStringLiteral("url")).toString();
    const QUrl target(url, QUrl::StrictMode);
    const QSet<QString> internal = {QStringLiteral("http"), QStringLiteral("https"),
        QStringLiteral("file"), QStringLiteral("data"), QStringLiteral("javascript"),
        QStringLiteral("about"), QStringLiteral("chrome"), QStringLiteral("resource")};
    bool valid = target.isValid() && !target.scheme().isEmpty() && !internal.contains(target.scheme());
    data.remove(QStringLiteral("fallback"));
    if (target.scheme() == QLatin1String("intent")) {
        const Lipstick::IntentUrl intent = Lipstick::IntentUrl::parse(url);
        valid = intent.valid;
        if (valid && !intent.fallback.isEmpty()) data.insert(QStringLiteral("fallback"), intent.fallback.toString(QUrl::FullyEncoded));
    }
    if (!valid) {
        emit checked(data, true, false);
        return;
    }
    Request pending;
    pending.data = data;
    m_requests.insert(id, pending);
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(
            fileServiceCall(QStringLiteral("checkUrlSupported"), url), 35000), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, id](QDBusPendingCallWatcher *call) {
        QDBusPendingReply<bool> reply = *call;
        call->deleteLater();
        auto it = m_requests.find(id);
        if (it == m_requests.end()) return;
        it->supported = !reply.isError() && reply.value();
        emit checked(it->data, !reply.isError(), it->supported);
    });
}

void ExternalUrlHandler::open(const QString &id)
{
    auto it = m_requests.find(id);
    if (it == m_requests.end() || !it->supported || it->opening) return;
    it->opening = true;
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(
            fileServiceCall(QStringLiteral("openUrl"), it->data.value(QStringLiteral("url")).toString()), 35000), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, id](QDBusPendingCallWatcher *call) {
        const bool delivered = !QDBusPendingReply<>(*call).isError();
        call->deleteLater();
        auto it = m_requests.find(id);
        if (it == m_requests.end()) return;
        const QVariantMap data = it->data;
        m_requests.erase(it);
        emit dispatched(data, delivered);
    });
}

void ExternalUrlHandler::cancel(const QString &id) { m_requests.remove(id); }
void ExternalUrlHandler::cancelAll() { m_requests.clear(); }
