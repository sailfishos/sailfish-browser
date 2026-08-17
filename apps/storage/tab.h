/****************************************************************************
**
** Copyright (c) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef TAB_H
#define TAB_H

#include <QString>
#include <QDebug>
#include <QList>
#include <QMetaType>

class Tab
{
public:
    explicit Tab();
    explicit Tab(int tabId, const QString &url, const QString &title, const QString &thumbPath, bool hidden);

    int tabId() const;
    void setTabId(int tabId);

    void setRequestedUrl(const QString &url);
    QString requestedUrl() const;

    QString url() const;
    void setUrl(const QString &url);

    bool hasResolvedUrl() const;

    QString thumbnailPath() const;
    void setThumbnailPath(const QString &thumbnailPath);

    QString title() const;
    void setTitle(const QString &title);

    bool desktopMode() const;
    void setDesktopMode(bool desktopMode);

    void setBrowsingContext(uintptr_t browsingContext);
    uintptr_t browsingContext() const;

    void setParentId(uint32_t parentId);
    uint32_t parentId() const;

    bool isValid() const;
    bool hidden() const;

    bool operator==(const Tab &other) const;
    bool operator!=(const Tab &other) const;

private:
    int m_tabId;
    QString m_requestedUrl;
    QString m_url;
    QString m_title;
    QString m_thumbPath;
    bool m_desktopMode;
    bool m_hidden;
    uintptr_t m_browsingContext;
    uint32_t m_parentId;
};

Q_DECLARE_METATYPE(Tab)

class PersistentTabHistoryEntry
{
public:
    PersistentTabHistoryEntry();
    PersistentTabHistoryEntry(const QString &url, const QString &title);

    QString url() const;
    QString title() const;

private:
    QString m_url;
    QString m_title;
};

class PersistentTabRestoreData
{
public:
    PersistentTabRestoreData();
    explicit PersistentTabRestoreData(int persistentId);

    int persistentId() const;
    const Tab &tab() const;
    void setTab(const Tab &tab);
    const QList<PersistentTabHistoryEntry> &history() const;
    void addHistoryEntry(const PersistentTabHistoryEntry &entry);
    int selectedHistoryIndex() const;
    void setSelectedHistoryIndex(int index);

private:
    int m_persistentId;
    Tab m_tab;
    QList<PersistentTabHistoryEntry> m_history;
    int m_selectedHistoryIndex;
};

class PersistentTabRestoreBatch
{
public:
    PersistentTabRestoreBatch();
    PersistentTabRestoreBatch(const QList<PersistentTabRestoreData> &tabs,
                              int activePersistentId);

    const QList<PersistentTabRestoreData> &tabs() const;
    int activePersistentId() const;

private:
    QList<PersistentTabRestoreData> m_tabs;
    int m_activePersistentId;
};

Q_DECLARE_METATYPE(PersistentTabHistoryEntry)
Q_DECLARE_METATYPE(PersistentTabRestoreData)
Q_DECLARE_METATYPE(PersistentTabRestoreBatch)

QDebug operator<<(QDebug, const Tab *);

#endif // TAB_H
