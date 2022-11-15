/****************************************************************************
**
** Copyright (c) 2013 - 2019 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BROWSERSERVICE_H
#define BROWSERSERVICE_H

#include <QObject>
#include <QDBusContext>
#include <QStringList>

class BrowserUIServicePrivate;

class BrowserService : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    BrowserService(QObject * parent);
    bool registered() const;
    QString serviceName() const;

public slots:
    // these two calls are kept in this service for compatibility
    // but any calls that require the UI to be shown should be added to
    // the BrowserUIService service instead
    void openUrl(const QStringList &args);
    void activateNewTabView();

    void cancelTransfer(int transferId);
    void restartTransfer(int transferId);
    void dumpMemoryInfo(const QString &fileName);

signals:
    void openUrlRequested(const QString &url);
    void activateNewTabViewRequested();
    void cancelTransferRequested(int transferId);
    void restartTransferRequested(int transferId);
    void dumpMemoryInfoRequested(const QString &fileName);

private:
    bool isPrivileged() const;
    bool callerMatchesService(const QString &serviceName) const;

    bool m_registered;
};

class BrowserUIService : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    BrowserUIService(QObject * parent);
    bool registered() const;
    QString serviceName() const;

public slots:
    void openUrl(const QStringList &args);
    void openSettings();
    void activateNewTabView();
    void requestTab(int id, const QString &url);
    void closeTab(int id);
    void requestTabReturned(int tabId, void* context);

signals:
    void openUrlRequested(const QString &url);
    void openSettingsRequested();
    void activateNewTabViewRequested();
    void showChrome();

private:
    uint getCallerPid() const;
    bool matchesOwner(uint pid) const;

    BrowserUIServicePrivate *d_ptr;
    Q_DISABLE_COPY(BrowserUIService)
    Q_DECLARE_PRIVATE(BrowserUIService)
};

#endif // BROWSERSERVICE_H
