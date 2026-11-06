#pragma once

#include "Image8.h"

#include <QObject>
#include <QString>

template <typename T> class QFutureWatcher;

struct ProcessParams {
    QString method;
    int blackPoint = 0;
    int whitePoint = 255;
    int radius = 3;
    double amount = 0.5;
    bool reflectEdges = false;
    int threshold = 128;
    int windowSize = 31;
    int offsetC = 10;
};

class ProcessingController : public QObject {
    Q_OBJECT

public:
    explicit ProcessingController(QObject* parent = nullptr);

    bool loadOriginal(const QString& path, QString* error = nullptr);
    bool saveResult(const QString& path, QString* error = nullptr) const;

    const imgproc::Image8& original() const { return original_; }
    const imgproc::Image8& result() const { return result_; }
    bool hasOriginal() const { return !original_.empty(); }
    bool busy() const { return busy_; }

public slots:
    void apply(const ProcessParams& params);
    void cancel();

signals:
    void originalLoaded(int width, int height);
    void loadFailed(const QString& message);
    void processStarted();
    void processFinished(double milliseconds, const QString& description);
    void processFailed(const QString& message);

private slots:
    void onDone();

private:
    imgproc::Image8 original_;
    imgproc::Image8 result_;
    QFutureWatcher<imgproc::Image8>* watcher_ = nullptr;
    double t0_ = 0.0;
    QString description_;
    bool busy_ = false;
};
