#pragma once

#include "ProcessingController.h"

#include <QMainWindow>

class QComboBox;
class QLabel;
class QLineEdit;
class QSlider;

class HistogramWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    void loadAndApplyForSmoke(const QString& imagePath, const QString& screenshotPath);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void browse();
    void saveResult();
    void applyCurrent();
    void onOriginalLoaded(int width, int height);
    void onLoadFailed(const QString& message);
    void onProcessFinished(double ms, const QString& method);
    void onProcessFailed(const QString& message);
    void showUserGuide();
    void showTheoryHelp();

private:
    void buildUi();
    void updateParameterVisibility();
    void refreshHistograms();
    void setPixmapScaled(QLabel* label, const imgproc::Image8& image);

    ProcessingController controller_;
    QLineEdit* pathEdit_ = nullptr;
    QComboBox* methodBox_ = nullptr;
    QSlider* thresholdSlider_ = nullptr;
    QSlider* windowSlider_ = nullptr;
    QSlider* offsetSlider_ = nullptr;
    QSlider* radiusSlider_ = nullptr;
    QSlider* amountSlider_ = nullptr;
    QSlider* blackSlider_ = nullptr;
    QSlider* whiteSlider_ = nullptr;
    QComboBox* edgeBox_ = nullptr;
    QLabel* thresholdTitle_ = nullptr;
    QLabel* windowTitle_ = nullptr;
    QLabel* offsetTitle_ = nullptr;
    QLabel* radiusTitle_ = nullptr;
    QLabel* amountTitle_ = nullptr;
    QLabel* blackTitle_ = nullptr;
    QLabel* whiteTitle_ = nullptr;
    QLabel* edgeTitle_ = nullptr;
    QLabel* thresholdLabel_ = nullptr;
    QLabel* windowLabel_ = nullptr;
    QLabel* offsetLabel_ = nullptr;
    QLabel* radiusLabel_ = nullptr;
    QLabel* amountLabel_ = nullptr;
    QLabel* blackLabel_ = nullptr;
    QLabel* whiteLabel_ = nullptr;
    QLabel* beforeLabel_ = nullptr;
    QLabel* afterLabel_ = nullptr;
    HistogramWidget* histBefore_ = nullptr;
    HistogramWidget* histAfter_ = nullptr;
    QString smokePath_;
    bool smokeShot_ = false;
};
