/****************************************************************************
**
** Copyright (c) 2015 Jolla Ltd.
** Contact: Siteshwar Vashisht <siteshwar@gmail.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef HOSTEDTABMODEL_H
#define HOSTEDTABMODEL_H

#include "declarativetabmodel.h"

#include <QHash>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class PersistentRuntimeTabState
{
public:
    PersistentRuntimeTabState();
    PersistentRuntimeTabState(quint64 runtimeId, int persistentId,
                              const QString &url, const QString &title,
                              bool selected, bool discarded,
                              quint64 locationRevision);

    quint64 runtimeId() const;
    int persistentId() const;
    QString url() const;
    QString title() const;
    bool selected() const;
    bool discarded() const;
    quint64 locationRevision() const;

private:
    quint64 m_runtimeId;
    int m_persistentId;
    QString m_url;
    QString m_title;
    bool m_selected;
    bool m_discarded;
    quint64 m_locationRevision;
};

class DeclarativeWebContainer;

class HostedTabModel : public DeclarativeTabModel
{
    Q_OBJECT

protected:
    void createTab(const Tab &tab) override;
    void removeTab(int tabId) override;
    void updateThumbPath(int tabId, const QString &path) override;

private slots:
    void saveActiveTab() const;
    void saveTabOrder() const;
    void tabsAvailable(const QList<Tab> &tabs);
    void persistentTabRestoreBatchAvailable(const PersistentTabRestoreBatch &batch);
    void rememberReservedRuntimeTab(const QString &url, const QString &persistentId,
                                    bool fromExternal);
    void expireRuntimeTabReservations();
    void expireRuntimeTraversals();

public:
    HostedTabModel(int nextTabId, bool persistent, DeclarativeWebContainer *webContainer = nullptr);
    ~HostedTabModel();

    const PersistentTabRestoreBatch &restoreBatch() const;
    int persistentIdForRuntimeId(quint64 runtimeId) const;
    QString runtimeIdForPersistentId(int persistentId) const;
    void applyRuntimeSnapshot(const QList<PersistentRuntimeTabState> &tabs);
    Q_INVOKABLE QString reserveRuntimeTab(const QString &url,
                                          const QString &title = QString());
    Q_INVOKABLE bool cancelRuntimeTabReservation(const QString &persistentId);
    Q_INVOKABLE QString runtimeIdForPersistentId(const QString &persistentId) const;
    Q_INVOKABLE QString persistentIdAt(int index) const;
    Q_INVOKABLE bool runtimeDesktopMode(const QString &persistentId) const;
    Q_INVOKABLE bool setRuntimeDesktopMode(const QString &persistentId,
                                           bool desktopMode);
    Q_INVOKABLE bool runtimeGoBack(const QString &persistentId);
    Q_INVOKABLE bool runtimeGoForward(const QString &persistentId);
    Q_INVOKABLE QVariantList takePendingRuntimeNewTabs();
    Q_INVOKABLE bool consumeConfirmedRuntimeTraversal(
            const QString &runtimeId, const QString &locationRevision);
    Q_INVOKABLE QVariantMap runtimeRestoreBatch() const;
    Q_INVOKABLE void applyRuntimeSnapshot(const QVariantList &tabs,
                                          const QString &selectedTabId);

signals:
    void restoreBatchReady(PersistentTabRestoreBatch batch);
    void runtimeTabAdopted(const QString &runtimeId, const QString &persistentId);
    void authoritativeActiveTabChanged(const QString &persistentId);
    void authoritativeSnapshotApplied();
    void runtimeHistoryTraversalConfirmed(const QString &runtimeId,
                                          const QString &locationRevision);
    void runtimeTabReservationRejected(const QString &persistentId);

private:
    void setRestoredTabs(const QList<Tab> &tabs, int activePersistentId);
    void scheduleRuntimeTabReservationExpiry();
    void scheduleRuntimeTraversalExpiry();
    void removePendingRuntimeNewTab(int persistentId);
    void restoreDesktopModes();
    void saveDesktopModes() const;

    struct PendingRuntimeTraversal {
        int direction;
        QString sourceLocation;
        QString targetLocation;
        quint64 baseRevision;
        qint64 deadline;
    };

    struct PendingRuntimeNewTab {
        QString url;
        int persistentId;
        bool fromExternal;
    };

    const bool m_persistent;
    bool m_runtimeSnapshotApplied = false;
    PersistentTabRestoreBatch m_restoreBatch;
    QHash<quint64, int> m_runtimePersistentIds;
    QHash<int, Tab> m_reservedRuntimeTabs;
    QHash<int, qint64> m_reservedRuntimeTabDeadlines;
    QTimer m_runtimeTabReservationTimer;
    QList<PendingRuntimeNewTab> m_pendingRuntimeNewTabs;
    QHash<int, quint64> m_runtimeLocationRevisions;
    QHash<int, PendingRuntimeTraversal> m_pendingRuntimeTraversals;
    QTimer m_runtimeTraversalTimer;
    QHash<quint64, quint64> m_confirmedRuntimeTraversals;

    friend class tst_persistenttabmodel;
    friend class tst_hostedtabmodel;
};

#endif // HOSTEDTABMODEL_H
