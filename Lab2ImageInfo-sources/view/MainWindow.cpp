#include "MainWindow.h"

#include "ImageInfo.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCloseEvent>
#include <QColor>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QStatusBar>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextBrowser>
#include <QTimer>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>

namespace {

const char* kColumnTips[] = {
    "Имя файла — имя файла в отсканированной папке.",
    "Формат — определяется по сигнатуре (magic-байтам) в начале файла, а не по расширению:\n"
    "BMP «BM», PNG 89 50 4E 47, GIF «GIF8», JPEG FF D8 FF, TIFF «II*\0»/«MM\0*», PCX 0x0A.",
    "Размер изображения (в пикселях) — ширина x высота растра.\n"
    "BMP: поля biWidth/biHeight из BITMAPINFOHEADER (смещения 18 и 22 от начала файла).\n"
    "PNG: поля width/height из чанка IHDR (байты 16-23).\n"
    "GIF: размеры логического экрана в дескрипторе экрана (байты 6-9).\n"
    "JPEG: поля высоты/ширины из маркера SOFn (FFC0-FFCF).\n"
    "TIFF: теги ImageWidth (256) и ImageLength (257) из IFD.\n"
    "PCX: разности xmax-xmin+1 и ymax-ymin+1 из заголовка (байты 4-11).",
    "Разрешение (dot/inch) — плотность пикселей при выводе; нужна, например, при печати.\n"
    "BMP: biXPelsPerMeter/biYPelsPerMeter (пикселей на метр), перевод в dpi: значение x 0,0254.\n"
    "PNG: чанк pHYs (пикселей на метр, unit=1).\n"
    "JPEG: плотность JFIF (APP0: единицы dpi или dpcm) либо теги XResolution/YResolution из EXIF (APP1).\n"
    "TIFF: теги XResolution (282)/YResolution (283) — рациональные числа, единицы из ResolutionUnit (296).\n"
    "GIF разрешение не хранит — «—»; у PCX поля hres/vres задают разрешение устройства-источника (часто 0).",
    "Глубина цвета — число бит на пиксель: сколько бит кодирует цвет одного пикселя.\n"
    "BMP: biBitCount (1/4/8/16/24/32).\n"
    "PNG: бит на канал (IHDR) x число каналов по типу цвета (RGB=3, RGBA=4...).\n"
    "GIF: биты палитры из дескриптора экрана/изображения (палитра до 256 цветов).\n"
    "JPEG: точность (обычно 8 бит/канал) x число компонент (YCbCr = 24).\n"
    "TIFF: сумма BitsPerSample (258); для палитровых (Photometric=3) — разрядность индекса.\n"
    "PCX: биты на плоскость x число плоскостей (1 плоскость — палитра, 3 — истинный цвет).",
    "Сжатие — способ упаковки пиксельных данных.\n"
    "BMP: biCompression: BI_RGB — без сжатия, BI_RLE8/BI_RLE4 — RLE, BI_BITFIELDS — битовые маски.\n"
    "PNG: Deflate (zlib), метод 0 из IHDR + фильтрация строк.\n"
    "GIF: LZW всегда.\n"
    "JPEG: DCT с потерями; baseline/progressive различаются по маркеру SOF.\n"
    "TIFF: тег Compression (259): без сжатия, LZW, PackBits, Deflate, JPEG и др.\n"
    "PCX: RLE по строкам (поле encoding = 1).",
    "Размер файла — фактический размер файла на диске; точное число байтов — в подсказке ячейки.",
    "Статус — результат проверки целостности файла:\n"
    "OK — структура корректна; OK (расширение не совпадает) — содержимое другого формата;\n"
    "«не изображение» — сигнатура не распознана (в т.ч. текст, переименованный в .jpg);\n"
    "«повреждён» — нет IEND у PNG, нет EOI у JPEG, файл короче, чем заявлено в заголовке,\n"
    "обрезаны блоки GIF/PCX, IFD за пределами файла и т.п.; «ошибка чтения» — файл не открылся.",
};

QString statusText(const img::ImageInfo& info) {
    if (info.ok && info.renamed) return QStringLiteral("OK (расширение не совпадает)");
    if (info.ok) return QStringLiteral("OK");
    if (info.format == img::Format::NotImage) return QStringLiteral("Не изображение");
    if (info.status.rfind("не удалось", 0) == 0) return QStringLiteral("Ошибка чтения");
    return QStringLiteral("Повреждён");
}

}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Лабораторная работа 2 — Информация о графических файлах"));
    resize(1180, 720);

    model_ = new QStandardItemModel(0, 8, this);
    const QStringList headers = {QStringLiteral("Имя файла"), QStringLiteral("Формат"),
                                 QStringLiteral("Размер (пикс)"), QStringLiteral("Разрешение (dpi)"),
                                 QStringLiteral("Глубина цвета"), QStringLiteral("Сжатие"),
                                 QStringLiteral("Размер файла"), QStringLiteral("Статус")};
    for (int c = 0; c < headers.size(); ++c) {
        QStandardItem* h = new QStandardItem(headers[c]);
        h->setToolTip(QString::fromUtf8(kColumnTips[c]));
        model_->setHorizontalHeaderItem(c, h);
    }

    table_ = new QTableView(this);
    table_->setModel(model_);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->verticalHeader()->setVisible(false);
    table_->verticalHeader()->setDefaultSectionSize(22);
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    const int widths[] = {70, 110, 130, 230, 230, 90, 190};
    for (int c = 1; c < 8; ++c) table_->setColumnWidth(c, widths[c - 1]);
    table_->setSortingEnabled(true);
    connect(table_, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex& idx) {
        const QString path = model_->item(idx.row(), 0)->data(Qt::UserRole + 1).toString();
        if (!path.isEmpty()) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });

    details_ = new QTextBrowser(this);
    details_->setMaximumHeight(170);
    details_->setOpenExternalLinks(false);
    details_->setPlaceholderText(QStringLiteral("Выберите строку — здесь появятся дополнительные сведения о файле "
                                               "(палитры, матрицы квантования, версии форматов и т.д.)."));

    auto* central = new QWidget(this);
    auto* lay = new QVBoxLayout(central);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addWidget(table_, 1);
    lay->addWidget(details_);
    setCentralWidget(central);

    auto* tb = addToolBar(QStringLiteral("main"));
    tb->setMovable(false);
    tb->addWidget(new QLabel(QStringLiteral(" Папка: "), tb));
    folderEdit_ = new QLineEdit(tb);
    folderEdit_->setMinimumWidth(380);
    folderEdit_->setPlaceholderText(QStringLiteral("Путь к папке с изображениями…"));
    connect(folderEdit_, &QLineEdit::returnPressed, this, &MainWindow::startScan);
    tb->addWidget(folderEdit_);
    auto* browseBtn = new QPushButton(QStringLiteral("Обзор…"), tb);
    connect(browseBtn, &QPushButton::clicked, this, &MainWindow::browse);
    tb->addWidget(browseBtn);
    scanButton_ = new QPushButton(QStringLiteral("Сканировать"));
    scanButton_->setDefault(true);
    connect(scanButton_, &QPushButton::clicked, this, &MainWindow::startScan);
    tb->addWidget(scanButton_);
    stopButton_ = new QPushButton(QStringLiteral("Стоп"));
    stopButton_->setEnabled(false);
    connect(stopButton_, &QPushButton::clicked, this, &MainWindow::stopScan);
    tb->addWidget(stopButton_);
    progressBar_ = new QProgressBar(tb);
    progressBar_->setFixedWidth(220);
    progressBar_->setVisible(false);
    tb->addWidget(progressBar_);
    progressLabel_ = new QLabel(tb);
    tb->addWidget(progressLabel_);

    auto* guideBtn = new QPushButton(QStringLiteral("i"), tb);
    guideBtn->setFixedSize(30, 30);
    guideBtn->setToolTip(QStringLiteral("Краткое руководство по использованию"));
    connect(guideBtn, &QPushButton::clicked, this, &MainWindow::showUserGuide);
    tb->addWidget(guideBtn);

    auto* helpMenu = menuBar()->addMenu(QStringLiteral("&Справка"));
    QAction* fieldsHelp = helpMenu->addAction(QStringLiteral("Откуда берутся данные…"));
    connect(fieldsHelp, &QAction::triggered, this, &MainWindow::showFieldsHelp);
    QAction* about = helpMenu->addAction(QStringLiteral("О программе"));
    connect(about, &QAction::triggered, this, [this] {
        QMessageBox::information(this, QStringLiteral("О программе"),
            QStringLiteral("Лабораторная работа 2: чтение информации из графических файлов (BMP, PNG, GIF, JPEG, "
                           "TIFF, PCX).\n\nМетаданные извлекаются напрямую из заголовков файлов по спецификациям "
                           "форматов, без сторонних библиотек. Чтение «ленивое»: открыв файл, программа считывает "
                           "только первые байты (заголовки и нужные смещения), пиксельные данные не читаются.\n\n"
                           "Сканирование папки выполняется в пуле потоков (QtConcurrent), интерфейс при этом "
                           "не блокируется, прогресс отображается в панели инструментов.\n\n"
                           "Двойной щелчок по строке открывает файл системным просмотрщиком."));
    });

    statusBar()->showMessage(QStringLiteral("Выберите папку и нажмите «Сканировать»."));

    connect(&scanner_, &FolderScanner::scanStarted, this, &MainWindow::onScanStarted);
    connect(&scanner_, &FolderScanner::progress, this, &MainWindow::onProgress);
    connect(&scanner_, &FolderScanner::rowsReady, this, &MainWindow::onRowsReady);
    connect(&scanner_, &FolderScanner::scanFinished, this, &MainWindow::onScanFinished);
    connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] { updateDetails(); });
}

void MainWindow::closeEvent(QCloseEvent* event) {
    scanner_.cancel();
    event->accept();
}

void MainWindow::startSmoke(const QString& screenshotPath) {
    smokeMode_ = true;
    screenshotPath_ = screenshotPath;
}

void MainWindow::browse() {
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("Выбор папки"),
                                                          folderEdit_->text().trimmed());
    if (!dir.isEmpty()) scanFolder(dir);
}

void MainWindow::startScan() {
    const QString dir = folderEdit_->text().trimmed();
    if (dir.isEmpty()) {
        browse();
        return;
    }
    scanFolder(dir);
}

void MainWindow::stopScan() {
    scanner_.cancel();
}

void MainWindow::scanFolder(const QString& folder) {
    folderEdit_->setText(folder);
    model_->setRowCount(0);
    details_->clear();
    scanner_.start(folder);
}

void MainWindow::onScanStarted(int total) {
    scanButton_->setEnabled(false);
    stopButton_->setEnabled(true);
    folderEdit_->setEnabled(false);
    progressBar_->setVisible(true);
    progressBar_->setRange(0, total);
    progressBar_->setValue(0);
    progressLabel_->setText(QStringLiteral("0 / %1").arg(total));
    statusBar()->showMessage(QStringLiteral("Сканирование…"));
}

void MainWindow::onProgress(int done) {
    progressBar_->setValue(done);
    progressLabel_->setText(QStringLiteral("%1 / %2").arg(done).arg(progressBar_->maximum()));
}

void MainWindow::onRowsReady(const QVector<img::ImageInfo>& rows) {
    const bool sorting = table_->isSortingEnabled();
    table_->setSortingEnabled(false);
    for (const img::ImageInfo& info : rows) addRow(info);
    table_->setSortingEnabled(sorting);
}

void MainWindow::onScanFinished(double seconds, int total, int images, int damaged, int nonImages) {
    scanButton_->setEnabled(true);
    stopButton_->setEnabled(false);
    folderEdit_->setEnabled(true);
    progressBar_->setVisible(false);
    progressLabel_->clear();
    statusBar()->showMessage(
        QStringLiteral("Готово: %1 файлов за %2 с — изображений: %3, повреждено: %4, не изображений: %5")
            .arg(total).arg(seconds, 0, 'f', 2).arg(images).arg(damaged).arg(nonImages));
    if (smokeMode_ && !screenshotPath_.isEmpty()) {
        QTimer::singleShot(400, this, [this] { saveScreenshot(screenshotPath_); });
    }
}

void MainWindow::addRow(const img::ImageInfo& info) {
    QList<QStandardItem*> row;

    auto* nameItem = new QStandardItem(QString::fromStdString(info.fileName));
    nameItem->setData(QString::fromStdString(info.filePath), Qt::UserRole + 1);
    nameItem->setData(QString::fromStdString(info.extra), Qt::UserRole + 2);
    nameItem->setData(QString::fromStdString(info.status), Qt::UserRole + 3);
    row.append(nameItem);

    row.append(new QStandardItem(QString::fromStdString(info.formatName)));

    QString sizeText = QStringLiteral("—");
    if (info.width || info.height) {
        sizeText = QStringLiteral("%1 x %2").arg(info.width).arg(info.height);
    }
    row.append(new QStandardItem(sizeText));

    QString dpiText = QStringLiteral("—");
    if (info.hasResolution) {
        dpiText = QStringLiteral("%1 x %2").arg(info.dpiX, 0, 'f', 1).arg(info.dpiY, 0, 'f', 1);
    }
    row.append(new QStandardItem(dpiText));

    row.append(new QStandardItem(QString::fromStdString(info.bitDepthText)));
    row.append(new QStandardItem(QString::fromStdString(info.compression)));

    auto* sizeItem = new QStandardItem(formatBytes(info.fileSize));
    sizeItem->setToolTip(QStringLiteral("%1 байт").arg(info.fileSize));
    row.append(sizeItem);

    auto* statusItem = new QStandardItem(statusText(info));
    statusItem->setToolTip(QString::fromStdString(info.status));
    if (!info.ok) {
        statusItem->setForeground(QColor(176, 0, 0));
    } else if (info.renamed) {
        statusItem->setForeground(QColor(184, 134, 11));
    }
    row.append(statusItem);

    model_->appendRow(row);
}

void MainWindow::updateDetails() {
    const QModelIndex index = table_->currentIndex();
    if (!index.isValid()) {
        details_->clear();
        return;
    }
    const int r = index.row();
    const QString path = model_->item(r, 0)->data(Qt::UserRole + 1).toString();
    const QString extra = model_->item(r, 0)->data(Qt::UserRole + 2).toString();
    const QString status = model_->item(r, 0)->data(Qt::UserRole + 3).toString();

    QString text;
    text += QStringLiteral("<b>%1</b><br>").arg(model_->item(r, 0)->text().toHtmlEscaped());
    text += QStringLiteral("Путь: %1<br>").arg(path.toHtmlEscaped());
    text += QStringLiteral("Статус: %1 (%2)<br>").arg(model_->item(r, 7)->text(), status.toHtmlEscaped());
    text += QStringLiteral("Формат: %1&nbsp;&nbsp;Размер: %2 пикс&nbsp;&nbsp;Глубина цвета: %3<br>")
                .arg(model_->item(r, 1)->text(), model_->item(r, 2)->text(),
                     model_->item(r, 4)->text().toHtmlEscaped());
    text += QStringLiteral("Разрешение: %1 dpi&nbsp;&nbsp;Сжатие: %2<br>")
                .arg(model_->item(r, 3)->text(), model_->item(r, 5)->text().toHtmlEscaped());
    text += QStringLiteral("Размер файла: %1 байт<br>").arg(model_->item(r, 6)->data(Qt::UserRole).toString());
    if (!extra.isEmpty()) {
        text += QStringLiteral("<hr>%1").arg(extra.toHtmlEscaped().replace('\n', QStringLiteral("<br>")));
    }
    details_->setHtml(text);
}

QString MainWindow::formatBytes(quint64 bytes) {
    if (bytes < 1024) return QStringLiteral("%1 Б").arg(bytes);
    if (bytes < 1024 * 1024) return QStringLiteral("%1 КБ").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024ull * 1024 * 1024) return QStringLiteral("%1 МБ").arg(bytes / 1048576.0, 0, 'f', 1);
    return QStringLiteral("%1 ГБ").arg(bytes / 1073741824.0, 0, 'f', 2);
}

void MainWindow::showUserGuide() {
    QMessageBox::information(this, QStringLiteral("Руководство по использованию"),
        QStringLiteral(
            "<h3>Как пользоваться программой</h3>"
            "<p><b>1.</b> Укажите папку с изображениями в поле «Папка» или кнопкой «Обзор…» "
            "и нажмите <b>«Сканировать»</b> (или Enter в поле пути). "
            "Сканирование идёт в фоне: прогресс виден в панели инструментов, кнопка <b>«Стоп»</b> "
            "прерывает обход.</p>"
            "<p><b>2.</b> В таблице для каждого файла показываются: формат, размер в пикселях, "
            "разрешение (dpi), глубина цвета, сжатие, размер файла и статус. Нажатие на заголовок "
            "столбца сортирует таблицу; назначение каждого столбца — в подсказке при наведении на "
            "заголовок.</p>"
            "<p><b>3.</b> Щелчок по строке выводит в панели под таблицей <b>дополнительные сведения</b>: "
            "палитры, матрицу квантования JPEG, число кадров GIF, версии форматов и т.д.</p>"
            "<p><b>4.</b> Двойной щелчок по строке открывает сам файл в системном просмотрщике.</p>"
            "<p><b>5.</b> Статусы: <b>ОК</b> — структура файла корректна; <b>ОК (расширение не совпадает)</b> — "
            "содержимое другого формата, чем расширение; <b>Не изображение</b> — сигнатура не распознана; "
            "<b>Повреждён</b> — структура нарушена (причина — в подсказке статуса и в панели сведений).</p>"
            "<p><b>6.</b> Поддерживаются форматы BMP, PNG, GIF, JPEG, TIFF, PCX. Подробное пояснение "
            "каждой выводимой характеристики — в меню «Справка → Откуда берутся данные…».</p>"));
}

void MainWindow::showFieldsHelp() {
    QString help;
    help += QStringLiteral("<h3>Что показывает таблица и откуда берутся данные</h3>");
    help += QStringLiteral("<p>Все характеристики читаются напрямую из заголовков файлов по спецификациям "
                           "форматов; сторонние библиотеки не используются. Формат определяется по сигнатуре "
                           "(magic-байтам) в начале файла, поэтому файл с подменённым расширением опознаётся "
                           "по содержимому.</p>");
    for (int c = 0; c < 8; ++c) {
        help += QStringLiteral("<p><b>%1.</b> %2</p>")
                    .arg(model_->horizontalHeaderItem(c)->text(),
                         QString::fromUtf8(kColumnTips[c]).toHtmlEscaped().replace('\n', QStringLiteral("<br>")));
    }
    help += QStringLiteral("<p><b>Дополнительные сведения</b> (панель под таблицей): палитры и их размер, "
                           "версии форматов, матрица квантования JPEG (шкала качества сжатия), число кадров "
                           "GIF, порядок байтов TIFF, межстрочная развёртка PNG — всё извлекается из тех же "
                           "заголовков.</p>");
    QMessageBox::information(this, QStringLiteral("Пояснение характеристик"), help);
}

void MainWindow::saveScreenshot(const QString& path) {
    grab().save(path);
    QApplication::quit();
}
