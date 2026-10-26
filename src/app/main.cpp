#include "app/ApplicationBootstrap.h"
#include "app/SmokeTestController.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char* argv[])
{
    ofgui::configureOpenGLSurfaceFormat();

    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("OpenFOAMGUI"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QCoreApplication::setOrganizationName(QStringLiteral("OpenFOAMGUI"));

    ofgui::MainWindow window;
    window.show();

    ofgui::scheduleSmokeTest(application, window, QCoreApplication::arguments());

    return application.exec();
}