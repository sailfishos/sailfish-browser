/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

#ifndef DECLARATIVEHISTORYMODEL_H
#define DECLARATIVEHISTORYMODEL_H

#include <QAbstractListModel>
#include <QQmlParserStatus>

#include "tab.h"
#include "link.h"

class DeclarativeHistoryModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int tabId READ tabId WRITE setTabId NOTIFY tabIdChanged)
public:
    DeclarativeHistoryModel(QObject *parent = 0);
    
    enum UrlRoles {
        ThumbPathRole = Qt::UserRole + 1,
        UrlRole,
        TitleRole
    };

    Q_INVOKABLE void clear();
    Q_INVOKABLE void search(const QString &filter);

    int tabId() const;
    void setTabId(int tabId);

    // From QAbstractListModel
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    // From QQmlParserStatus
    void classBegin();
    void componentComplete();

signals:
    void countChanged();
    void tabIdChanged();

private slots:
    void tabHistoryAvailable(int tabId, QList<Link> linkList);
    void historyAvailable(QList<Link> linkList);
    void tabChanged(Tab tab);
    void updateThumbPath(QString url, QString path);
    void updateTitle(QString url, QString title);

private:
    void load();
    void updateModel(QList<Link> linkList);

    int m_tabId;
    QList<Link> m_links;
};
#endif // DECLARATIVEHISTORYMODEL_H
