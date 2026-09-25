#include "MainWindow.h"

#include <QColorDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

namespace {

ModelGroupWidget* makeRgbGroup(QWidget* parent) {
    return new ModelGroupWidget(QStringLiteral("RGB"),
                                {{"R", 0, 255}, {"G", 0, 255}, {"B", 0, 255}}, parent);
}

ModelGroupWidget* makeXyzGroup(QWidget* parent) {
    return new ModelGroupWidget(QStringLiteral("XYZ"),
                                {{"X", 0, 110}, {"Y", 0, 110}, {"Z", 0, 110}}, parent);
}

ModelGroupWidget* makeLabGroup(QWidget* parent) {
    return new ModelGroupWidget(QStringLiteral("Lab"),
                                {{"L", 0, 100}, {"a", -128, 127}, {"b", -128, 127}}, parent);
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildUi();

    connect(rgbGroup_, &ModelGroupWidget::valuesChanged, this,
            [this](double r, double g, double b) { viewModel_.setRgb(
                static_cast<int>(std::lround(r)), static_cast<int>(std::lround(g)),
                static_cast<int>(std::lround(b))); });
    connect(xyzGroup_, &ModelGroupWidget::valuesChanged, this,
            [this](double x, double y, double z) { viewModel_.setXyz(x, y, z); });
    connect(labGroup_, &ModelGroupWidget::valuesChanged, this,
            [this](double l, double a, double b) { viewModel_.setLab(l, a, b); });
    connect(paletteButton_, &QPushButton::clicked, this, &MainWindow::pickFromPalette);
    connect(&viewModel_, &ColorViewModel::stateChanged, this, &MainWindow::refreshFromModel);

    viewModel_.setRgb(255, 0, 0);  // emits stateChanged -> initial refresh
}

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setSpacing(12);

    preview_ = new QLabel(central);
    preview_->setFixedHeight(56);
    preview_->setAlignment(Qt::AlignCenter);
    QFont previewFont = preview_->font();
    previewFont.setPointSize(14);
    previewFont.setBold(true);
    preview_->setFont(previewFont);
    preview_->setStyleSheet(
        QStringLiteral("border: 1px solid #b0b0b0; border-radius: 6px;"));

    paletteButton_ = new QPushButton(QStringLiteral("Выбрать из палитры..."), central);
    paletteButton_->setToolTip(
        QStringLiteral("Стандартный диалог выбора цвета (как в графических редакторах)"));

    auto* infoButton = new QPushButton(QStringLiteral("i"), central);
    infoButton->setToolTip(QStringLiteral("Справка по использованию программы"));
    infoButton->setFixedSize(30, 30);
    QFont infoFont = infoButton->font();
    infoFont.setBold(true);
    infoFont.setItalic(true);
    infoButton->setFont(infoFont);
    connect(infoButton, &QPushButton::clicked, this, &MainWindow::showGuide);

    auto* topRow = new QHBoxLayout();
    topRow->addWidget(preview_, 1);
    topRow->addWidget(paletteButton_);
    topRow->addWidget(infoButton);
    rootLayout->addLayout(topRow);

    rgbGroup_ = makeRgbGroup(central);
    xyzGroup_ = makeXyzGroup(central);
    labGroup_ = makeLabGroup(central);

    rgbGroup_->setToolTip(
        QStringLiteral("Аддитивная модель: интенсивности красного, зелёного и синего (0–255)"));
    xyzGroup_->setToolTip(QStringLiteral(
        "Аппаратно-независимая модель МКО, источник D65. X, Y, Z — в шкале 0–100; "
        "белому цвету соответствуют (95.047; 100; 108.883)"));
    labGroup_->setToolTip(QStringLiteral(
        "Перцептивная модель МКО: L — светлота (0–100), a — зелёный–пурпурный, "
        "b — синий–жёлтый"));

    auto* modelsRow = new QHBoxLayout();
    modelsRow->addWidget(rgbGroup_);
    modelsRow->addWidget(xyzGroup_);
    modelsRow->addWidget(labGroup_);
    rootLayout->addLayout(modelsRow);

    warning_ = new QLabel(central);
    warning_->setWordWrap(true);
    warning_->setAlignment(Qt::AlignVCenter);
    warning_->setStyleSheet(QStringLiteral(
        "background: #fff3cd; color: #664d03; border: 1px solid #ffe69c; "
        "border-radius: 4px; padding: 6px;"));
    // Панель предупреждения закреплена: место под неё выделено постоянно,
    // чтобы макет не «дёргался»; появляется и исчезает только текст.
    warning_->setFixedHeight(34);
    warning_->setText(QString());
    rootLayout->addWidget(warning_);

    setCentralWidget(central);
    setWindowTitle(QStringLiteral(
        "Лабораторная работа 1. Цветовые модели — вариант 3: RGB ↔ XYZ ↔ Lab"));
    resize(1000, 300);
    setMinimumSize(860, 260);
}

void MainWindow::refreshFromModel() {
    const colormodel::Rgb rgb = viewModel_.rgb();
    const colormodel::Xyz xyz = viewModel_.xyz();
    const colormodel::Lab lab = viewModel_.lab();

    rgbGroup_->setValues({static_cast<double>(rgb.r), static_cast<double>(rgb.g),
                          static_cast<double>(rgb.b)});
    xyzGroup_->setValues({xyz.x, xyz.y, xyz.z});
    labGroup_->setValues({lab.l, lab.a, lab.b});

    const QColor previewColor(rgb.r, rgb.g, rgb.b);
    const QString textColor = viewModel_.lab().l > 55.0 ? QStringLiteral("#000000")
                                                        : QStringLiteral("#ffffff");
    preview_->setStyleSheet(QStringLiteral(
        "background: %1; color: %2; border: 1px solid #b0b0b0; border-radius: 6px;")
        .arg(previewColor.name().toUpper(), textColor));
    preview_->setText(previewColor.name().toUpper());

    warning_->setText(viewModel_.warningText());
}

void MainWindow::pickFromPalette() {
    const colormodel::Rgb rgb = viewModel_.rgb();
    const QColor initial(rgb.r, rgb.g, rgb.b);
    const QColor chosen =
        QColorDialog::getColor(initial, this, QStringLiteral("Выбор цвета"));
    if (chosen.isValid()) {
        viewModel_.setRgb(chosen.red(), chosen.green(), chosen.blue());
    }
}

void MainWindow::showGuide() {
    const QString guide = QStringLiteral(
        "Как пользоваться программой (вариант 3: RGB ↔ XYZ ↔ Lab)\n"
        "\n"
        "1. Задайте цвет в любой из трёх моделей одним из способов:\n"
        "   • перетащите ползунок компоненты (плавное изменение);\n"
        "   • введите точное значение в числовое поле справа от ползунка;\n"
        "   • нажмите «Выбрать из палитры...» и укажите цвет в стандартном\n"
        "     диалоге.\n"
        "\n"
        "2. При изменении любой компоненты цвет пересчитывается в двух\n"
        "   остальных моделях автоматически.\n"
        "\n"
        "3. Большое поле сверху показывает текущий цвет и его HEX-код\n"
        "   (#RRGGBB).\n"
        "\n"
        "4. Если введённый XYZ- или Lab-цвет не существует в sRGB, внизу\n"
        "   в жёлтой панели появится предупреждение о том, какие компоненты\n"
        "   были обрезаны; RGB-поля покажут ближайший отображаемый цвет.\n"
        "\n"
        "5. Диапазоны значений:\n"
        "   • RGB: 0–255;\n"
        "   • XYZ: 0–110 (белому цвету D65 соответствует (95.047; 100; 108.883));\n"
        "   • Lab: светлота L — 0–100, оси a и b — от −128 до 127.\n"
        "\n"
        "Наведите курсор на заголовок группы модели, чтобы увидеть её\n"
        "краткое описание.");
    QMessageBox::information(this, QStringLiteral("Справка — Лабораторная работа 1"),
                             guide);
}
