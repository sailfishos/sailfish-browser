/****************************************************************************
**
** Copyright (c) 2026 Jolla Mobile Ltd
**
****************************************************************************/

#include "hostedthumbnailgrabber.h"

#include <QFile>
#include <QFutureWatcher>
#include <QImage>
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

bool HostedThumbnailGrabber::grab(QQuickItem *item, const QString &persistentId,
                                  const QString &location,
                                  const QString &locationRevision,
                                  const QSize &size)
{
    if (!item || persistentId.isEmpty() || location.isEmpty() || !size.isValid()) {
        return false;
    }

    const quint64 generation = ++m_generations[persistentId];
    QSharedPointer<QQuickItemGrabResult> result = item->grabToImage(size);
    if (!result) {
        return false;
    }

    Capture capture;
    capture.persistentId = persistentId;
    capture.location = location;
    capture.locationRevision = locationRevision;
    capture.generation = generation;
    capture.result = result;
    m_grabs.insert(result.data(), capture);

    connect(result.data(), &QQuickItemGrabResult::ready,
            this, &HostedThumbnailGrabber::handleGrabReady);
    return true;
}

void HostedThumbnailGrabber::invalidate(const QString &persistentId)
{
    if (!persistentId.isEmpty()) {
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
    const QImage image = capture.result->image();
    if (image.isNull() || !current(capture)) {
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
