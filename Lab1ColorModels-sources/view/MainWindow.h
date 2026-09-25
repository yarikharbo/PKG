#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// View layer: builds the interface, forwards user edits to the ViewModel
// and renders the ViewModel state. Contains no conversion math.

#include <QLabel>
#include <QMainWindow>
#include <QPushButton>

#include "ColorViewModel.h"
#include "ModelGroupWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void refreshFromModel();
    void pickFromPalette();
    void showGuide();

private:
    void buildUi();

    ColorViewModel viewModel_;

    ModelGroupWidget* rgbGroup_ = nullptr;
    ModelGroupWidget* xyzGroup_ = nullptr;
    ModelGroupWidget* labGroup_ = nullptr;
    QLabel* preview_ = nullptr;
    QLabel* warning_ = nullptr;
    QPushButton* paletteButton_ = nullptr;
};

#endif  // MAINWINDOW_H
