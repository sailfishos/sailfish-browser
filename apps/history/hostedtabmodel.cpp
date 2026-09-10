/****************************************************************************
**
** Copyright (c) 2013 - 2021 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
** Contact: Siteshwar Vashisht <siteshwar@gmail.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "declarativewebcontainer.h"
#include "hostedtabmodel.h"
#include "dbmanager.h"

#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QSet>
#include <QStringList>

namespace {

const int RuntimeTabReservationTimeout = 5000;
const int RuntimeTraversalTimeout = 5000;

}

PersistentRuntimeTabState::PersistentRuntimeTabState()
    : m_runtimeId(0)
    , m_persistentId(0)
    , m_selected(false)
    , m_discarded(false)
    , m_locationRevision(0)
{
}

PersistentRuntimeTabState::PersistentRuntimeTabState(
        quint64 runtimeId, int persistentId, const QString &url,
        const QString &title, bool selected, bool discarded,
        quint64 locationRevision)
    : m_runtimeId(runtimeId)
    , m_persistentId(persistentId)
    , m_url(url)
    , m_title(title)
    , m_selected(selected)
    , m_discarded(discarded)
    , m_locationRevision(locationRevision)
{
}

quint64 PersistentRuntimeTabState::runtimeId() const
{
    return m_runtimeId;
}

int PersistentRuntimeTabState::persistentId() const
{
    return m_persistentId;
}

QString PersistentRuntimeTabState::url() const
{
    return m_url;
}

QString PersistentRuntimeTabState::title() const
{
    return m_title;
}

bool PersistentRuntimeTabState::selected() const
{
    return m_selected;
}

bool PersistentRuntimeTabState::discarded() const
{
    return m_discarded;
}

quint64 PersistentRuntimeTabState::locationRevision() const
{
    return m_locationRevision;
}

HostedTabModel::HostedTabModel(int nextTabId, bool persistent, DeclarativeWebContainer *webContainer)
    : DeclarativeTabModel(nextTabId, webContainer)
    , m_persistent(persistent)
{
    if (m_persistent) {
        connect(DBManager::instance(), &DBManager::tabsAvailable,
                this, &HostedTabModel::tabsAvailable);
        connect(DBManager::instance(), &DBManager::persistentTabRestoreBatchAvailable,
                this, &HostedTabModel::persistentTabRestoreBatchAvailable);
    }
    connect(this, &HostedTabModel::tabClosed,
            this, &HostedTabModel::saveTabOrder);
    connect(this, &HostedTabModel::tabClosed,
            this, &HostedTabModel::saveDesktopModes);
    connect(this, &HostedTabModel::runtimeNewTabRequested,
            this, &HostedTabModel::rememberReservedRuntimeTab);
    m_runtimeTabReservationTimer.setSingleShot(true);
    connect(&m_runtimeTabReservationTimer, &QTimer::timeout,
            this, &HostedTabModel::expireRuntimeTabReservations);
    m_runtimeTraversalTimer.setSingleShot(true);
    connect(&m_runtimeTraversalTimer, &QTimer::timeout,
            this, &HostedTabModel::expireRuntimeTraversals);

    if (m_persistent) {
        DBManager::instance()->getPersistentTabRestoreBatch();
    } else {
        m_loaded = true;
        QMetaObject::invokeMethod(this, "loadedChanged", Qt::QueuedConnection);
    }
}

HostedTabModel::~HostedTabModel()
{
}

void HostedTabModel::tabsAvailable(const QList<Tab> &tabs)
{
    if (!m_persistent) {
        return;
    }

    int activePersistentId = DBManager::instance()->getSetting("activeTabId").toInt();
    bool activePersistentIdFound = false;
    for (const Tab &tab : tabs) {
        if (tab.tabId() == activePersistentId) {
            activePersistentIdFound = true;
            break;
        }
    }
    if (!activePersistentIdFound && !tabs.isEmpty()) {
        activePersistentId = tabs.first().tabId();
    }
    setRestoredTabs(tabs, activePersistentId);
}

void HostedTabModel::persistentTabRestoreBatchAvailable(
        const PersistentTabRestoreBatch &batch)
{
    if (!m_persistent) {
        return;
    }

    QList<Tab> tabs;
    for (const PersistentTabRestoreData &restoreData : batch.tabs()) {
        tabs.append(restoreData.tab());
    }

    m_restoreBatch = batch;
    setRestoredTabs(tabs, batch.activePersistentId());
    emit restoreBatchReady(batch);
}

void HostedTabModel::setRestoredTabs(const QList<Tab> &tabs,
                                     int activePersistentId)
{
    // Later database reads must not replace Gecko's live tab list with
    // durable reservations that have not become runtime tabs.
    if (m_runtimeSnapshotApplied) {
        return;
    }
    beginResetModel();
    int oldCount = count();
    const int oldActiveTabId = m_activeTabId;

    m_tabs = tabs;
    restoreDesktopModes();
    m_activeTabId = findTabIndex(activePersistentId) >= 0
            ? activePersistentId : (m_tabs.isEmpty() ? 0 : m_tabs.first().tabId());

    endResetModel();

    if (count() != oldCount) {
        emit countChanged();
    }
    if (m_activeTabId != oldActiveTabId) {
        emit activeTabIndexChanged();
    }

    int maxTabId(0);
    for (const Tab &tab : tabs) {
        if (maxTabId < tab.tabId()) {
            maxTabId = tab.tabId();
        }
    }

    if (m_nextTabId != maxTabId + 1) {
        m_nextTabId = maxTabId + 1;
    }

    // Startup should be synced to this.
    if (!m_loaded) {
        m_loaded = true;
        emit loadedChanged();
    }

    connect(this, &HostedTabModel::activeTabIndexChanged,
            this, &HostedTabModel::saveActiveTab, Qt::UniqueConnection);
}

const PersistentTabRestoreBatch &HostedTabModel::restoreBatch() const
{
    return m_restoreBatch;
}

int HostedTabModel::persistentIdForRuntimeId(quint64 runtimeId) const
{
    return m_runtimePersistentIds.value(runtimeId);
}

QString HostedTabModel::runtimeIdForPersistentId(int persistentId) const
{
    for (auto it = m_runtimePersistentIds.constBegin();
         it != m_runtimePersistentIds.constEnd(); ++it) {
        if (it.value() == persistentId) {
            return QString::number(it.key());
        }
    }
    return QString();
}

QString HostedTabModel::runtimeIdForPersistentId(const QString &persistentId) const
{
    bool ok = false;
    const int id = persistentId.toInt(&ok);
    return ok ? runtimeIdForPersistentId(id) : QString();
}

QString HostedTabModel::persistentIdAt(int index) const
{
    return index >= 0 && index < m_tabs.count()
            ? QString::number(m_tabs.at(index).tabId()) : QString();
}

bool HostedTabModel::runtimeDesktopMode(const QString &persistentId) const
{
    bool ok = false;
    const int id = persistentId.toInt(&ok);
    const int tabIndex = ok ? findTabIndex(id) : -1;
    return tabIndex >= 0 && m_tabs.at(tabIndex).desktopMode();
}

bool HostedTabModel::setRuntimeDesktopMode(const QString &persistentId,
                                                bool desktopMode)
{
    bool ok = false;
    const int id = persistentId.toInt(&ok);
    const int tabIndex = ok ? findTabIndex(id) : -1;
    if (tabIndex < 0) {
        return false;
    }

    if (m_tabs.at(tabIndex).desktopMode() != desktopMode) {
        m_tabs[tabIndex].setDesktopMode(desktopMode);
        QVector<int> roles;
        roles << DesktopModeRole;
        emit dataChanged(index(tabIndex, 0), index(tabIndex, 0), roles);
        saveDesktopModes();
    }
    return true;
}

QString HostedTabModel::reserveRuntimeTab(const QString &url, const QString &title)
{
    Q_UNUSED(url)
    Q_UNUSED(title)
    const int persistentId = m_nextTabId++;
    Tab tab(persistentId, QString(), QString(), QString(), false);
    tab.setRequestedUrl(QString());
    m_reservedRuntimeTabs.insert(persistentId, tab);
    m_reservedRuntimeTabDeadlines.insert(
                persistentId,
                QDateTime::currentMSecsSinceEpoch() + RuntimeTabReservationTimeout);
    scheduleRuntimeTabReservationExpiry();
    createTab(tab);
    return QString::number(persistentId);
}

bool HostedTabModel::cancelRuntimeTabReservation(const QString &persistentId)
{
    bool ok = false;
    const int id = persistentId.toInt(&ok);
    if (!ok || contains(id) || !m_reservedRuntimeTabs.contains(id)) {
        return false;
    }

    removeTab(id);
    m_reservedRuntimeTabs.remove(id);
    m_reservedRuntimeTabDeadlines.remove(id);
    removePendingRuntimeNewTab(id);
    scheduleRuntimeTabReservationExpiry();
    emit runtimeTabReservationRejected(QString::number(id));
    return true;
}

bool HostedTabModel::runtimeGoBack(const QString &persistentId)
{
    bool ok = false;
    const int id = persistentId.toInt(&ok);
    if (!ok || !contains(id) || m_pendingRuntimeTraversals.contains(id)) {
        return false;
    }

    if (!m_persistent) {
        return true;
    }

    const QString targetLocation = DBManager::instance()->peekBackTarget(id);
    if (targetLocation.isEmpty()) {
        return false;
    }

    PendingRuntimeTraversal traversal;
    traversal.direction = -1;
    traversal.sourceLocation = url(id);
    traversal.targetLocation = targetLocation;
    traversal.baseRevision = m_runtimeLocationRevisions.value(id);
    traversal.deadline = QDateTime::currentMSecsSinceEpoch() + RuntimeTraversalTimeout;
    m_pendingRuntimeTraversals.insert(id, traversal);
    scheduleRuntimeTraversalExpiry();
    return true;
}

bool HostedTabModel::runtimeGoForward(const QString &persistentId)
{
    bool ok = false;
    const int id = persistentId.toInt(&ok);
    if (!ok || !contains(id) || m_pendingRuntimeTraversals.contains(id)) {
        return false;
    }

    if (!m_persistent) {
        return true;
    }

    const QString targetLocation = DBManager::instance()->peekForwardTarget(id);
    if (targetLocation.isEmpty()) {
        return false;
    }

    PendingRuntimeTraversal traversal;
    traversal.direction = 1;
    traversal.sourceLocation = url(id);
    traversal.targetLocation = targetLocation;
    traversal.baseRevision = m_runtimeLocationRevisions.value(id);
    traversal.deadline = QDateTime::currentMSecsSinceEpoch() + RuntimeTraversalTimeout;
    m_pendingRuntimeTraversals.insert(id, traversal);
    scheduleRuntimeTraversalExpiry();
    return true;
}

bool HostedTabModel::consumeConfirmedRuntimeTraversal(
        const QString &runtimeId, const QString &locationRevision)
{
    bool runtimeIdOk = false;
    bool revisionOk = false;
    const quint64 runtime = runtimeId.toULongLong(&runtimeIdOk);
    const quint64 revision = locationRevision.toULongLong(&revisionOk);
    if (!runtimeIdOk || !revisionOk
            || !m_confirmedRuntimeTraversals.contains(runtime)
            || m_confirmedRuntimeTraversals.value(runtime) != revision) {
        return false;
    }
    m_confirmedRuntimeTraversals.remove(runtime);
    return true;
}

void HostedTabModel::rememberReservedRuntimeTab(const QString &url,
                                                    const QString &persistentId,
                                                    bool fromExternal)
{
    bool ok = false;
    const int id = persistentId.toInt(&ok);
    if (ok && id > 0) {
        Q_UNUSED(url)
        Tab tab(id, QString(), QString(), QString(), false);
        tab.setRequestedUrl(QString());
        m_reservedRuntimeTabs.insert(id, tab);
        m_reservedRuntimeTabDeadlines.insert(
                    id,
                    QDateTime::currentMSecsSinceEpoch() + RuntimeTabReservationTimeout);
        PendingRuntimeNewTab pendingCommand;
        pendingCommand.url = url;
        pendingCommand.persistentId = id;
        pendingCommand.fromExternal = fromExternal;
        m_pendingRuntimeNewTabs.append(pendingCommand);
        scheduleRuntimeTabReservationExpiry();
    }
}

QVariantList HostedTabModel::takePendingRuntimeNewTabs()
{
    QVariantList commands;
    for (const PendingRuntimeNewTab &pendingCommand : m_pendingRuntimeNewTabs) {
        QVariantMap command;
        command.insert(QStringLiteral("url"), pendingCommand.url);
        command.insert(QStringLiteral("persistentId"),
                       QString::number(pendingCommand.persistentId));
        command.insert(QStringLiteral("fromExternal"), pendingCommand.fromExternal);
        commands.append(command);
    }
    m_pendingRuntimeNewTabs.clear();
    return commands;
}

void HostedTabModel::removePendingRuntimeNewTab(int persistentId)
{
    for (int index = m_pendingRuntimeNewTabs.count() - 1; index >= 0; --index) {
        if (m_pendingRuntimeNewTabs.at(index).persistentId == persistentId) {
            m_pendingRuntimeNewTabs.removeAt(index);
        }
    }
}

void HostedTabModel::scheduleRuntimeTabReservationExpiry()
{
    if (m_reservedRuntimeTabDeadlines.isEmpty()) {
        m_runtimeTabReservationTimer.stop();
        return;
    }

    qint64 nextDeadline = -1;
    for (qint64 deadline : m_reservedRuntimeTabDeadlines) {
        if (nextDeadline < 0 || deadline < nextDeadline) {
            nextDeadline = deadline;
        }
    }

    const qint64 remaining = nextDeadline - QDateTime::currentMSecsSinceEpoch();
    m_runtimeTabReservationTimer.start(static_cast<int>(qMax<qint64>(1, remaining)));
}

void HostedTabModel::expireRuntimeTabReservations()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QList<int> expiredReservations;
    for (auto it = m_reservedRuntimeTabDeadlines.constBegin();
         it != m_reservedRuntimeTabDeadlines.constEnd(); ++it) {
        if (it.value() <= now) {
            expiredReservations.append(it.key());
        }
    }

    for (int persistentId : expiredReservations) {
        cancelRuntimeTabReservation(QString::number(persistentId));
    }
    scheduleRuntimeTabReservationExpiry();
}

void HostedTabModel::scheduleRuntimeTraversalExpiry()
{
    if (m_pendingRuntimeTraversals.isEmpty()) {
        m_runtimeTraversalTimer.stop();
        return;
    }

    qint64 nextDeadline = -1;
    for (const PendingRuntimeTraversal &traversal : m_pendingRuntimeTraversals) {
        if (nextDeadline < 0 || traversal.deadline < nextDeadline) {
            nextDeadline = traversal.deadline;
        }
    }

    const qint64 remaining = nextDeadline - QDateTime::currentMSecsSinceEpoch();
    m_runtimeTraversalTimer.start(static_cast<int>(qMax<qint64>(1, remaining)));
}

void HostedTabModel::expireRuntimeTraversals()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QList<int> expiredTraversals;
    for (auto it = m_pendingRuntimeTraversals.constBegin();
         it != m_pendingRuntimeTraversals.constEnd(); ++it) {
        if (it->deadline <= now) {
            expiredTraversals.append(it.key());
        }
    }

    for (int persistentId : expiredTraversals) {
        m_pendingRuntimeTraversals.remove(persistentId);
    }
    scheduleRuntimeTraversalExpiry();
}

QVariantMap HostedTabModel::runtimeRestoreBatch() const
{
    QVariantList runtimeTabs;
    int selectedIndex = -1;
    const QList<PersistentTabRestoreData> restoredTabs = m_restoreBatch.tabs();
    for (int i = 0; i < restoredTabs.count(); ++i) {
        const PersistentTabRestoreData &restoreData = restoredTabs.at(i);
        QVariantList history;
        for (const PersistentTabHistoryEntry &entry : restoreData.history()) {
            QVariantMap historyEntry;
            historyEntry.insert(QStringLiteral("location"), entry.url());
            historyEntry.insert(QStringLiteral("title"), entry.title());
            history.append(historyEntry);
        }

        QVariantMap runtimeTab;
        runtimeTab.insert(QStringLiteral("persistentId"),
                          QString::number(restoreData.persistentId()));
        runtimeTab.insert(QStringLiteral("history"), history);
        runtimeTab.insert(QStringLiteral("selectedHistoryIndex"),
                          restoreData.selectedHistoryIndex());
        runtimeTabs.append(runtimeTab);
        if (restoreData.persistentId() == m_restoreBatch.activePersistentId()) {
            selectedIndex = i;
        }
    }

    QVariantMap batch;
    batch.insert(QStringLiteral("tabs"), runtimeTabs);
    batch.insert(QStringLiteral("selectedIndex"), selectedIndex);
    batch.insert(QStringLiteral("activePersistentId"),
                 QString::number(m_restoreBatch.activePersistentId()));
    return batch;
}

void HostedTabModel::applyRuntimeSnapshot(const QVariantList &runtimeTabs,
                                              const QString &selectedTabId)
{
    bool selectedTabIdOk = false;
    const quint64 selectedRuntimeId = selectedTabId.toULongLong(&selectedTabIdOk);
    QList<PersistentRuntimeTabState> tabs;
    for (const QVariant &runtimeTabValue : runtimeTabs) {
        const QVariantMap runtimeTab = runtimeTabValue.toMap();
        bool runtimeIdOk = false;
        bool locationRevisionOk = false;
        const quint64 runtimeId = runtimeTab.value(QStringLiteral("tabId"))
                .toString().toULongLong(&runtimeIdOk);
        const quint64 locationRevision = runtimeTab.value(QStringLiteral("locationRevision"))
                .toString().toULongLong(&locationRevisionOk);
        if (!runtimeIdOk) {
            qWarning() << "Ignoring runtime tab with invalid runtimeId";
            continue;
        }
        tabs.append(PersistentRuntimeTabState(
                        runtimeId,
                        runtimeTab.value(QStringLiteral("persistentId")).toInt(),
                        runtimeTab.value(QStringLiteral("location")).toString(),
                        runtimeTab.value(QStringLiteral("title")).toString(),
                        selectedTabIdOk && runtimeId == selectedRuntimeId,
                        runtimeTab.value(QStringLiteral("discarded")).toBool(),
                        locationRevisionOk ? locationRevision : 0));
    }
    applyRuntimeSnapshot(tabs);
}

void HostedTabModel::applyRuntimeSnapshot(
        const QList<PersistentRuntimeTabState> &runtimeTabs)
{
    m_runtimeSnapshotApplied = true;
    QHash<int, Tab> oldTabs;
    for (const Tab &tab : m_tabs) {
        oldTabs.insert(tab.tabId(), tab);
    }

    QList<Tab> newTabs;
    QHash<quint64, int> runtimePersistentIds;
    QSet<quint64> seenRuntimeIds;
    QSet<int> seenPersistentIds;
    QList<QPair<quint64, int> > adoptedTabs;
    QList<QPair<quint64, quint64> > confirmedTraversals;
    int activePersistentId = 0;

    for (const PersistentRuntimeTabState &runtimeTab : runtimeTabs) {
        if (!runtimeTab.runtimeId() || seenRuntimeIds.contains(runtimeTab.runtimeId())) {
            qWarning() << "Ignoring invalid or duplicate runtime tab id"
                       << runtimeTab.runtimeId();
            continue;
        }
        seenRuntimeIds.insert(runtimeTab.runtimeId());

        int persistentId = runtimeTab.persistentId();
        if (persistentId <= 0) {
            persistentId = m_runtimePersistentIds.value(runtimeTab.runtimeId());
        }
        if (persistentId <= 0) {
            persistentId = m_nextTabId++;
            adoptedTabs.append(qMakePair(runtimeTab.runtimeId(), persistentId));
        }
        if (seenPersistentIds.contains(persistentId)) {
            qWarning() << "Ignoring duplicate persistent tab id" << persistentId;
            continue;
        }
        seenPersistentIds.insert(persistentId);
        runtimePersistentIds.insert(runtimeTab.runtimeId(), persistentId);
        m_nextTabId = qMax(m_nextTabId, persistentId + 1);

        const bool existed = oldTabs.contains(persistentId)
                || m_reservedRuntimeTabs.contains(persistentId);
        const Tab oldTab = oldTabs.contains(persistentId)
                ? oldTabs.value(persistentId)
                : m_reservedRuntimeTabs.value(persistentId);
        const bool locationChanged = existed && oldTab.url() != runtimeTab.url();
        // Keep the last accepted thumbnail while the same location is being
        // restored or reloaded.  A newer capture replaces it once ready; an
        // inactive lazy-restored tab may otherwise never get that capture.
        const bool invalidateThumbnail = locationChanged;
        const QString thumbnail = existed && !invalidateThumbnail
                ? oldTab.thumbnailPath() : QString();
        if (m_persistent && invalidateThumbnail && !oldTab.thumbnailPath().isEmpty()) {
            QFile::remove(oldTab.thumbnailPath());
        }
        Tab tab(persistentId, runtimeTab.url(), runtimeTab.title(), thumbnail, false);
        tab.setRequestedUrl(QString());
        if (existed) {
            tab.setDesktopMode(oldTab.desktopMode());
        }
        newTabs.append(tab);

        // Confirmation is consumable only until the next complete snapshot.
        if (m_confirmedRuntimeTraversals.contains(runtimeTab.runtimeId())) {
            m_confirmedRuntimeTraversals.remove(runtimeTab.runtimeId());
        }

        bool confirmedTraversal = false;
        bool persistCommittedNavigation = false;
        bool awaitingTraversal = false;
        bool awaitingDifferentLocation = false;
        auto traversalIt = m_pendingRuntimeTraversals.find(persistentId);
        if (traversalIt != m_pendingRuntimeTraversals.end()) {
            const bool committedLocation =
                    runtimeTab.locationRevision() > traversalIt->baseRevision;
            const bool reachedTarget = committedLocation
                    && runtimeTab.url() == traversalIt->targetLocation;
            const bool reachedDifferentLocation = committedLocation
                    && runtimeTab.url() != traversalIt->sourceLocation;
            if (reachedTarget || reachedDifferentLocation) {
                const PendingRuntimeTraversal traversal = traversalIt.value();
                m_pendingRuntimeTraversals.erase(traversalIt);
                if (reachedTarget) {
                    const QString movedTarget = traversal.direction < 0
                            ? DBManager::instance()->goBackTarget(persistentId)
                            : DBManager::instance()->goForwardTarget(persistentId);
                    if (movedTarget == traversal.targetLocation) {
                        confirmedTraversal = true;
                        m_confirmedRuntimeTraversals.insert(
                                    runtimeTab.runtimeId(), runtimeTab.locationRevision());
                        confirmedTraversals.append(qMakePair(
                                                       runtimeTab.runtimeId(),
                                                       runtimeTab.locationRevision()));
                    } else {
                        persistCommittedNavigation = true;
                    }
                } else {
                    persistCommittedNavigation = true;
                }
            } else {
                awaitingTraversal = true;
                awaitingDifferentLocation =
                        runtimeTab.url() != traversalIt->sourceLocation;
                if (!awaitingDifferentLocation
                        && runtimeTab.locationRevision() > traversalIt->baseRevision) {
                    traversalIt->baseRevision = runtimeTab.locationRevision();
                }
            }
        }

        if (m_persistent) {
            if (!existed) {
                DBManager::instance()->createTab(tab);
            } else if (awaitingTraversal && awaitingDifferentLocation) {
                // A changed URL without a newer location revision is not a
                // committed navigation. Keep waiting without persisting it.
            } else if (persistCommittedNavigation && !runtimeTab.url().isEmpty()) {
                DBManager::instance()->navigateTo(persistentId, runtimeTab.url(),
                                                  runtimeTab.title(), thumbnail);
            } else if (!runtimeTab.url().isEmpty() && oldTab.url() != runtimeTab.url()) {
                if (confirmedTraversal) {
                    if (oldTab.title() != runtimeTab.title()) {
                        DBManager::instance()->updateTitle(persistentId, runtimeTab.url(),
                                                           runtimeTab.title());
                    }
                } else {
                    DBManager::instance()->navigateTo(persistentId, runtimeTab.url(),
                                                      runtimeTab.title(), thumbnail);
                }
            } else if (oldTab.title() != runtimeTab.title()) {
                DBManager::instance()->updateTitle(persistentId, runtimeTab.url(),
                                                   runtimeTab.title());
            }
        }

        if (runtimeTab.selected()) {
            activePersistentId = persistentId;
        }
        m_reservedRuntimeTabs.remove(persistentId);
        m_reservedRuntimeTabDeadlines.remove(persistentId);
        removePendingRuntimeNewTab(persistentId);
        m_runtimeLocationRevisions.insert(persistentId,
                                          runtimeTab.locationRevision());
    }

    scheduleRuntimeTabReservationExpiry();

    QList<int> removedPersistentIds;
    for (const Tab &oldTab : m_tabs) {
        if (!seenPersistentIds.contains(oldTab.tabId())) {
            removeTab(oldTab.tabId());
            m_pendingRuntimeTraversals.remove(oldTab.tabId());
            m_runtimeLocationRevisions.remove(oldTab.tabId());
            if (m_persistent && !oldTab.thumbnailPath().isEmpty()) {
                QFile::remove(oldTab.thumbnailPath());
            }
            removedPersistentIds.append(oldTab.tabId());
        }
    }
    scheduleRuntimeTraversalExpiry();

    if (!activePersistentId && !newTabs.isEmpty()) {
        activePersistentId = seenPersistentIds.contains(m_activeTabId)
                ? m_activeTabId : newTabs.first().tabId();
    }

    const int oldCount = count();
    const int oldActivePersistentId = m_activeTabId;
    beginResetModel();
    m_tabs = newTabs;
    m_activeTabId = activePersistentId;
    m_runtimePersistentIds = runtimePersistentIds;
    endResetModel();

    const QList<quint64> confirmedRuntimeIds = m_confirmedRuntimeTraversals.keys();
    for (quint64 runtimeId : confirmedRuntimeIds) {
        if (!seenRuntimeIds.contains(runtimeId)) {
            m_confirmedRuntimeTraversals.remove(runtimeId);
        }
    }

    if (oldCount != count()) {
        emit countChanged();
    }
    if (oldActivePersistentId != m_activeTabId) {
        emit activeTabIndexChanged();
        emit authoritativeActiveTabChanged(QString::number(m_activeTabId));
    }

    saveTabOrder();
    saveDesktopModes();
    for (const QPair<quint64, int> &adoptedTab : adoptedTabs) {
        emit runtimeTabAdopted(QString::number(adoptedTab.first),
                               QString::number(adoptedTab.second));
    }
    for (const QPair<quint64, quint64> &traversal : confirmedTraversals) {
        emit runtimeHistoryTraversalConfirmed(QString::number(traversal.first),
                                              QString::number(traversal.second));
    }
    for (int persistentId : removedPersistentIds) {
        emit tabClosed(persistentId);
    }
    emit authoritativeSnapshotApplied();
}

void HostedTabModel::createTab(const Tab &tab)
{
    if (!m_persistent) {
        return;
    }

    DBManager::instance()->createTab(
                Tab(tab.tabId(), QString(), QString(), QString(), false));
}

void HostedTabModel::removeTab(int tabId)
{
    if (!m_persistent) {
        return;
    }

    DBManager::instance()->removeTab(tabId);
}

void HostedTabModel::updateThumbPath(int tabId, const QString &path)
{
    if (!m_persistent) {
        return;
    }

    DBManager::instance()->updateThumbPath(tabId, path);
}

void HostedTabModel::saveActiveTab() const
{
    if (!m_persistent) {
        return;
    }

    DBManager::instance()->saveSetting("activeTabId", QString("%1").arg(m_activeTabId));
}

void HostedTabModel::saveTabOrder() const
{
    if (!m_persistent) {
        return;
    }

    QStringList persistentIds;
    for (const Tab &tab : m_tabs) {
        persistentIds.append(QString::number(tab.tabId()));
    }
    DBManager::instance()->saveSetting("tabOrder", persistentIds.join(QLatin1Char(',')));
}

void HostedTabModel::restoreDesktopModes()
{
    if (!m_persistent) {
        return;
    }

    const QStringList desktopModeIds = DBManager::instance()->getSetting(
                "desktopModeTabs").split(QLatin1Char(','), QString::SkipEmptyParts);
    QSet<int> enabledIds;
    for (const QString &desktopModeId : desktopModeIds) {
        bool ok = false;
        const int id = desktopModeId.toInt(&ok);
        if (ok) {
            enabledIds.insert(id);
        }
    }
    for (Tab &tab : m_tabs) {
        tab.setDesktopMode(enabledIds.contains(tab.tabId()));
    }
}

void HostedTabModel::saveDesktopModes() const
{
    if (!m_persistent) {
        return;
    }

    QStringList desktopModeIds;
    for (const Tab &tab : m_tabs) {
        if (tab.desktopMode()) {
            desktopModeIds.append(QString::number(tab.tabId()));
        }
    }
    const QString serializedModes = desktopModeIds.join(QLatin1Char(','));
    if (DBManager::instance()->getSetting("desktopModeTabs") != serializedModes) {
        DBManager::instance()->saveSetting("desktopModeTabs", serializedModes);
    }
}
