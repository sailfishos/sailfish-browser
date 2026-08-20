/****************************************************************************
**
** Copyright (c) 2026 Jolla Mobile Ltd
**
****************************************************************************/

#ifndef HOSTEDTHUMBNAILGRABBER_H
#define HOSTEDTHUMBNAILGRABBER_H

#include <QObject>
#include <QHash>
#include <QSharedPointer>

template<typename T> class QFutureWatcher;
class QImage;
class QQuickItem;
class QQuickItemGrabResult;
class QSize;

class HostedThumbnailGrabber : public QObject
{
    Q_OBJECT

public:
    explicit HostedThumbnailGrabber(QObject *parent = nullptr);
    ~HostedThumbnailGrabber();

    Q_INVOKABLE bool grab(QQuickItem *item, const QString &persistentId,
                          const QString &location,
                          const QString &locationRevision,
                          const QSize &size);
    Q_INVOKABLE void invalidate(const QString &persistentId);
    Q_INVOKABLE void discard(const QString &fileName) const;

signals:
    void captureReady(const QString &persistentId, const QString &location,
                      const QString &locationRevision, const QString &fileName);

private slots:
    void handleGrabReady();
    void handleWriteFinished();

private:
    struct Capture {
        QString persistentId;
        QString location;
        QString locationRevision;
        quint64 generation;
        QSharedPointer<QQuickItemGrabResult> result;
    };

    static QString saveImage(const QImage &image, const QString &persistentId,
                             const QString &locationRevision, quint64 generation);
    bool current(const Capture &capture) const;

    QHash<QString, quint64> m_generations;
    QHash<QQuickItemGrabResult *, Capture> m_grabs;
    QHash<QFutureWatcher<QString> *, Capture> m_writes;
};

#endif // HOSTEDTHUMBNAILGRABBER_H
