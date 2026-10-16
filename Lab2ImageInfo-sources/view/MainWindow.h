#pragma once

#include "FolderScanner.h"

#include <QMainWindow>

class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QStandardItemModel;
class QTableView;
class QTextBrowser;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    void scanFolder(const QString& folder);
    void startSmoke(const QString& screenshotPath);
    void saveScreenshot(const QString& path);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void browse();
    void startScan();
    void stopScan();
    void onScanStarted(int total);
    void onProgress(int done);
    void onRowsReady(const QVector<img::ImageInfo>& rows);
    void onScanFinished(double seconds, int total, int images, int damaged, int nonImages);
    void showUserGuide();
    void showFieldsHelp();

private:
    void addRow(const img::ImageInfo& info);
    void updateDetails();
    static QString formatBytes(quint64 bytes);

    FolderScanner scanner_;
    QLineEdit* folderEdit_ = nullptr;
    QPushButton* scanButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QLabel* progressLabel_ = nullptr;
    QTableView* table_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QTextBrowser* details_ = nullptr;
    bool smokeMode_ = false;
    QString screenshotPath_;
};
