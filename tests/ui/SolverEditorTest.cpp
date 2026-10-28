#include "core/CaseProject.h"
#include "core/SolverSettings.h"
#include "io/writers/ControlDictWriter.h"
#include "io/writers/TransportPropertiesWriter.h"
#include "ui/CaseDocumentController.h"
#include "ui/InspectorPanel.h"
#include "ui/ProjectNodeRef.h"
#include "ui/SolverEditor.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTest>

#include <filesystem>
#include <fstream>

using namespace ofgui;

namespace {

const char* const kCavityRelative =
    "tutorials/incompressible/icoFoam/cavity/cavity";

std::filesystem::path cavityCase()
{
    return std::filesystem::path(OFGUI_FOAM_REFERENCE_DIR) / kCavityRelative;
}

bool loadCavity(CaseDocumentController& document)
{
    const std::filesystem::path caseRoot = cavityCase();
    if (!std::filesystem::exists(caseRoot / "system" / "controlDict"))
    {
        return false;
    }
    return document.openCaseDirectory(QString::fromStdString(caseRoot.string()));
}

void writeTextFile(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << text;
}

bool writeDimensionedNuCase(const std::filesystem::path& root)
{
    writeTextFile(root / "system" / "controlDict",
                  "FoamFile\n{\n    version     2.0;\n    format      ascii;\n"
                  "    class       dictionary;\n    object      controlDict;\n}\n\n"
                  "application     icoFoam;\nstartFrom       startTime;\nstartTime       0;\n"
                  "stopAt          endTime;\nendTime         1;\ndeltaT          0.01;\n"
                  "writeControl    timeStep;\nwriteInterval   10;\n");
    writeTextFile(root / "constant" / "transportProperties",
                  "FoamFile\n{\n    version     2.0;\n    format      ascii;\n"
                  "    class       dictionary;\n    object      transportProperties;\n}\n\n"
                  "transportModel  Newtonian;\n\n"
                  "nu              [0 2 -1 0 0 0 0] 1e-05;\n");
    writeTextFile(root / "0" / "p",
                  "FoamFile\n{\n    version     2.0;\n    format      ascii;\n"
                  "    class       volScalarField;\n    object      p;\n}\n\n"
                  "dimensions      [0 2 -2 0 0 0 0];\n\n"
                  "internalField   uniform 0;\n\nboundaryField\n{\n}\n");
    return std::filesystem::exists(root / "system" / "controlDict");
}

} // namespace

class SolverEditorTest final : public QObject
{
    Q_OBJECT

private slots:
    void showsImportedControlDictAndTransport();
    void editingControlDictUpdatesModelAndPreview();
    void invalidInputIsRejectedWithoutTouchingModel();
    void editingNuUpdatesTransportPreview();
    void dimensionedNuStaysReadOnly();
    void previewMatchesWriters();
    void sourceDictionaryTabShowsImportedText();
    void transportPersistenceLimitIsExplained();
    void newProjectShowsCavitySolver();
    void inspectorRoutesSolverNode();
};

void SolverEditorTest::showsImportedControlDictAndTransport()
{
    CaseDocumentController document;
    if (!loadCavity(document))
    {
        QSKIP("OpenFOAM-v2206 cavity tutorial is not available or not importable");
    }

    SolverEditor editor;
    editor.setDocument(&document);

    QCOMPARE(editor.applicationCombo()->currentText(), QStringLiteral("icoFoam"));
    QCOMPARE(editor.startTimeSpin()->value(), 0.0);
    QCOMPARE(editor.endTimeSpin()->value(), 0.5);
    QCOMPARE(editor.deltaTSpin()->value(), 0.005);
    QCOMPARE(editor.writeIntervalSpin()->value(), 20);
    QVERIFY(editor.controlDictNote()->text().contains(QStringLiteral("startFrom = startTime")));
    QVERIFY(editor.controlDictNote()->text().contains(QStringLiteral("writeControl = timeStep")));

    QVERIFY(editor.nuSpin()->isEnabled());
    QCOMPARE(editor.nuSpin()->value(), 0.01);
    QVERIFY(editor.turbulenceLabel()->text().contains(QStringLiteral("laminar")));
    QVERIFY(editor.transportModelCombo()->isEnabled());
}

void SolverEditorTest::editingControlDictUpdatesModelAndPreview()
{
    CaseDocumentController document;
    if (!loadCavity(document))
    {
        QSKIP("OpenFOAM-v2206 cavity tutorial is not available or not importable");
    }

    SolverEditor editor;
    editor.setDocument(&document);

    editor.endTimeSpin()->setValue(0.75);
    QCOMPARE(document.project().solver().endTime(), 0.75);
    QVERIFY(document.isDirty());
    QVERIFY(editor.previewText()->toPlainText().contains(QStringLiteral("endTime")));
    QVERIFY(editor.previewText()->toPlainText().contains(QStringLiteral("0.75")));

    editor.writeIntervalSpin()->setValue(5);
    QCOMPARE(document.project().solver().writeInterval(), 5);
    QVERIFY(editor.previewText()->toPlainText().contains(QStringLiteral("writeInterval")));

    editor.applicationCombo()->setCurrentText(QStringLiteral("pimpleFoam"));
    QCOMPARE(QString::fromStdString(document.project().solver().application()),
             QStringLiteral("pimpleFoam"));
    QVERIFY(editor.previewText()->toPlainText().contains(QStringLiteral("pimpleFoam")));
}

void SolverEditorTest::invalidInputIsRejectedWithoutTouchingModel()
{
    CaseDocumentController document;
    if (!loadCavity(document))
    {
        QSKIP("OpenFOAM-v2206 cavity tutorial is not available or not importable");
    }

    SolverEditor editor;
    editor.setDocument(&document);

    editor.startTimeSpin()->setValue(2.0);
    QCOMPARE(document.project().solver().startTime(), 0.0);
    QCOMPARE(editor.startTimeSpin()->value(), 0.0);
    QVERIFY(!editor.controlDictError()->text().isEmpty());

    editor.applicationCombo()->setCurrentText(QString());
    QCOMPARE(QString::fromStdString(document.project().solver().application()),
             QStringLiteral("icoFoam"));
    QVERIFY(!editor.controlDictError()->text().isEmpty());

    editor.endTimeSpin()->setValue(1.0);
    QCOMPARE(document.project().solver().endTime(), 1.0);
    QVERIFY(editor.controlDictError()->text().isEmpty());
}

void SolverEditorTest::editingNuUpdatesTransportPreview()
{
    CaseDocumentController document;
    if (!loadCavity(document))
    {
        QSKIP("OpenFOAM-v2206 cavity tutorial is not available or not importable");
    }

    SolverEditor editor;
    editor.setDocument(&document);

    editor.nuSpin()->setValue(0.02);
    QCOMPARE(document.transportSettings().kinematicViscosity, 0.02);
    QVERIFY(document.isDirty());

    editor.previewCombo()->setCurrentIndex(1);
    QCOMPARE(editor.currentPreviewPath(), QStringLiteral("constant/transportProperties"));
    const QString text = editor.previewText()->toPlainText();
    QVERIFY(text.contains(QStringLiteral("nu")));
    QVERIFY(text.contains(QStringLiteral("0.02")));
    QCOMPARE(text,
             QString::fromStdString(
                 io::TransportPropertiesWriter::write(document.transportSettings())));
}

void SolverEditorTest::dimensionedNuStaysReadOnly()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const std::filesystem::path root =
        std::filesystem::path(directory.path().toStdString()) / "case";
    QVERIFY(writeDimensionedNuCase(root));

    CaseDocumentController document;
    QVERIFY2(document.openCaseDirectory(QString::fromStdString(root.string())),
             qPrintable(document.lastError()));
    QVERIFY(!document.isKinematicViscosityEditable());

    SolverEditor editor;
    editor.setDocument(&document);
    QVERIFY(!editor.nuSpin()->isEnabled());
    QVERIFY(editor.transportNote()->text().contains(QStringLiteral("[0 2 -1 0 0 0 0]")));

    editor.previewCombo()->setCurrentIndex(1);
    QVERIFY(editor.previewText()->toPlainText().contains(QStringLiteral("[0 2 -1 0 0 0 0]")));
    QVERIFY(editor.previewText()->toPlainText().contains(QStringLiteral("1e-05")));
}

void SolverEditorTest::previewMatchesWriters()
{
    CaseDocumentController document;
    if (!loadCavity(document))
    {
        QSKIP("OpenFOAM-v2206 cavity tutorial is not available or not importable");
    }

    SolverEditor editor;
    editor.setDocument(&document);

    QCOMPARE(editor.previewText()->toPlainText(),
             QString::fromStdString(io::ControlDictWriter::write(document.project())));

    editor.previewCombo()->setCurrentIndex(1);
    QCOMPARE(editor.previewText()->toPlainText(),
             QString::fromStdString(io::TransportPropertiesWriter::write(document.transportSettings())));
}

void SolverEditorTest::sourceDictionaryTabShowsImportedText()
{
    CaseDocumentController document;
    if (!loadCavity(document))
    {
        QSKIP("OpenFOAM-v2206 cavity tutorial is not available or not importable");
    }

    SolverEditor editor;
    editor.setDocument(&document);

    QVERIFY(editor.rawCombo()->count() >= 3);
    const int controlIndex = editor.rawCombo()->findText(QStringLiteral("system/controlDict"));
    QVERIFY(controlIndex >= 0);
    editor.rawCombo()->setCurrentIndex(controlIndex);
    QVERIFY(editor.rawText()->toPlainText().contains(QStringLiteral("application")));
    QVERIFY(editor.rawText()->toPlainText().contains(QStringLiteral("icoFoam")));

    const int schemesIndex = editor.rawCombo()->findText(QStringLiteral("system/fvSchemes"));
    QVERIFY(schemesIndex >= 0);
    editor.rawCombo()->setCurrentIndex(schemesIndex);
    QVERIFY(editor.rawText()->toPlainText().contains(QStringLiteral("divSchemes")));
}

void SolverEditorTest::transportPersistenceLimitIsExplained()
{
    // F-05 (t14 review): transportModel/nu are not part of .ofgproj (strategy B),
    // so the constant page must state the re-open behaviour instead of silently
    // reverting to the source-case projection.
    CaseDocumentController document;
    SolverEditor editor;
    editor.setDocument(&document);

    const QString note = editor.transportNote()->text();
    QVERIFY(note.contains(QStringLiteral(".ofgproj")));
    QVERIFY(note.contains(QStringLiteral("重读")));

    if (!loadCavity(document))
    {
        return; // 无参考算例时只验证默认工程的提示；导入路径由其它槽覆盖。
    }
    const QString importedNote = editor.transportNote()->text();
    QVERIFY(importedNote.contains(QStringLiteral(".ofgproj")));
    QVERIFY(importedNote.contains(QStringLiteral("重读")));
}

void SolverEditorTest::newProjectShowsCavitySolver()
{
    CaseDocumentController document;

    SolverEditor editor;
    editor.setDocument(&document);

    // New projects carry the cavity template (t12) so they are exportable at once.
    QCOMPARE(editor.applicationCombo()->currentText(), QStringLiteral("icoFoam"));
    QCOMPARE(editor.endTimeSpin()->value(), document.project().solver().endTime());
    QCOMPARE(editor.endTimeSpin()->value(), 0.5);
    QVERIFY(editor.nuSpin()->isEnabled());
    QVERIFY(!editor.previewText()->toPlainText().isEmpty());
    QVERIFY(editor.previewText()->toPlainText().contains(QStringLiteral("controlDict")));
}

void SolverEditorTest::inspectorRoutesSolverNode()
{
    CaseDocumentController document;
    if (!loadCavity(document))
    {
        QSKIP("OpenFOAM-v2206 cavity tutorial is not available or not importable");
    }

    InspectorPanel inspector;
    inspector.setDocument(&document);
    inspector.showNode(ProjectNodeRef::section(NodeKind::Solver));

    QVERIFY(inspector.isEditingSolver());
    QCOMPARE(inspector.solverEditor()->applicationCombo()->currentText(),
             QStringLiteral("icoFoam"));
}

QTEST_MAIN(SolverEditorTest)
#include "SolverEditorTest.moc"
