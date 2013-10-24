/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

#ifndef DECLARATIVEWEBCONTAINER_H
#define DECLARATIVEWEBCONTAINER_H

#include <QQuickItem>
#include <QPointer>

class DeclarativeWebContainer : public QQuickItem {
    Q_OBJECT

    Q_PROPERTY(QQuickItem *webView READ webView WRITE setWebView NOTIFY webViewChanged FINAL)
    Q_PROPERTY(bool foreground READ foreground WRITE setForeground NOTIFY foregroundChanged FINAL)
    Q_PROPERTY(bool pageActive READ pageActive WRITE setPageActive NOTIFY pageActiveChanged FINAL)
    Q_PROPERTY(bool inputPanelVisible READ inputPanelVisible NOTIFY inputPanelVisibleChanged FINAL)
    Q_PROPERTY(qreal inputPanelHeight READ inputPanelHeight WRITE setInputPanelHeight NOTIFY inputPanelHeightChanged FINAL)
    Q_PROPERTY(qreal inputPanelOpenHeight READ inputPanelOpenHeight WRITE setInputPanelOpenHeight NOTIFY inputPanelOpenHeightChanged FINAL)
    Q_PROPERTY(qreal toolbarHeight READ toolbarHeight WRITE setToolbarHeight NOTIFY toolbarHeightChanged FINAL)

public:
    DeclarativeWebContainer(QQuickItem *parent = 0);
    ~DeclarativeWebContainer();

    QQuickItem *webView() const;
    void setWebView(QQuickItem *webView);

    bool foreground() const;
    void setForeground(bool active);

    bool pageActive() const;
    void setPageActive(bool active);

    bool inputPanelVisible() const;

    qreal inputPanelHeight() const;
    void setInputPanelHeight(qreal height);

    qreal inputPanelOpenHeight() const;
    void setInputPanelOpenHeight(qreal height);

    qreal toolbarHeight() const;
    void setToolbarHeight(qreal height);

signals:
    void webViewChanged();
    void pageStackChanged();
    void foregroundChanged();
    void pageActiveChanged();
    void inputPanelVisibleChanged();
    void inputPanelHeightChanged();
    void inputPanelOpenHeightChanged();
    void toolbarHeightChanged();

public slots:
    void resetHeight(bool respectContentHeight = true);

private slots:
    void imeNotificationChanged(int state, bool open, int cause, int focusChange, const QString& type);

private:
    qreal contentHeight() const;

private:
    QPointer<QQuickItem> m_webView;
    bool m_foreground;
    bool m_pageActive;
    bool m_inputPanelVisible;
    qreal m_inputPanelHeight;
    qreal m_inputPanelOpenHeight;
    qreal m_toolbarHeight;
};

#endif // DECLARATIVEWEBCONTAINER_H
