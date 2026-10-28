#include "core/Version.h"

#include <QString>
#include <QtTest>

#include <string>

namespace ofgui::core {
namespace {

class VersionTest final : public QObject
{
    Q_OBJECT

private slots:
    void exposesModuleIdentity();
    void formatsSemanticVersion();
    void bannerIsNamePlusVersion();
};

void VersionTest::exposesModuleIdentity()
{
    QCOMPARE(QString::fromStdString(std::string{kCoreModuleName}), QStringLiteral("ofgui_core"));
    QVERIFY(kCoreModuleVersion.major >= 0);
    QVERIFY(kCoreModuleVersion.minor >= 0);
    QVERIFY(kCoreModuleVersion.patch >= 0);
}

void VersionTest::formatsSemanticVersion()
{
    QCOMPARE(
        QString::fromStdString(formatModuleVersion(ModuleVersion{2, 11, 4})),
        QStringLiteral("2.11.4"));
    QCOMPARE(
        QString::fromStdString(formatModuleVersion(ModuleVersion{0, 0, 0})),
        QStringLiteral("0.0.0"));
}

void VersionTest::bannerIsNamePlusVersion()
{
    const QString expected = QStringLiteral("ofgui_core ")
                             + QString::fromStdString(formatModuleVersion(kCoreModuleVersion));

    QCOMPARE(QString::fromStdString(coreModuleBanner()), expected);
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::VersionTest)

#include "VersionTest.moc"
