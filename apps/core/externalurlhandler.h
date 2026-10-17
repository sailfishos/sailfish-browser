/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */
#ifndef EXTERNALURLHANDLER_H
#define EXTERNALURLHANDLER_H

#include <QObject>
#include <QHash>
#include <QVariantMap>

class ExternalUrlHandler : public QObject
{
    Q_OBJECT
public:
    explicit ExternalUrlHandler(QObject *parent = nullptr);
    Q_INVOKABLE void check(const QVariantMap &request);
    Q_INVOKABLE void open(const QString &id);
    Q_INVOKABLE void cancel(const QString &id);
    Q_INVOKABLE void cancelAll();
signals:
    void checked(const QVariantMap &request, bool querySucceeded, bool supported);
    void dispatched(const QVariantMap &request, bool delivered);
private:
    struct Request {
        QVariantMap data;
        bool supported = false;
        bool opening = false;
    };
    QHash<QString, Request> m_requests;
};
#endif
