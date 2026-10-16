#pragma once

#include "ImageInfo.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVector>

#include <atomic>

template <typename T> class QFutureWatcher;

class FolderScanner : public QObject {
    Q_OBJECT

public:
    explicit FolderScanner(QObject* parent = nullptr);
    ~FolderScanner() override;

    bool isScanning() const { return scanning_; }

public slots:
    void start(const QString& folder);
    void cancel();

signals:
    void scanStarted(int total);
    void progress(int done);
    void rowsReady(const QVector<img::ImageInfo>& rows);
    void scanFinished(double seconds, int total, int images, int damaged, int nonImages);

private slots:
    void onResultsReadyAt(int begin, int end);
    void onFinished();
    void flushRows();

private:
    QFutureWatcher<img::ImageInfo>* watcher_ = nullptr;
    QTimer flushTimer_;
    QVector<img::ImageInfo> pending_;
    int emitted_ = 0;
    int total_ = 0;
    int images_ = 0;
    int damaged_ = 0;
    int nonImages_ = 0;
    bool scanning_ = false;
    qint64 t0_ = 0;
};
