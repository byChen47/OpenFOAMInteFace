#include "core/CaseProject.h"
#include "core/CaseSourceInfo.h"
#include "core/CaseValidator.h"

#include <QtTest>

#include <cstdint>
#include <string>

namespace ofgui::core {
namespace {

Sha256Digest makeDigest(std::uint8_t seed)
{
    Sha256Digest digest{};
    for (std::size_t i = 0; i < digest.size(); ++i)
    {
        digest[i] = static_cast<std::uint8_t>(seed + i);
    }
    return digest;
}

class CaseSourceInfoTest final : public QObject
{
    Q_OBJECT

private slots:
    void formatsDigestAsLowercaseHex();
    void roundTripsDigestThroughHex();
    void acceptsUppercaseHex();
    void rejectsInvalidHex();
    void keepsDigestUntouchedWhenParsingFails();
    void sourceInfoStartsEmpty();
    void sourceInfoStoresMetadata();
    void managesFileHashes();
    void caseProjectHasNoSourceByDefault();
    void caseProjectStoresAndClearsSource();
    void sourceDoesNotAffectValidation();
};

void CaseSourceInfoTest::formatsDigestAsLowercaseHex()
{
    const Sha256Digest zeros{};
    QCOMPARE(QString::fromStdString(sha256ToHex(zeros)),
        QString(static_cast<int>(kSha256HexLength), QLatin1Char('0')));

    Sha256Digest filled{};
    filled.fill(0xAB);
    const QString hex = QString::fromStdString(sha256ToHex(filled));
    QCOMPARE(hex.size(), static_cast<int>(kSha256HexLength));
    QCOMPARE(hex, QStringLiteral("ab").repeated(static_cast<int>(kSha256ByteCount)));
}

void CaseSourceInfoTest::roundTripsDigestThroughHex()
{
    const Sha256Digest digest = makeDigest(0x10);
    Sha256Digest parsed{};
    QVERIFY(sha256FromHex(sha256ToHex(digest), parsed));
    QCOMPARE(parsed, digest);
}

void CaseSourceInfoTest::acceptsUppercaseHex()
{
    const Sha256Digest digest = makeDigest(0x5A);
    std::string hex = sha256ToHex(digest);
    for (char& c : hex)
    {
        if (c >= 'a' && c <= 'f')
        {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }

    Sha256Digest parsed{};
    QVERIFY(sha256FromHex(hex, parsed));
    QCOMPARE(parsed, digest);
}

void CaseSourceInfoTest::rejectsInvalidHex()
{
    Sha256Digest parsed{};
    QVERIFY(!sha256FromHex("", parsed));
    QVERIFY(!sha256FromHex("0", parsed));
    QVERIFY(!sha256FromHex(std::string(kSha256HexLength - 1, 'a'), parsed));
    QVERIFY(!sha256FromHex(std::string(kSha256HexLength + 1, 'a'), parsed));
    QVERIFY(!sha256FromHex(std::string(kSha256HexLength, 'g'), parsed));
    QVERIFY(!sha256FromHex("0x" + std::string(kSha256HexLength - 2, 'a'), parsed));
    QVERIFY(!sha256FromHex(std::string(kSha256HexLength - 1, 'a') + " ", parsed));
}

void CaseSourceInfoTest::keepsDigestUntouchedWhenParsingFails()
{
    Sha256Digest digest = makeDigest(0x20);
    const Sha256Digest before = digest;

    QVERIFY(!sha256FromHex("not-a-digest", digest));
    QCOMPARE(digest, before);
}

void CaseSourceInfoTest::sourceInfoStartsEmpty()
{
    const CaseSourceInfo info;
    QVERIFY(info.isEmpty());
    QCOMPARE(static_cast<int>(info.fileCount()), 0);
    QVERIFY(info.findFile("system/blockMeshDict") == nullptr);
}

void CaseSourceInfoTest::sourceInfoStoresMetadata()
{
    CaseSourceInfo info;
    info.sourceRelativePath = "cases/cavity";
    info.startDirectory = "0.orig";
    info.importedAt = "2026-10-27T15:04:05Z";

    QVERIFY(!info.isEmpty());
    QCOMPARE(QString::fromStdString(info.sourceRelativePath), QStringLiteral("cases/cavity"));
    QCOMPARE(QString::fromStdString(info.startDirectory), QStringLiteral("0.orig"));
    QCOMPARE(QString::fromStdString(info.importedAt), QStringLiteral("2026-10-27T15:04:05Z"));
}

void CaseSourceInfoTest::managesFileHashes()
{
    CaseSourceInfo info;

    const Sha256Digest first = makeDigest(0x01);
    const Sha256Digest second = makeDigest(0x80);
    QVERIFY(info.setFileHash("0/p", sha256ToHex(first)));
    QVERIFY(info.setFileHash("0/U", sha256ToHex(second)));
    QCOMPARE(static_cast<int>(info.fileCount()), 2);
    QCOMPARE(static_cast<int>(info.files.size()), 2);

    const SourceFileHash* pFile = info.findFile("0/p");
    QVERIFY(pFile != nullptr);
    QCOMPARE(pFile->sha256, first);
    QCOMPARE(QString::fromStdString(pFile->sha256Hex()), QString::fromStdString(sha256ToHex(first)));
    QVERIFY(info.findFile("0/k") == nullptr);

    // Updating an existing path must not append a duplicate entry.
    QVERIFY(info.setFileHash("0/p", sha256ToHex(second)));
    QCOMPARE(static_cast<int>(info.fileCount()), 2);
    QCOMPARE(info.findFile("0/p")->sha256, second);

    // Invalid hex never reaches storage.
    QVERIFY(!info.setFileHash("0/k", "zz"));
    QVERIFY(info.findFile("0/k") == nullptr);
    QCOMPARE(static_cast<int>(info.fileCount()), 2);

    QVERIFY(info.removeFile("0/U"));
    QVERIFY(!info.removeFile("0/U"));
    QCOMPARE(static_cast<int>(info.fileCount()), 1);
    QVERIFY(info.findFile("0/U") == nullptr);
}

void CaseSourceInfoTest::caseProjectHasNoSourceByDefault()
{
    const CaseProject project("cavity");
    QVERIFY(project.source().isEmpty());
    QVERIFY(!project.hasSource());
}

void CaseSourceInfoTest::caseProjectStoresAndClearsSource()
{
    CaseProject project("cavity");

    CaseSourceInfo info;
    info.sourceRelativePath = "cases/cavity";
    info.startDirectory = "0";
    info.importedAt = "2026-10-27T15:04:05Z";
    QVERIFY(info.setFileHash("system/controlDict", sha256ToHex(makeDigest(0x33))));
    project.setSource(info);

    QVERIFY(project.hasSource());
    QCOMPARE(QString::fromStdString(project.source().sourceRelativePath), QStringLiteral("cases/cavity"));
    QCOMPARE(static_cast<int>(project.source().fileCount()), 1);
    QVERIFY(project.source().findFile("system/controlDict") != nullptr);

    // Mutable access is the importer's path once the project is handed over.
    project.source().setFileHash("system/fvSchemes", sha256ToHex(makeDigest(0x44)));
    QCOMPARE(static_cast<int>(project.source().fileCount()), 2);

    project.setSource(CaseSourceInfo{});
    QVERIFY(!project.hasSource());
    QVERIFY(project.source().isEmpty());
}

void CaseSourceInfoTest::sourceDoesNotAffectValidation()
{
    CaseProject project("cavity");
    const CaseValidator validator;
    const std::string before = validator.validate(project).summary();

    CaseSourceInfo info;
    info.sourceRelativePath = "cases/cavity";
    info.startDirectory = "0.orig";
    QVERIFY(info.setFileHash("0/p", sha256ToHex(makeDigest(0x55))));
    project.setSource(info);

    QCOMPARE(validator.validate(project).summary(), before);
    QVERIFY(project.hasSource());
}

} // namespace
} // namespace ofgui::core

QTEST_APPLESS_MAIN(ofgui::core::CaseSourceInfoTest)

#include "CaseSourceInfoTest.moc"
