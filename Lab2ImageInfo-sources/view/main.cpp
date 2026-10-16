#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Lab2ImageInfo"));
    QApplication::setOrganizationName(QStringLiteral("PKG"));
    QApplication::setApplicationVersion(QStringLiteral("1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Информация о графических файлах (лабораторная работа 2)"));
    parser.addHelpOption();
    QCommandLineOption smokeOpt(QStringLiteral("smoke"),
                                QStringLiteral("Отсканировать <folder>, сохранить скриншот в <file> и выйти."),
                                QStringLiteral("smoke"));
    parser.addOption(smokeOpt);
    parser.addPositionalArgument(QStringLiteral("folder"), QStringLiteral("Папка для сканирования при запуске."));
    parser.process(app);

    MainWindow w;
    w.show();

    const QStringList pos = parser.positionalArguments();
    if (parser.isSet(smokeOpt)) {
        const QStringList vals = parser.value(smokeOpt).split(QLatin1Char(';'));
        const QString folder = vals.size() > 0 ? vals[0] : QString();
        const QString shot = vals.size() > 1 ? vals[1] : QStringLiteral("screenshot.png");
        w.startSmoke(shot);
        if (!folder.isEmpty()) w.scanFolder(folder);
    } else if (!pos.isEmpty()) {
        w.scanFolder(pos.first());
    }

    return QApplication::exec();
}
