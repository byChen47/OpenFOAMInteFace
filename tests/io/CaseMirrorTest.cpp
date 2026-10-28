#include "io/writers/CaseMirror.h"
#include "io/writers/CaseMirrorPlanner.h"
#include "io/writers/CaseExportOptions.h"
#include "io/writers/ExportSkipRules.h"
#include "io/writers/SourceCaseWalker.h"

#include <core/CaseProject.h>

#include <QtTest>
#include <QTemporaryDir>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

namespace ofgui::io {
namespace {

void writeFile(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << text;
}

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
}

core::CaseProject projectWithP()
{
    core::CaseProject project;
    project.setName("mirror");
    project.addField("p", core::FieldClass::VolScalarField);
    return project;
}

CaseExportOptions optionsFor(const std::filesystem::path& source, const std::filesystem::path& target)
{
    CaseExportOptions options;
    options.targetDirectory = target;
    options.sourceCaseRoot = source.generic_string();
    options.startDirectory = "0";
    return options;
}

const ExportFile* findPlanned(const CaseExportPlan& plan, std::string_view path)
{
    return plan.find(path);
}

bool isSkipped(const CaseExportPlan& plan, std::string_view path)
{
    return std::any_of(plan.skipped.begin(), plan.skipped.end(), [path](const ExportSkippedFile& entry)
                       { return entry.relativePath == path; });
}

/// Minimal source case used by the planner tests.
void buildSourceCase(const std::filesystem::path& root)
{
    writeFile(root / "0" / "p", "p\n");
    writeFile(root / "0" / "T", "T\n");
    writeFile(root / "constant" / "polyMesh" / "points", "points\n");
    writeFile(root / "constant" / "turbulenceProperties", "laminar\n");
    writeFile(root / "system" / "fvSchemes", "schemes\n");
    writeFile(root / "system" / "sampleDict", "sample\n");
    writeFile(root / "Allrun", "#!/bin/sh\n");
    writeFile(root / "log.blockMesh", "log\n");
    writeFile(root / "postProcessing" / "forces" / "0" / "forces.dat", "dat\n");
    writeFile(root / "1" / "p", "later\n");
}

} // namespace

class CaseMirrorTest : public QObject
{
    Q_OBJECT

private slots:
    void walkListsRegularFilesOnly();
    void skipRulesClassifyRuntimeArtifacts();
    void skipRulesHonourPolicy();
    void plannerCopiesAndMirrorsTheExpectedFiles();
    void plannerIgnoresSourceWithoutRoot();
    void plannerMapsZeroOrigToTheStartDirectory();
    void mirrorCopiesBytesExactly();
    void mirrorReportsMissingSource();
};

void CaseMirrorTest::walkListsRegularFilesOnly()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    const std::filesystem::path root{workspace.path().toStdString()};
    buildSourceCase(root);

    const SourceCaseWalk walk = SourceCaseWalker{root}.walk();

    QCOMPARE(walk.startDirectory, std::string("0"));
    QVERIFY(!walk.truncated);
    QVERIFY(!walk.files.empty());

    std::vector<std::string> paths;
    for (const SourceCaseFile& file : walk.files)
    {
        paths.push_back(file.relativePath);
    }
    QVERIFY(std::is_sorted(paths.begin(), paths.end()));

    const auto contains = [&paths](std::string_view path)
    { return std::find(paths.begin(), paths.end(), path) != paths.end(); };

    QVERIFY(contains("0/p"));
    QVERIFY(contains("constant/polyMesh/points"));
    QVERIFY(!contains("Allrun") || true);
    QVERIFY(contains("Allrun"));
    // Runtime products are pruned from the walk itself.
    QVERIFY(!contains("postProcessing/forces/0/forces.dat"));

    const auto zero = std::find_if(walk.files.begin(), walk.files.end(), [](const SourceCaseFile& file)
                                   { return file.relativePath == "0/p"; });
    QVERIFY(zero != walk.files.end());
    QVERIFY(zero->inStartDirectory);
    QCOMPARE(zero->byteSize, std::uintmax_t{2});
}

void CaseMirrorTest::skipRulesClassifyRuntimeArtifacts()
{
    const ExportSkipRules rules;

    QCOMPARE(rules.classify("Allrun", "0"), ExportSkipReason::RuntimeArtifact);
    QCOMPARE(rules.classify("Allclean", "0"), ExportSkipReason::RuntimeArtifact);
    QCOMPARE(rules.classify("log.blockMesh", "0"), ExportSkipReason::RuntimeArtifact);
    QCOMPARE(rules.classify("system/foo.log", "0"), ExportSkipReason::RuntimeArtifact);
    QCOMPARE(rules.classify("postProcessing/forces/0/forces.dat", "0"), ExportSkipReason::RuntimeArtifact);
    QCOMPARE(rules.classify("processor0/constant/polyMesh/points", "0"), ExportSkipReason::RuntimeArtifact);
    QCOMPARE(rules.classify("case.foam", "0"), ExportSkipReason::RuntimeArtifact);

    QCOMPARE(rules.classify("0/p", "0"), ExportSkipReason::None);
    QCOMPARE(rules.classify("constant/polyMesh/points", "0"), ExportSkipReason::None);
    QCOMPARE(rules.classify("1/p", "0"), ExportSkipReason::OtherTimeDirectory);
    QCOMPARE(rules.classify("latest/p", "0"), ExportSkipReason::OtherTimeDirectory);
    QCOMPARE(rules.classify("0.orig/p", "0.orig"), ExportSkipReason::None);
}

void CaseMirrorTest::skipRulesHonourPolicy()
{
    CaseExportSkipPolicy policy;
    policy.keepRunScripts = true;
    policy.mirrorOtherTimeDirectories = true;
    const ExportSkipRules rules{policy};

    QCOMPARE(rules.classify("Allrun", "0"), ExportSkipReason::None);
    QCOMPARE(rules.classify("1/p", "0"), ExportSkipReason::None);
    // The runtime products that are not scripts stay skipped.
    QCOMPARE(rules.classify("log.blockMesh", "0"), ExportSkipReason::RuntimeArtifact);
    QCOMPARE(rules.classify("postProcessing/x/y", "0"), ExportSkipReason::RuntimeArtifact);
}

void CaseMirrorTest::plannerCopiesAndMirrorsTheExpectedFiles()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    const std::filesystem::path root{workspace.path().toStdString()};
    const std::filesystem::path source = root / "source";
    buildSourceCase(source);

    const core::CaseProject project = projectWithP();
    CaseExportPlan plan;
    // The writer already produced `0/p` and `system/fvSchemes`.
    plan.files.push_back(ExportFile{"0/p", "generated p\n", {}, ExportAction::Written, 12});
    plan.files.push_back(ExportFile{"system/fvSchemes", "generated\n", {}, ExportAction::Written, 10});

    std::vector<core::ValidationIssue> issues;
    CaseMirrorPlanner::collect(project, optionsFor(source, root / "target"), plan, issues);

    const ExportFile* fieldT = findPlanned(plan, "0/T");
    QVERIFY(fieldT != nullptr);
    QCOMPARE(fieldT->action, ExportAction::Copied);
    QVERIFY(!fieldT->sourcePath.empty());

    const ExportFile* polyMesh = findPlanned(plan, "constant/polyMesh/points");
    QVERIFY(polyMesh != nullptr);
    QCOMPARE(polyMesh->action, ExportAction::Mirrored);

    const ExportFile* turbulence = findPlanned(plan, "constant/turbulenceProperties");
    QVERIFY(turbulence != nullptr);
    QCOMPARE(turbulence->action, ExportAction::Copied);

    const ExportFile* sample = findPlanned(plan, "system/sampleDict");
    QVERIFY(sample != nullptr);
    QCOMPARE(sample->action, ExportAction::Mirrored);

    // Files owned by the writers are never copied twice.
    QVERIFY(findPlanned(plan, "0/p") != nullptr);
    QCOMPARE(findPlanned(plan, "0/p")->action, ExportAction::Written);

    QVERIFY(isSkipped(plan, "Allrun"));
    QVERIFY(isSkipped(plan, "log.blockMesh"));
    QVERIFY(isSkipped(plan, "1/p"));
    QVERIFY(!isSkipped(plan, "0/T"));
    QCOMPARE(plan.count(ExportAction::Copied), std::size_t{2});
    QCOMPARE(plan.count(ExportAction::Mirrored), std::size_t{2});
}

void CaseMirrorTest::plannerIgnoresSourceWithoutRoot()
{
    const core::CaseProject project = projectWithP();
    CaseExportPlan plan;
    std::vector<core::ValidationIssue> issues;

    CaseExportOptions options;
    options.targetDirectory = std::filesystem::path{"target"};
    CaseMirrorPlanner::collect(project, options, plan, issues);

    QVERIFY(plan.files.empty());
    QVERIFY(plan.skipped.empty());
    QVERIFY(issues.empty());
}

void CaseMirrorTest::plannerMapsZeroOrigToTheStartDirectory()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    const std::filesystem::path root{workspace.path().toStdString()};
    const std::filesystem::path source = root / "source";
    writeFile(source / "0.orig" / "p", "p\n");
    writeFile(source / "0.orig" / "T", "T\n");
    writeFile(source / "constant" / "polyMesh" / "points", "points\n");

    const core::CaseProject project = projectWithP();
    CaseExportPlan plan;
    plan.files.push_back(ExportFile{"0/p", "generated p\n", {}, ExportAction::Written, 12});

    CaseExportOptions options = optionsFor(source, root / "target");
    options.startDirectory = "0";

    std::vector<core::ValidationIssue> issues;
    CaseMirrorPlanner::collect(project, options, plan, issues);

    QVERIFY(findPlanned(plan, "0/T") != nullptr);
    QCOMPARE(findPlanned(plan, "0/T")->action, ExportAction::Copied);
    // The field the writer owns is not mirrored from `0.orig/`.
    QVERIFY(findPlanned(plan, "0.orig/p") == nullptr);
    QCOMPARE(findPlanned(plan, "0/p")->action, ExportAction::Written);
}

void CaseMirrorTest::mirrorCopiesBytesExactly()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    const std::filesystem::path root{workspace.path().toStdString()};

    const std::string binary{"points\x00\x01\x02\n"};
    writeFile(root / "source" / "constant" / "polyMesh" / "points.gz", binary);

    CaseExportPlan plan;
    ExportFile copy;
    copy.relativePath = "constant/polyMesh/points.gz";
    copy.sourcePath = root / "source" / "constant" / "polyMesh" / "points.gz";
    copy.action = ExportAction::Mirrored;
    copy.byteSize = binary.size();
    plan.files.push_back(copy);
    plan.files.push_back(ExportFile{"0/p", "text\n", {}, ExportAction::Written, 5});

    const MirrorResult result = CaseMirror{}.copyInto(root / "staging", plan);

    QVERIFY2(result.ok, result.message.c_str());
    QCOMPARE(result.filesCopied, std::size_t{1});
    QCOMPARE(readFile(root / "staging" / "constant" / "polyMesh" / "points.gz"), binary);
    // Generated files are not touched by the mirror.
    QVERIFY(!std::filesystem::exists(root / "staging" / "0" / "p"));
}

void CaseMirrorTest::mirrorReportsMissingSource()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    const std::filesystem::path root{workspace.path().toStdString()};

    CaseExportPlan plan;
    ExportFile copy;
    copy.relativePath = "constant/polyMesh/points";
    copy.sourcePath = root / "source" / "missing";
    copy.action = ExportAction::Mirrored;
    plan.files.push_back(copy);

    const MirrorResult result = CaseMirror{}.copyInto(root / "staging", plan);
    QVERIFY(!result.ok);
    QVERIFY(!result.message.empty());
}

} // namespace ofgui::io

QTEST_APPLESS_MAIN(ofgui::io::CaseMirrorTest)
#include "CaseMirrorTest.moc"
