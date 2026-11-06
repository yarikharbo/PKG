#include "ProcessingController.h"

#include "Contrast.h"
#include "Filters.h"
#include "Threshold.h"

#include <QDateTime>
#include <QFutureWatcher>
#include <QImage>
#include <QImageReader>
#include <QtConcurrent/QtConcurrentRun>

using namespace imgproc;

namespace {

imgproc::Image8 fromQImage(const QImage& image) {
    QImage img = image.convertToFormat(QImage::Format_RGB888);
    Image8 out(img.width(), img.height());
    const int w = img.width();
    for (int y = 0; y < img.height(); ++y) {
        std::memcpy(out.row(y), img.constScanLine(y), static_cast<size_t>(w) * 3);
    }
    return out;
}

QImage toQImage(const imgproc::Image8& img) {
    QImage out(img.w(), img.h(), QImage::Format_RGB888);
    for (int y = 0; y < img.h(); ++y) {
        std::memcpy(out.scanLine(y), img.row(y), static_cast<size_t>(img.w()) * 3);
    }
    return out;
}

Image8 runMethod(const Image8& in, const ProcessParams& p) {
    const EdgeMode em = p.reflectEdges ? EdgeMode::Reflect : EdgeMode::Clamp;
    if (p.method == QStringLiteral("threshold")) return toGrayscaleBin(in, p.threshold);
    if (p.method == QStringLiteral("otsu")) return thresholdOtsu(in);
    if (p.method == QStringLiteral("adaptive")) return thresholdAdaptive(in, p.windowSize, p.offsetC);
    if (p.method == QStringLiteral("linear")) return linearContrast(in, p.blackPoint, p.whitePoint);
    if (p.method == QStringLiteral("equalize_gray")) return equalizeGlobal(in, EqualizeMode::Gray);
    if (p.method == QStringLiteral("equalize_rgb")) return equalizeGlobal(in, EqualizeMode::PerChannelRGB);
    if (p.method == QStringLiteral("equalize_hsv")) return equalizeGlobal(in, EqualizeMode::HSV_V);
    if (p.method == QStringLiteral("laplacian4")) return laplacianSharpen(in, 4, p.amount, em);
    if (p.method == QStringLiteral("laplacian8")) return laplacianSharpen(in, 8, p.amount, em);
    if (p.method == QStringLiteral("unsharp")) return unsharpMask(in, p.radius, p.amount, em);
    if (p.method == QStringLiteral("gauss")) return gaussianBlur(in, p.radius, em);
    return in;
}

}

ProcessingController::ProcessingController(QObject* parent)
    : QObject(parent) {
    watcher_ = new QFutureWatcher<Image8>(this);
    connect(watcher_, &QFutureWatcherBase::finished, this, &ProcessingController::onDone);
}

bool ProcessingController::loadOriginal(const QString& path, QString* error) {
    QImageReader reader(path);
    reader.setAutoTransform(true);
    QImage image;
    if (!reader.read(&image)) {
        if (error) *error = reader.errorString();
        emit loadFailed(reader.errorString());
        return false;
    }
    original_ = fromQImage(image);
    result_ = Image8();
    if (error) error->clear();
    emit originalLoaded(original_.w(), original_.h());
    return true;
}

bool ProcessingController::saveResult(const QString& path, QString* error) const {
    if (result_.empty()) {
        if (error) *error = QStringLiteral("Нет результата для сохранения");
        return false;
    }
    QImage image = toQImage(result_);
    if (!image.save(path)) {
        if (error) *error = QStringLiteral("Не удалось сохранить файл");
        return false;
    }
    if (error) error->clear();
    return true;
}

void ProcessingController::apply(const ProcessParams& params) {
    if (busy_ || original_.empty()) return;
    busy_ = true;
    description_ = params.method;
    t0_ = QDateTime::currentMSecsSinceEpoch();
    emit processStarted();
    watcher_->setFuture(QtConcurrent::run([in = original_, p = params]() mutable {
        return runMethod(in, p);
    }));
}

void ProcessingController::cancel() {
    busy_ = false;
}

void ProcessingController::onDone() {
    busy_ = false;
    result_ = watcher_->result();
    const double ms = QDateTime::currentMSecsSinceEpoch() - t0_;
    emit processFinished(ms, description_);
}
