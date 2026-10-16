#include "FolderScanner.h"

#include "ImageInfoReader.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentMap>

namespace {

img::ImageInfo scanOne(const QString& path) {
    return img::readImageInfo(path.toStdString());
}

}

FolderScanner::FolderScanner(QObject* parent)
    : QObject(parent) {
    flushTimer_.setInterval(80);
    flushTimer_.setSingleShot(false);
    connect(&flushTimer_, &QTimer::timeout, this, &FolderScanner::flushRows);
}

FolderScanner::~FolderScanner() {
    cancel();
}

void FolderScanner::start(const QString& folder) {
    if (scanning_) return;
    const QFileInfo fi(folder);
    if (!fi.isDir()) return;

    QStringList files;
    QDir dir(folder);
    const auto entries = dir.entryInfoList(QDir::Files | QDir::Hidden, QDir::Name);
    files.reserve(entries.size());
    for (const QFileInfo& e : entries) files.append(e.absoluteFilePath());

    total_ = files.size();
    emitted_ = 0;
    images_ = 0;
    damaged_ = 0;
    nonImages_ = 0;
    pending_.clear();
    scanning_ = true;
    t0_ = QDateTime::currentMSecsSinceEpoch();

    emit scanStarted(total_);
    if (total_ == 0) {
        scanning_ = false;
        emit scanFinished(0.0, 0, 0, 0, 0);
        return;
    }

    if (!watcher_) {
        watcher_ = new QFutureWatcher<img::ImageInfo>(this);
        connect(watcher_, &QFutureWatcherBase::resultsReadyAt, this, &FolderScanner::onResultsReadyAt);
        connect(watcher_, &QFutureWatcherBase::finished, this, &FolderScanner::onFinished);
        connect(watcher_, &QFutureWatcherBase::progressValueChanged, this, [this](int v) { emit progress(v); });
    }

    const QFuture<img::ImageInfo> future = QtConcurrent::mapped(files, scanOne);
    watcher_->setFuture(future);
    flushTimer_.start();
}

void FolderScanner::cancel() {
    if (!scanning_ || !watcher_) return;
    watcher_->future().cancel();
    watcher_->waitForFinished();
}

void FolderScanner::onResultsReadyAt(int begin, int end) {
    for (int i = begin; i < end && i < total_; ++i) {
        pending_.append(watcher_->resultAt(i));
    }
    emitted_ = qMax(emitted_, end);
}

void FolderScanner::onFinished() {
    flushTimer_.stop();
    for (int i = emitted_; i < total_; ++i) {
        if (watcher_->future().isResultReadyAt(i)) {
            pending_.append(watcher_->resultAt(i));
            emitted_ = i + 1;
        }
    }
    flushRows();
    scanning_ = false;
    const double secs = (QDateTime::currentMSecsSinceEpoch() - t0_) / 1000.0;
    emit scanFinished(secs, total_, images_, damaged_, nonImages_);
}

void FolderScanner::flushRows() {
    if (pending_.isEmpty()) return;
    for (const img::ImageInfo& info : pending_) {
        if (info.format == img::Format::NotImage) ++nonImages_;
        else if (info.ok) ++images_;
        else ++damaged_;
    }
    emit rowsReady(pending_);
    pending_.clear();
    if (!scanning_) flushTimer_.stop();
}
