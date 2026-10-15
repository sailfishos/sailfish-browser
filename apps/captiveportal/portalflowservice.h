/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */
#ifndef PORTALFLOWSERVICE_H
#define PORTALFLOWSERVICE_H

#include <QObject>
#include <QDBusContext>
#include <QDBusServiceWatcher>
#include <QHash>
#include <QDBusMessage>
#include <QVariantMap>
#include <QTimer>
#include <QUrl>

// The D-Bus adaptor exposes only caller-owned flow methods. The invokables below
// belong to the trusted portal UI, never to a remote caller.
class PortalFlowService : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_PROPERTY(bool pendingResults READ pendingResults NOTIFY pendingResultsChanged)
public:
    explicit PortalFlowService(QObject *parent = nullptr);
    bool registered() const;
    void openFlow(const QString &requestId, const QString &url,
                  const QString &purpose, const QString &callbackUrl,
                  const QString &heading = QString());
    QVariantMap waitForEvent(const QString &requestId);
    bool pendingResults() const;
    void closeFlow(const QString &requestId);
    Q_INVOKABLE void setReady();
    Q_INVOKABLE bool callbackMatches(const QString &id, const QString &url) const;
    Q_INVOKABLE bool webUrl(const QString &url) const;
    Q_INVOKABLE void updated(const QString &id, const QString &url, const QString &title);
    Q_INVOKABLE void loaded(const QString &id, bool secure);
    Q_INVOKABLE void failed(const QString &id, const QString &reason);
    Q_INVOKABLE void closed(const QString &id, const QString &reason);

signals:
    void pendingResultsChanged();
    void openRequested(const QString &id, const QString &url, const QString &purpose, const QString &heading);
    void closeRequested(const QString &id);

private:
    struct Flow {
        QString owner;
        QString requestId;
        QString url;
        QString purpose;
        QString heading;
        QUrl callback;
        bool closing = false;
        bool finished = false;
        QList<QVariantMap> events;
        int eventBytes = 0;
        QDBusMessage waiter;
        QTimer *timer = nullptr;
    };
    void enqueue(const QString &id, const QVariantMap &event);
    void deliver(const QString &id);
    void remove(const QString &id);
    void ownerGone(const QString &owner);
    QString ownedFlow(const QString &requestId) const;
    QHash<QString, Flow> m_flows;
    QDBusServiceWatcher m_watch;
    bool m_ready = false;
    bool m_registered = false;
};

#endif
