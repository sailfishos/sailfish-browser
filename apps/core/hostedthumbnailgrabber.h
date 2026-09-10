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
#include <QSize>

template<typename T> class QFutureWatcher;
class QImage;
class QQuickItem;
class QQuickItemGrabResult;

class HostedThumbnailGrabber : public QObject
{
    Q_OBJECT

public:
    explicit HostedThumbnailGrabber(QObject *parent = nullptr);
    ~HostedThumbnailGrabber();

    Q_INVOKABLE quint64 grab(QQuickItem *item, const QString &persistentId,
                             const QString &location,
                             const QString &locationRevision,
                             const QSize &size);
    Q_INVOKABLE void invalidate(const QString &persistentId);
    Q_INVOKABLE void invalidateAll();
    Q_INVOKABLE void cancel(const QString &persistentId, quint64 generation);
    Q_INVOKABLE void discard(const QString &fileName) const;

signals:
    void grabReady(const QString &persistentId, const QString &location,
                   const QString &locationRevision, quint64 generation);
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
        QSize targetSize;
        QSharedPointer<QQuickItemGrabResult> result;
    };

    static QString saveImage(const QImage &image, const QString &persistentId,
                             const QString &locationRevision, quint64 generation);
    bool current(const Capture &capture) const;
    void writeCapture(const Capture &capture, QImage image);

    QHash<QString, quint64> m_generations;
    QHash<QQuickItemGrabResult *, Capture> m_grabs;
    QHash<QFutureWatcher<QString> *, Capture> m_writes;
};

#endif // HOSTEDTHUMBNAILGRABBER_H
