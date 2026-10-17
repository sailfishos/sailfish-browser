/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */
#include "portalflowservice.h"

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMessage>
#include <QUuid>
#include <QUrlQuery>

class PortalFlowAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.sailfishos.captiveportal.Flow1")
public:
    explicit PortalFlowAdaptor(PortalFlowService *service)
        : QDBusAbstractAdaptor(service), m_service(service)
    {
        // OAuth results travel only in replies to the owning caller. Directed
        // signals to arbitrary callers are blocked by the Sailjail bus proxy.
        setAutoRelaySignals(false);
    }
public slots:
    void openFlow(const QString &requestId, const QString &url,
                  const QString &purpose, const QString &callbackUrl)
    { m_service->openFlow(requestId, url, purpose, callbackUrl); }
    void openFlowWithHeading(const QString &requestId, const QString &url,
                             const QString &purpose, const QString &callbackUrl,
                             const QString &heading)
    { m_service->openFlow(requestId, url, purpose, callbackUrl, heading); }
    void closeFlow(const QString &requestId) { m_service->closeFlow(requestId); }
    QVariantMap waitForEvent(const QString &requestId)
    { return m_service->waitForEvent(requestId); }
private:
    PortalFlowService *m_service;
};

PortalFlowService::PortalFlowService(QObject *parent)
    : QObject(parent)
    , m_watch(this)
{
    new PortalFlowAdaptor(this);
    auto bus = QDBusConnection::sessionBus();
    m_watch.setConnection(bus);
    m_watch.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(&m_watch, &QDBusServiceWatcher::serviceUnregistered,
            this, &PortalFlowService::ownerGone);
    m_registered = bus.registerObject(QStringLiteral("/flows"), this);
}

bool PortalFlowService::registered() const { return m_registered; }

bool PortalFlowService::webUrl(const QString &value) const
{
    const QUrl url(value, QUrl::StrictMode);
    return url.isValid() && !url.host().isEmpty() && url.userInfo().isEmpty()
            && (url.scheme() == QLatin1String("https") || url.scheme() == QLatin1String("http"));
}

QString PortalFlowService::ownedFlow(const QString &requestId) const
{
    if (!calledFromDBus()) return QString();
    for (auto it = m_flows.constBegin(); it != m_flows.constEnd(); ++it) {
        if (it->owner == message().service() && it->requestId == requestId) return it.key();
    }
    return QString();
}

void PortalFlowService::openFlow(const QString &requestId, const QString &url,
                               const QString &purpose, const QString &callbackUrl,
                               const QString &heading)
{
    if (!calledFromDBus()) return;
    const QUrl callback(callbackUrl, QUrl::StrictMode);
    if (requestId.isEmpty() || requestId.size() > 128 || heading.size() > 256 || !webUrl(url)
            || (purpose != QLatin1String("terms") && purpose != QLatin1String("authentication"))
            || (!callbackUrl.isEmpty() && (!callback.isValid() || callback.isRelative()
                                         || !callback.userInfo().isEmpty()))) {
        sendErrorReply(QDBusError::InvalidArgs, QStringLiteral("Invalid portal flow"));
        return;
    }
    if (!ownedFlow(requestId).isEmpty()) {
        sendErrorReply(QDBusError::InvalidArgs, QStringLiteral("Request already exists"));
        return;
    }
    int count = 0;
    for (const Flow &flow : m_flows) if (flow.owner == message().service()) ++count;
    if (count >= 8 || m_flows.size() >= 32) {
        sendErrorReply(QDBusError::LimitsExceeded, QStringLiteral("Too many portal flows"));
        return;
    }
    Flow flow;
    flow.owner = message().service();
    flow.requestId = requestId;
    flow.url = url;
    flow.purpose = purpose;
    flow.heading = heading;
    if (purpose == QLatin1String("authentication")) flow.callback = callback;
    const QString id = QUuid::createUuid().toString();
    flow.timer = new QTimer(this);
    flow.timer->setSingleShot(true);
    connect(flow.timer, &QTimer::timeout, this, [this, id]() {
        auto it = m_flows.find(id);
        if (it == m_flows.end()) return;
        if (it->finished) {
            remove(id);
        } else if (it->waiter.type() == QDBusMessage::MethodCallMessage) {
            // Re-arm below the normal D-Bus method timeout, without busy polling.
            QDBusConnection::sessionBus().send(it->waiter.createReply(
                        QVariantList() << QVariantMap()));
            it->waiter = QDBusMessage();
        }
    });
    m_flows.insert(id, flow);
    m_watch.addWatchedService(flow.owner);
    if (m_ready) emit openRequested(id, url, purpose, heading);
}

void PortalFlowService::closeFlow(const QString &requestId)
{
    const QString id = ownedFlow(requestId);
    if (id.isEmpty()) {
        if (calledFromDBus()) sendErrorReply(QDBusError::AccessDenied, QStringLiteral("Unknown owned flow"));
        return;
    }
    if (m_flows[id].closing) return;
    m_flows[id].closing = true;
    if (m_ready) emit closeRequested(id);
    else closed(id, QStringLiteral("completed"));
}

void PortalFlowService::setReady()
{
    if (m_ready) return;
    m_ready = true;
    const auto flows = m_flows;
    for (auto it = flows.constBegin(); it != flows.constEnd(); ++it)
        if (m_flows.contains(it.key()) && !m_flows[it.key()].closing) emit openRequested(it.key(), it->url, it->purpose, it->heading);
}

bool PortalFlowService::callbackMatches(const QString &id, const QString &value) const
{
    const auto it = m_flows.constFind(id);
    if (it == m_flows.constEnd() || it->finished || it->callback.isEmpty()) return false;
    const QUrl url(value, QUrl::StrictMode);
    const QUrl &callback = it->callback;
    if (!url.isValid() || !url.userInfo().isEmpty()
            || url.adjusted(QUrl::RemoveQuery | QUrl::RemoveFragment)
               != callback.adjusted(QUrl::RemoveQuery | QUrl::RemoveFragment)) return false;
    // Fixed query parameters in a registered redirect remain part of its
    // identity. The provider may append code/state/error parameters.
    const QUrlQuery query(url);
    for (const auto &item : QUrlQuery(callback).queryItems(QUrl::FullyEncoded))
        if (!query.allQueryItemValues(item.first, QUrl::FullyEncoded).contains(item.second)) return false;
    return callback.fragment().isEmpty() || url.fragment() == callback.fragment();
}

QVariantMap PortalFlowService::waitForEvent(const QString &requestId)
{
    const QString id = ownedFlow(requestId);
    if (id.isEmpty()) {
        if (calledFromDBus()) sendErrorReply(QDBusError::AccessDenied, QStringLiteral("Unknown owned flow"));
        return QVariantMap();
    }
    auto &flow = m_flows[id];
    if (flow.waiter.type() == QDBusMessage::MethodCallMessage) {
        sendErrorReply(QDBusError::LimitsExceeded, QStringLiteral("A wait is already pending"));
        return QVariantMap();
    }
    setDelayedReply(true);
    flow.waiter = message();
    if (flow.events.isEmpty()) flow.timer->start(20000);
    deliver(id);
    return QVariantMap();
}

bool PortalFlowService::pendingResults() const
{
    for (const Flow &flow : m_flows)
        if (flow.finished && !flow.events.isEmpty()) return true;
    return false;
}

void PortalFlowService::remove(const QString &id)
{
    auto it = m_flows.find(id);
    if (it == m_flows.end()) return;
    const QString owner = it->owner;
    it->timer->stop();
    it->timer->deleteLater();
    m_flows.erase(it);
    bool watched = false;
    for (const Flow &other : m_flows) if (other.owner == owner) watched = true;
    if (!watched) m_watch.removeWatchedService(owner);
    emit pendingResultsChanged();
}

void PortalFlowService::deliver(const QString &id)
{
    auto it = m_flows.find(id);
    if (it == m_flows.end() || it->events.isEmpty()
            || it->waiter.type() != QDBusMessage::MethodCallMessage) return;
    const QVariantMap event = it->events.takeFirst();
    it->eventBytes -= event.value(QStringLiteral("url")).toString().size() * 2
            + event.value(QStringLiteral("title")).toString().size() * 2;
    QDBusConnection::sessionBus().send(it->waiter.createReply(QVariantList() << event));
    it->waiter = QDBusMessage();
    if (!it->finished) it->timer->stop();
    if (it->finished && it->events.isEmpty()) remove(id);
}

void PortalFlowService::enqueue(const QString &id, const QVariantMap &event)
{
    auto it = m_flows.find(id);
    if (it == m_flows.end() || it->closing || it->owner.isEmpty()) return;
    const int bytes = event.value(QStringLiteral("url")).toString().size() * 2
            + event.value(QStringLiteral("title")).toString().size() * 2;
    // Do not coalesce URLs: an intermediate URL can be the OAuth callback.
    if (it->events.size() >= 64 || bytes > 1048576 - it->eventBytes) {
        it->events.clear();
        it->eventBytes = 0;
        it->events.append(QVariantMap{{QStringLiteral("event"), QStringLiteral("failed")},
                                     {QStringLiteral("reason"), QStringLiteral("overflow")}});
        it->closing = true;
        deliver(id);
        if (m_ready) emit closeRequested(id);
        else closed(id, QStringLiteral("failed"));
        return;
    }
    it->events.append(event);
    it->eventBytes += bytes;
    deliver(id);
}

void PortalFlowService::updated(const QString &id, const QString &url, const QString &title)
{
    enqueue(id, QVariantMap{{QStringLiteral("event"), QStringLiteral("updated")},
                           {QStringLiteral("url"), url}, {QStringLiteral("title"), title}});
}

void PortalFlowService::loaded(const QString &id, bool secure)
{
    enqueue(id, QVariantMap{{QStringLiteral("event"), QStringLiteral("loaded")},
                           {QStringLiteral("secure"), secure}});
}

void PortalFlowService::failed(const QString &id, const QString &reason)
{
    enqueue(id, QVariantMap{{QStringLiteral("event"), QStringLiteral("failed")},
                           {QStringLiteral("reason"), reason}});
}

void PortalFlowService::closed(const QString &id, const QString &reason)
{
    auto it = m_flows.find(id);
    if (it == m_flows.end() || it->finished) return;
    if (it->owner.isEmpty()) {
        remove(id);
        return;
    }
    it->closing = true;
    it->finished = true;
    it->events.append(QVariantMap{{QStringLiteral("event"), QStringLiteral("closed")},
                                 {QStringLiteral("reason"), reason}});
    // Give an owner between waits time to collect the final result. An absent
    // caller must not keep the hidden portal alive indefinitely.
    it->timer->start(5000);
    deliver(id);
    emit pendingResultsChanged();
}

void PortalFlowService::ownerGone(const QString &owner)
{
    const auto ids = m_flows.keys();
    for (const QString &id : ids) {
        auto it = m_flows.find(id);
        if (it != m_flows.end() && it->owner == owner) {
            const bool finished = it->finished;
            remove(id);
            if (m_ready && !finished) emit closeRequested(id);
        }
    }
    m_watch.removeWatchedService(owner);
}

#include "portalflowservice.moc"
