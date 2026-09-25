#include <QApplication>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Lab1ColorModels"));
    QApplication::setOrganizationName(QStringLiteral("PKG"));
    MainWindow window;
    window.show();
    return app.exec();
}
