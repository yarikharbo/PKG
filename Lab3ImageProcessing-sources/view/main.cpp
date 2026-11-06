#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Lab3ImageProcessing"));
    QApplication::setOrganizationName(QStringLiteral("PKG"));
    QApplication::setApplicationVersion(QStringLiteral("1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Обработка цифровых изображений (лабораторная работа 3)"));
    parser.addHelpOption();
    QCommandLineOption smokeOpt(QStringLiteral("smoke"),
                                QStringLiteral("Загрузить <image>, применить метод, сохранить скриншот в <file> и выйти."),
                                QStringLiteral("smoke"));
    parser.addOption(smokeOpt);
    parser.addPositionalArgument(QStringLiteral("image"), QStringLiteral("Изображение для загрузки при запуске."));
    parser.process(app);

    MainWindow w;
    w.show();

    if (parser.isSet(smokeOpt)) {
        const QStringList vals = parser.value(smokeOpt).split(QLatin1Char(';'));
        w.loadAndApplyForSmoke(vals.value(0), vals.value(1));
    }

    return QApplication::exec();
}
