#include "io/Version.h"

#include "core/Version.h"

#include <QString>
#include <QtTest>

#include <string>

namespace ofgui::io {
namespace {

class IoVersionTest final : public QObject
{
    Q_OBJECT

private slots:
    void exposesModuleIdentity();
    void bannerReusesCoreVersionFormatter();
};

void IoVersionTest::exposesModuleIdentity()
{
    QCOMPARE(QString::fromStdString(std::string{kIoModuleName}), QStringLiteral("ofgui_io"));
    QVERIFY(kIoModuleVersion.major >= 0);
    QVERIFY(kIoModuleVersion.minor >= 0);
    QVERIFY(kIoModuleVersion.patch >= 0);
}

void IoVersionTest::bannerReusesCoreVersionFormatter()
{
    const QString expected = QStringLiteral("ofgui_io ")
                             + QString::fromStdString(ofgui::core::formatModuleVersion(kIoModuleVersion));

    QCOMPARE(QString::fromStdString(ioModuleBanner()), expected);
}

} // namespace
} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::IoVersionTest)

#include "VersionTest.moc"
