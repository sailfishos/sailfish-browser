/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

#ifndef DECLARATIVETAB_H
#define DECLARATIVETAB_H

#include <QImage>
#include <QObject>
#include <QQuickItem>
#include <QStringList>

#include "tab.h"
#include "link.h"

class DeclarativeTab : public QQuickItem {
    Q_OBJECT

    Q_PROPERTY(int tabId READ tabId WRITE setTabId NOTIFY tabIdChanged)
    Q_PROPERTY(bool valid READ valid NOTIFY validChanged FINAL)
    Q_PROPERTY(QString thumbnailPath READ thumbnailPath WRITE setThumbnailPath NOTIFY thumbPathChanged)
    Q_PROPERTY(QString url READ url NOTIFY urlChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY canGoFowardChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY canGoBackChanged)

public:
    DeclarativeTab(QQuickItem *parent = 0);
    ~DeclarativeTab();

    QString thumbnailPath() const;
    void setThumbnailPath(QString thumbnailPath);

    QString url() const;

    QString title() const;
    void setTitle(QString title);

    int tabId() const;
    void setTabId(int tabId);

    bool valid() const;

    bool canGoForward() const;
    bool canGoBack() const;

    Q_INVOKABLE void goForward();
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void updateTab(QString url, QString title, QString path);
    Q_INVOKABLE void navigateTo(QString url, QString title, QString path);
    Q_INVOKABLE void captureScreen(QString url, int x, int y, int width, int height, qreal rotate);

public slots:
    void tabChanged(Tab tab);
    void updateTitle(QString url, QString title);

private slots:
    void updateThumbPath(QString url, QString path, int tabId);

signals:
    void thumbPathChanged();
    void urlChanged();
    void tabIdChanged();
    void validChanged();
    void titleChanged();
    void canGoFowardChanged();
    void canGoBackChanged();

private:
    void init();
    void saveToFile(QString url, QString path, QImage image, int tabId, qreal rotate);

    int m_tabId;
    bool m_valid;
    Link m_link;
    int m_nextLinkId, m_previousLinkId;
};

#endif // DECLARATIVETAB_H
