/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

#ifndef DECLARATIVETABMODEL_H
#define DECLARATIVETABMODEL_H

#include <QAbstractListModel>
#include <QQmlParserStatus>

#include "tab.h"

class DeclarativeTabModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int currentTabId READ currentTabId NOTIFY currentTabChanged)
    Q_PROPERTY(int currentTabIndex READ currentTabIndex WRITE setCurrentTabIndex NOTIFY currentTabChanged)
public:
    DeclarativeTabModel(QObject *parent = 0);
    
    enum TabRoles {
        ThumbPathRole = Qt::UserRole + 1,
        TitleRole,
        UrlRole,
        TabIdRole
    };

    Q_INVOKABLE void addTab(const QString& url, bool foreground = false);
    Q_INVOKABLE void remove(const int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool activateTab(const QString& url);

    // From QAbstractListModel
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    // From QQmlParserStatus
    void classBegin();
    void componentComplete();

    void setCurrentTabIndex(const int index);
    int currentTabIndex() const;

    int currentTabId() const;

public slots:
    void tabsAvailable(QList<Tab> tabs);

signals:
    void countChanged();
    void currentTabChanged();

private slots:
    void updateThumbPath(QString url, QString path);
    void updateTitle(QString url, QString title);
    void tabChanged(Tab tab);

private:
    void load();

    QList<Tab> m_tabs;
    int m_currentTabIndex;
};
#endif // DECLARATIVETABMODEL_H
