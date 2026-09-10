/****************************************************************************
**
** Copyright (c) 2026 Jolla Mobile Ltd
**
****************************************************************************/

#include "hostedthumbnailgrabber.h"

#include <QFile>
#include <QFutureWatcher>
#include <QImage>
#include <QTimer>
#include <qmoznativeview.h>
#include <QSaveFile>
#include <QtConcurrentRun>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickItemGrabResult>

#include "browserpaths.h"

HostedThumbnailGrabber::HostedThumbnailGrabber(QObject *parent)
    : QObject(parent)
{
}

HostedThumbnailGrabber::~HostedThumbnailGrabber()
{
    for (QQuickItemGrabResult *result : m_grabs.keys()) {
        disconnect(result, nullptr, this, nullptr);
    }
    m_grabs.clear();

    const QList<QFutureWatcher<QString> *> watchers = m_writes.keys();
    for (QFutureWatcher<QString> *watcher : watchers) {
        disconnect(watcher, nullptr, this, nullptr);
        watcher->waitForFinished();
        discard(watcher->result());
    }
    m_writes.clear();
}

quint64 HostedThumbnailGrabber::grab(QQuickItem *item,
                                     const QString &persistentId,
                                     const QString &location,
                                     const QString &locationRevision,
                                     const QSize &size)
{
    if (!item || persistentId.isEmpty() || location.isEmpty() || !size.isValid()) {
        return 0;
    }

    // Preserve the source aspect ratio while filling the requested capture
    // size, then crop the overflow from the right and bottom.
    QSize grabSize = item->boundingRect().size().toSize();
    if (!grabSize.isValid()) {
        return 0;
    }
    grabSize.scale(size, Qt::KeepAspectRatioByExpanding);

    const quint64 generation = ++m_generations[persistentId];
    if (QMozNativeView *view = qobject_cast<QMozNativeView *>(item)) {
        const QImage image = view->captureImage(grabSize);
        if (image.isNull()) return 0;
        const Capture capture = {persistentId, location, locationRevision,
                                 generation, size, {}};
        // Let QML record the generation before delivering the capture.
        QTimer::singleShot(0, this, [this, capture, image]() {
            writeCapture(capture, image);
        });
        return generation;
    }
    QSharedPointer<QQuickItemGrabResult> result = item->grabToImage(grabSize);
    if (!result) {
        return 0;
    }

    Capture capture;
    capture.persistentId = persistentId;
    capture.location = location;
    capture.locationRevision = locationRevision;
    capture.generation = generation;
    capture.targetSize = size;
    capture.result = result;
    m_grabs.insert(result.data(), capture);

    connect(result.data(), &QQuickItemGrabResult::ready,
            this, &HostedThumbnailGrabber::handleGrabReady);
    return generation;
}

void HostedThumbnailGrabber::invalidate(const QString &persistentId)
{
    if (!persistentId.isEmpty()) {
        ++m_generations[persistentId];
    }
}

void HostedThumbnailGrabber::invalidateAll()
{
    for (auto it = m_generations.begin(); it != m_generations.end(); ++it) {
        ++it.value();
    }
}

void HostedThumbnailGrabber::cancel(const QString &persistentId,
                                    quint64 generation)
{
    if (!persistentId.isEmpty()
            && m_generations.value(persistentId) == generation) {
        ++m_generations[persistentId];
    }
}

void HostedThumbnailGrabber::discard(const QString &fileName) const
{
    if (!fileName.isEmpty()) {
        QFile::remove(fileName);
    }
}

void HostedThumbnailGrabber::handleGrabReady()
{
    QQuickItemGrabResult *result = qobject_cast<QQuickItemGrabResult *>(sender());
    if (!result || !m_grabs.contains(result)) {
        return;
    }

    const Capture capture = m_grabs.take(result);
    writeCapture(capture, capture.result->image());
}

void HostedThumbnailGrabber::writeCapture(const Capture &capture, QImage image)
{
    if (image.isNull() || !current(capture)) {
        return;
    }

    const QSize scaledSize = image.size().scaled(
                capture.targetSize, Qt::KeepAspectRatioByExpanding);
    if (image.size() != scaledSize) {
        image = image.scaled(scaledSize, Qt::IgnoreAspectRatio,
                             Qt::SmoothTransformation);
    }
    image = image.copy(0, 0, capture.targetSize.width(),
                       capture.targetSize.height());
    if (image.isNull()) {
        return;
    }

    QFutureWatcher<QString> *watcher = new QFutureWatcher<QString>(this);
    m_writes.insert(watcher, capture);
    connect(watcher, &QFutureWatcher<QString>::finished,
            this, &HostedThumbnailGrabber::handleWriteFinished);
    watcher->setFuture(QtConcurrent::run(&HostedThumbnailGrabber::saveImage,
                                         image, capture.persistentId,
                                         capture.locationRevision,
                                         capture.generation));
    emit grabReady(capture.persistentId, capture.location,
                   capture.locationRevision, capture.generation);
}

void HostedThumbnailGrabber::handleWriteFinished()
{
    QFutureWatcher<QString> *watcher =
            static_cast<QFutureWatcher<QString> *>(sender());
    if (!watcher || !m_writes.contains(watcher)) {
        return;
    }

    const Capture capture = m_writes.take(watcher);
    const QString fileName = watcher->result();
    watcher->deleteLater();

    if (fileName.isEmpty()) {
        return;
    }
    if (!current(capture)) {
        discard(fileName);
        return;
    }

    emit captureReady(capture.persistentId, capture.location,
                      capture.locationRevision, fileName);
}

QString HostedThumbnailGrabber::saveImage(const QImage &image,
                                          const QString &persistentId,
                                          const QString &locationRevision,
                                          quint64 generation)
{
    const QString cacheLocation = BrowserPaths::cacheLocation();
    if (cacheLocation.isEmpty()) {
        return QString();
    }

    const QString fileName = QStringLiteral("%1/tab-%2-thumb-%3-%4.jpg")
            .arg(cacheLocation, persistentId, locationRevision)
            .arg(generation);
    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)
            || !image.save(&file, "JPG", 75)
            || !file.commit()) {
        return QString();
    }
    return fileName;
}

bool HostedThumbnailGrabber::current(const Capture &capture) const
{
    return m_generations.value(capture.persistentId) == capture.generation;
}
