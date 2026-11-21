#include <QApplication>
#include <QTimer>
#include <QCommandLineParser>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addOption({"smoke-test", "Run headless smoke test and exit"});
    parser.process(app);

    bool smokeTest = parser.isSet("smoke-test") || qEnvironmentVariableIsSet("ALLEGROSCORE_SMOKE_TEST");

    MainWindow window;
    window.show();

    if (smokeTest) {
        QTimer::singleShot(200, &app, &QApplication::quit);
    }

    return app.exec();
}
