#include "app/SmokeTestController.h"

#include <QApplication>
#include <QPixmap>
#include <QScreen>
#include <QStringList>
#include <QTimer>
#include <QWidget>

namespace ofgui {
namespace {

constexpr int kSmokeTestDelayMs = 1500;

QString screenshotPathFrom(const QStringList& arguments)
{
    const int optionIndex = arguments.indexOf(QStringLiteral("--screenshot"));
    if (optionIndex < 0 || optionIndex + 1 >= arguments.size())
    {
        return {};
    }

    return arguments.at(optionIndex + 1);
}

} // namespace

void scheduleSmokeTest(QApplication& application, QWidget& window, const QStringList& arguments)
{
    const QString screenshotPath = screenshotPathFrom(arguments);
    const bool smokeTestRequested =
        arguments.contains(QStringLiteral("--smoke-test")) || !screenshotPath.isEmpty();

    if (!smokeTestRequested)
    {
        return;
    }

    QTimer::singleShot(kSmokeTestDelayMs, &application,
        [&application, &window, screenshotPath]()
        {
            if (screenshotPath.isEmpty())
            {
                application.quit();
                return;
            }

            QScreen* screen = window.screen();
            if (screen == nullptr)
            {
                application.exit(2);
                return;
            }

            const QPixmap screenshot = screen->grabWindow(window.winId());
            application.exit(screenshot.save(screenshotPath, "PNG") ? 0 : 3);
        });
}

} // namespace ofgui