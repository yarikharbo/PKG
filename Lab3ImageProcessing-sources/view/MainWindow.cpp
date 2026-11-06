#include "MainWindow.h"

#include "Contrast.h"
#include "Filters.h"
#include "HistogramWidget.h"
#include "Image8.h"
#include "Threshold.h"

#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSlider>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {

struct MethodSpec {
    const char* id;
    const char* title;
    bool threshold, window, offset, radius, amount, black, white, edges;
};

const MethodSpec kMethods[] = {
    {"threshold", "Пороговая (ручной порог)", true, false, false, false, false, false, false, false},
    {"otsu", "Пороговая (Оцу — автоматический подбор)", false, false, false, false, false, false, false, false},
    {"adaptive", "Пороговая адаптивная (среднее − C)", false, true, true, false, false, false, false, false},
    {"gauss", "Сглаживание: гауссов фильтр", false, false, false, true, false, false, false, true},
    {"box", "Сглаживание: прямоугольный фильтр", false, false, false, true, false, false, false, true},
    {"linear", "Доп. линейное контрастирование", false, false, false, false, false, true, true, false},
    {"equalize_gray", "Доп. эквализация (серое)", false, false, false, false, false, false, false, false},
    {"equalize_rgb", "Доп. эквализация RGB (по каналам)", false, false, false, false, false, false, false, false},
    {"equalize_hsv", "Доп. эквализация HSV (по яркости V)", false, false, false, false, false, false, false, false},
    {"laplacian4", "Доп. резкость: лапласиан 4-связный", false, false, false, false, true, false, false, true},
    {"laplacian8", "Доп. резкость: лапласиан 8-связный", false, false, false, false, true, false, false, true},
    {"unsharp", "Доп. нерезкое маскирование", false, false, false, true, true, false, false, true},
    {"gray", "Доп. градации серого", false, false, false, false, false, false, false, false},
};

int methodIndex(const QString& id) {
    for (size_t i = 0; i < sizeof(kMethods) / sizeof(kMethods[0]); ++i) {
        if (id == kMethods[i].id) return static_cast<int>(i);
    }
    return 0;
}

ProcessParams paramsFor(int methodIdx, int threshold, int window, int offsetC, int radius, int amountPct,
                        int black, int white, bool reflect) {
    const MethodSpec& spec = kMethods[methodIdx];
    ProcessParams p;
    p.method = QString::fromUtf8(spec.id);
    p.threshold = threshold;
    p.windowSize = window;
    p.offsetC = offsetC;
    p.radius = radius;
    p.amount = amountPct / 100.0;
    p.blackPoint = black;
    p.whitePoint = white;
    p.reflectEdges = reflect;
    return p;
}

}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    buildUi();
    setWindowTitle(QStringLiteral("Лабораторная работа 3 — Обработка цифровых изображений (вариант 7)"));
    resize(1240, 780);
}

void MainWindow::buildUi() {
    auto* tb = addToolBar(QStringLiteral("main"));
    tb->setMovable(false);
    auto* openBtn = new QPushButton(QStringLiteral("Открыть…"), tb);
    connect(openBtn, &QPushButton::clicked, this, &MainWindow::browse);
    tb->addWidget(openBtn);
    pathEdit_ = new QLineEdit(tb);
    pathEdit_->setMinimumWidth(300);
    pathEdit_->setPlaceholderText(QStringLiteral("Путь к изображению…"));
    tb->addWidget(pathEdit_);
    auto* saveBtn = new QPushButton(QStringLiteral("Сохранить результат…"), tb);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveResult);
    tb->addWidget(saveBtn);
    tb->addSeparator();
    tb->addWidget(new QLabel(QStringLiteral(" Метод: "), tb));
    methodBox_ = new QComboBox(tb);
    for (const MethodSpec& m : kMethods) methodBox_->addItem(QString::fromUtf8(m.title), QString::fromUtf8(m.id));
    methodBox_->setCurrentIndex(2);
    tb->addWidget(methodBox_);
    auto* applyBtn = new QPushButton(QStringLiteral("Применить"), tb);
    applyBtn->setDefault(true);
    connect(applyBtn, &QPushButton::clicked, this, &MainWindow::applyCurrent);
    tb->addWidget(applyBtn);
    auto* guideBtn = new QPushButton(QStringLiteral("i"), tb);
    guideBtn->setFixedSize(30, 30);
    guideBtn->setToolTip(QStringLiteral("Краткое руководство по использованию"));
    connect(guideBtn, &QPushButton::clicked, this, &MainWindow::showUserGuide);
    tb->addWidget(guideBtn);

    auto* paramRow = new QWidget(this);
    auto* pl = new QHBoxLayout(paramRow);
    pl->setContentsMargins(8, 2, 8, 2);

    auto addSlider = [&](QSlider*& slider, QLabel*& label, QLabel*& title, const QString& titleText, int min, int max,
                         int value) {
        title = new QLabel(titleText, paramRow);
        pl->addWidget(title);
        slider = new QSlider(Qt::Horizontal, paramRow);
        slider->setRange(min, max);
        slider->setValue(value);
        slider->setFixedWidth(110);
        pl->addWidget(slider);
        label = new QLabel(QString::number(value), paramRow);
        label->setMinimumWidth(30);
        pl->addWidget(label);
    };
    addSlider(thresholdSlider_, thresholdLabel_, thresholdTitle_, QStringLiteral("Порог:"), 0, 255, 128);
    addSlider(windowSlider_, windowLabel_, windowTitle_, QStringLiteral("Окно:"), 3, 101, 31);
    windowSlider_->setSingleStep(2);
    addSlider(offsetSlider_, offsetLabel_, offsetTitle_, QStringLiteral("C:"), 0, 64, 10);
    addSlider(radiusSlider_, radiusLabel_, radiusTitle_, QStringLiteral("Радиус:"), 0, 10, 3);
    addSlider(amountSlider_, amountLabel_, amountTitle_, QStringLiteral("Сила:"), 1, 300, 50);
    amountLabel_->setText(QStringLiteral("0.50"));
    addSlider(blackSlider_, blackLabel_, blackTitle_, QStringLiteral("Чёрная:"), 0, 254, 0);
    addSlider(whiteSlider_, whiteLabel_, whiteTitle_, QStringLiteral("Белая:"), 1, 255, 255);

    edgeTitle_ = new QLabel(QStringLiteral("Края:"), paramRow);
    pl->addWidget(edgeTitle_);
    edgeBox_ = new QComboBox(paramRow);
    edgeBox_->addItem(QStringLiteral("Clamp"));
    edgeBox_->addItem(QStringLiteral("Reflect"));
    pl->addWidget(edgeBox_);
    pl->addStretch(1);

    connect(thresholdSlider_, &QSlider::valueChanged, this,
            [this](int v) { thresholdLabel_->setText(QString::number(v)); });
    connect(windowSlider_, &QSlider::valueChanged, this, [this](int v) {
        if (v % 2 == 0) {
            v = std::max(3, v - 1);
            windowSlider_->blockSignals(true);
            windowSlider_->setValue(v);
            windowSlider_->blockSignals(false);
        }
        windowLabel_->setText(QString::number(v));
    });
    connect(offsetSlider_, &QSlider::valueChanged, this, [this](int v) { offsetLabel_->setText(QString::number(v)); });
    connect(radiusSlider_, &QSlider::valueChanged, this, [this](int v) { radiusLabel_->setText(QString::number(v)); });
    connect(amountSlider_, &QSlider::valueChanged, this,
            [this](int v) { amountLabel_->setText(QString::number(v / 100.0, 'f', 2)); });
    connect(blackSlider_, &QSlider::valueChanged, this, [this](int v) {
        blackLabel_->setText(QString::number(v));
        if (v >= whiteSlider_->value()) whiteSlider_->setValue(std::min(255, v + 1));
    });
    connect(whiteSlider_, &QSlider::valueChanged, this, [this](int v) {
        whiteLabel_->setText(QString::number(v));
        if (v <= blackSlider_->value()) blackSlider_->setValue(std::max(0, v - 1));
    });
    connect(methodBox_, &QComboBox::currentIndexChanged, this, [this](int) {
        updateParameterVisibility();
        applyCurrent();
    });

    auto* central = new QWidget(this);
    auto* grid = new QGridLayout(central);
    grid->setContentsMargins(6, 6, 6, 6);
    beforeLabel_ = new QLabel(QStringLiteral("Откройте изображение…"), central);
    afterLabel_ = new QLabel(QStringLiteral("Результат появится здесь"), central);
    beforeLabel_->setMinimumSize(430, 320);
    afterLabel_->setMinimumSize(430, 320);
    beforeLabel_->setAlignment(Qt::AlignCenter);
    afterLabel_->setAlignment(Qt::AlignCenter);
    beforeLabel_->setFrameShape(QFrame::StyledPanel);
    afterLabel_->setFrameShape(QFrame::StyledPanel);
    histBefore_ = new HistogramWidget(central);
    histAfter_ = new HistogramWidget(central);
    auto* leftBox = new QVBoxLayout();
    leftBox->addWidget(new QLabel(QStringLiteral("До:"), central), 0, Qt::AlignLeft);
    leftBox->addWidget(beforeLabel_, 1);
    leftBox->addWidget(histBefore_);
    auto* rightBox = new QVBoxLayout();
    rightBox->addWidget(new QLabel(QStringLiteral("После:"), central), 0, Qt::AlignLeft);
    rightBox->addWidget(afterLabel_, 1);
    rightBox->addWidget(histAfter_);
    grid->addLayout(leftBox, 0, 0);
    grid->addLayout(rightBox, 0, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    setCentralWidget(central);

    auto* helpMenu = menuBar()->addMenu(QStringLiteral("&Справка"));
    QAction* theory = helpMenu->addAction(QStringLiteral("Как работают методы…"));
    connect(theory, &QAction::triggered, this, &MainWindow::showTheoryHelp);
    QAction* about = helpMenu->addAction(QStringLiteral("О программе"));
    connect(about, &QAction::triggered, this, [this] {
        QMessageBox::information(this, QStringLiteral("О программе"),
            QStringLiteral("Лабораторная работа 3: обработка цифровых изображений (вариант 7).\n\n"
                           "Методы: низкочастотные сглаживающие фильтры (гаусс, прямоугольный);\n"
                           "глобальная пороговая обработка (ручной порог и метод Оцу);\n"
                           "адаптивная пороговая обработка (среднее в окне минус C).\n\n"
                           "Все алгоритмы написаны с нуля на C++ (слой model), интерфейс — Qt.\n"
                           "Обработка выполняется в фоновом потоке, интерфейс не блокируется."));
    });

    statusBar()->showMessage(QStringLiteral("Откройте изображение (PPM/PNG/BMP/JPEG) и выберите метод."));

    connect(&controller_, &ProcessingController::originalLoaded, this, &MainWindow::onOriginalLoaded);
    connect(&controller_, &ProcessingController::loadFailed, this, &MainWindow::onLoadFailed);
    connect(&controller_, &ProcessingController::processFinished, this, &MainWindow::onProcessFinished);
    connect(&controller_, &ProcessingController::processFailed, this, &MainWindow::onProcessFailed);
    updateParameterVisibility();
}

void MainWindow::updateParameterVisibility() {
    const MethodSpec& spec = kMethods[methodBox_->currentIndex()];
    auto set = [&](QWidget* w, bool show) { w->setVisible(show); };
    set(thresholdSlider_, spec.threshold);
    set(thresholdLabel_, spec.threshold);
    set(thresholdTitle_, spec.threshold);
    set(windowSlider_, spec.window);
    set(windowLabel_, spec.window);
    set(windowTitle_, spec.window);
    set(offsetSlider_, spec.offset);
    set(offsetLabel_, spec.offset);
    set(offsetTitle_, spec.offset);
    set(radiusSlider_, spec.radius);
    set(radiusLabel_, spec.radius);
    set(radiusTitle_, spec.radius);
    set(amountSlider_, spec.amount);
    set(amountLabel_, spec.amount);
    set(amountTitle_, spec.amount);
    set(blackSlider_, spec.black);
    set(blackLabel_, spec.black);
    set(blackTitle_, spec.black);
    set(whiteSlider_, spec.white);
    set(whiteLabel_, spec.white);
    set(whiteTitle_, spec.white);
    set(edgeBox_, spec.edges);
    set(edgeTitle_, spec.edges);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    event->accept();
}

void MainWindow::browse() {
    const QString file = QFileDialog::getOpenFileName(
        this, QStringLiteral("Открыть изображение"), pathEdit_->text().trimmed(),
        QStringLiteral("Изображения (*.ppm *.png *.bmp *.jpg *.jpeg *.tif *.tiff);;Все файлы (*)"));
    if (file.isEmpty()) return;
    QString err;
    if (!controller_.loadOriginal(file, &err)) {
        statusBar()->showMessage(QStringLiteral("Ошибка открытия: %1").arg(err));
        return;
    }
}

void MainWindow::saveResult() {
    if (!controller_.hasOriginal()) return;
    const QString file = QFileDialog::getSaveFileName(
        this, QStringLiteral("Сохранить результат"), QStringLiteral("result.png"),
        QStringLiteral("PNG (*.png);;JPEG (*.jpg);;BMP (*.bmp);;PPM (*.ppm)"));
    if (file.isEmpty()) return;
    QString err;
    if (!controller_.saveResult(file, &err)) {
        QMessageBox::warning(this, QStringLiteral("Сохранение"), err);
    } else {
        statusBar()->showMessage(QStringLiteral("Сохранено: %1").arg(file));
    }
}

void MainWindow::applyCurrent() {
    if (!controller_.hasOriginal()) {
        const QString p = pathEdit_->text().trimmed();
        if (!p.isEmpty()) {
            QString err;
            if (!controller_.loadOriginal(p, &err)) {
                statusBar()->showMessage(QStringLiteral("Ошибка открытия: %1").arg(err));
                return;
            }
        } else {
            return;
        }
    }
    controller_.apply(paramsFor(methodBox_->currentIndex(), thresholdSlider_->value(), windowSlider_->value(),
                                offsetSlider_->value(), radiusSlider_->value(), amountSlider_->value(),
                                blackSlider_->value(), whiteSlider_->value(), edgeBox_->currentIndex() == 1));
}

void MainWindow::onOriginalLoaded(int width, int height) {
    pathEdit_->setText(controller_.original().w() > 0 ? pathEdit_->text() : pathEdit_->text());
    setPixmapScaled(beforeLabel_, controller_.original());
    afterLabel_->setText(QStringLiteral("Нажмите «Применить»"));
    refreshHistograms();
    histAfter_->clear();
    statusBar()->showMessage(QStringLiteral("Загружено: %1 x %2 пикселей").arg(width).arg(height));
}

void MainWindow::onLoadFailed(const QString& message) {
    QMessageBox::warning(this, QStringLiteral("Открытие файла"), message);
}

void MainWindow::onProcessFinished(double ms, const QString& method) {
    Q_UNUSED(method);
    setPixmapScaled(afterLabel_, controller_.result());
    refreshHistograms();
    statusBar()->showMessage(QStringLiteral("Метод выполнен за %1 мс").arg(ms, 0, 'f', 1));
    if (!smokePath_.isEmpty() && !smokeShot_) {
        smokeShot_ = true;
        QTimer::singleShot(400, this, [this] {
            grab().save(smokePath_);
            QApplication::quit();
        });
    }
}

void MainWindow::onProcessFailed(const QString& message) {
    QMessageBox::warning(this, QStringLiteral("Обработка"), message);
}

void MainWindow::refreshHistograms() {
    if (!controller_.hasOriginal()) return;
    auto chB = imgproc::histogramChannels(controller_.original());
    histBefore_->setChannelHistograms(chB[0], chB[1], chB[2], QStringLiteral("Гистограмма: до (RGB-каналы)"));
    if (!controller_.result().empty()) {
        auto chA = imgproc::histogramChannels(controller_.result());
        histAfter_->setChannelHistograms(chA[0], chA[1], chA[2], QStringLiteral("Гистограмма: после (RGB-каналы)"));
    }
}

void MainWindow::setPixmapScaled(QLabel* label, const imgproc::Image8& image) {
    QImage img(image.w(), image.h(), QImage::Format_RGB888);
    for (int y = 0; y < image.h(); ++y) {
        std::memcpy(img.scanLine(y), image.row(y), static_cast<size_t>(image.w()) * 3);
    }
    label->setPixmap(QPixmap::fromImage(img).scaled(label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::loadAndApplyForSmoke(const QString& imagePath, const QString& screenshotPath) {
    smokePath_ = screenshotPath;
    QString err;
    if (!controller_.loadOriginal(imagePath, &err)) {
        QApplication::quit();
        return;
    }
    const QStringList parts = smokePath_.split(QLatin1Char('|'));
    smokePath_ = parts.value(0);
    ProcessParams p;
    p.method = parts.value(1, QStringLiteral("otsu"));
    methodBox_->setCurrentIndex(methodIndex(p.method));
    updateParameterVisibility();
    if (p.method == QStringLiteral("threshold")) {
        p.threshold = parts.value(2, QStringLiteral("128")).toInt();
    } else if (p.method == QStringLiteral("adaptive")) {
        p.windowSize = parts.value(2, QStringLiteral("31")).toInt();
        p.offsetC = parts.value(3, QStringLiteral("10")).toInt();
    } else if (p.method == QStringLiteral("linear")) {
        p.blackPoint = parts.value(2, QStringLiteral("0")).toInt();
        p.whitePoint = parts.value(3, QStringLiteral("255")).toInt();
    } else if (p.method == QStringLiteral("gauss") || p.method == QStringLiteral("box")) {
        p.radius = parts.value(2, QStringLiteral("3")).toInt();
    }
    controller_.apply(p);
}

void MainWindow::showUserGuide() {
    QMessageBox::information(this, QStringLiteral("Руководство по использованию"),
        QStringLiteral(
            "<h3>Как пользоваться программой</h3>"
            "<p><b>1.</b> Нажмите «Открыть…» и выберите изображение (PPM, PNG, BMP, JPEG, TIFF).</p>"
            "<p><b>2.</b> Выберите метод: первые три — пороговая обработка (ручной порог, Оцу, "
            "адаптивная), далее сглаживающие фильтры (гаусс, прямоугольный), затем дополнительные "
            "методы. Параметры меняются ползунками под панелью инструментов — видны только нужные "
            "для выбранного метода. Применение — кнопка «Применить» (при смене метода автоматически).</p>"
            "<p><b>3.</b> Для сглаживающих фильтров выбирается способ обработки краёв: Clamp "
            "(дублирование граничного пикселя) или Reflect (зеркальное отражение).</p>"
            "<p><b>4.</b> Слева — исходное изображение и его гистограмма, справа — результат. "
            "Для пороговых методов смотрите гистограмму: у «хороших» для Оцу изображений два "
            "выраженных пика.</p>"
            "<p><b>5.</b> «Сохранить результат…» записывает картинку в PNG/JPEG/BMP/PPM. "
            "Формулы всех методов — «Справка → Как работают методы…».</p>"));
}

void MainWindow::showTheoryHelp() {
    QMessageBox::information(this, QStringLiteral("Как работают методы (вариант 7)"),
        QStringLiteral(
            "<h3>Низкочастотные фильтры и пороговая обработка</h3>"
            "<p><b>Свёртка.</b> Каждый пиксель заменяется взвешенной суммой окрестности: "
            "g(x,y) = ΣΣ K(i,j)·f(x+i−r, y+j−r). У краёв окрестность выходит за картинку, поэтому "
            "вне диапазона пиксели берутся: <b>Clamp</b> — дублируется ближайший граничный пиксель, "
            "<b>Reflect</b> — изображение зеркально отражается.</p>"
            "<p><b>Прямоугольный (box) фильтр.</b> Ядро (2r+1)×(2r+1) с равными весами 1/(2r+1)² — "
            "простое среднее по окну. Сильное сглаживание, но «блочные» края.</p>"
            "<p><b>Гауссов фильтр.</b> Веса по нормальному распределению: K(i) ∝ exp(−d²/2σ²), "
            "σ = радиус/3. Мягче и естественнее box; считается разделяемо — два прохода одномерным "
            "ядром (строки, потом столбцы), что даёт тот же результат при меньшей сложности.</p>"
            "<p><b>Глобальная пороговая обработка.</b> Изображение переводится в яркости "
            "(0,299R+0,587G+0,114B) и бинаризуется: пиксель &gt; порога — белый, иначе чёрный.</p>"
            "<p><b>Ручной порог.</b> Порог задаёт пользователь ползунком — наглядно, но не "
            "универсально: на другой картинке то же значение может не работать.</p>"
            "<p><b>Метод Оцу.</b> Перебирает все 256 порогов и выбирает тот, при котором "
            "межклассовая дисперсия σ²(t) = ω₀(t)·ω₁(t)·(μ₀(t)−μ₁(t))² максимальна, где ω — доли "
            "пикселей «фона» и «объекта», μ — их средние яркости. Работает, когда гистограмма "
            "бимодальна (два выраженных пика).</p>"
            "<p><b>Адаптивная пороговая обработка (среднее − C).</b> Порог вычисляется для каждого "
            "пикселя отдельно: T(x,y) = средняя яркость в скользящем окне − C. Спасает при "
            "неравномерном освещении, где глобальные пороги дают чёрные заливки в тёмных областях. "
            "В программе окно считается через интегральное изображение — сумма яркостей по прямоугольнику "
            "за O(1), поэтому размер окна не замедляет обработку. Ключевой параметр C задаёт "
            "«запас» яркости, отсекающий ровные области.</p>"));
}
