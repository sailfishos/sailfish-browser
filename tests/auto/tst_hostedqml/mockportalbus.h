/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */
#ifndef MOCKPORTALBUS_H
#define MOCKPORTALBUS_H

#include <QJSValue>
#include <QObject>
#include <QVariantList>

// The client tests control method replies independently of service changes.
class MockPortalBus : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString service MEMBER service)
    Q_PROPERTY(QString path MEMBER path)
    Q_PROPERTY(QString iface MEMBER iface)
    Q_PROPERTY(bool signalsEnabled MEMBER signalsEnabled)
    Q_PROPERTY(bool watchServiceStatus MEMBER watchServiceStatus)
    Q_PROPERTY(int status READ status WRITE setStatus NOTIFY statusChanged)
public:
    enum Status { Unknown, Unavailable, Available };
    Q_ENUM(Status)
    explicit MockPortalBus(QObject *parent = nullptr) : QObject(parent) {}
    int status() const { return m_status; }
    void setStatus(int value) {
        if (m_status == value) return;
        m_status = value;
        emit statusChanged();
    }
    struct Call {
        QString method;
        QVariantList args;
        QJSValue success;
        QJSValue failure;
    };
    Q_INVOKABLE void call(const QString &method, const QVariantList &args,
                          QJSValue success = QJSValue(), QJSValue failure = QJSValue()) {
        calls.append({method, args, success, failure});
    }
    QString service;
    QString path;
    QString iface;
    bool signalsEnabled = false;
    bool watchServiceStatus = false;
    QList<Call> calls;
signals:
    void statusChanged();
private:
    int m_status = Available;
};

#endif
