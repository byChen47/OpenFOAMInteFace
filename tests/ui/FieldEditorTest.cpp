#include "core/BoundaryCondition.h"
#include "core/CaseProject.h"
#include "core/FieldDefinition.h"
#include "core/FieldValue.h"
#include "ui/CaseDocumentController.h"
#include "ui/ExportPreviewPanel.h"
#include "ui/FieldBoundaryTable.h"
#include "ui/FieldEditor.h"
#include "ui/InspectorPanel.h"
#include "ui/ProjectNodeRef.h"
#include "ui/UniformValueEditor.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QTest>

#include <filesystem>

using namespace ofgui;

namespace {

const char* const kCavityRelative =
    "tutorials/incompressible/icoFoam/cavity/cavity";

core::ObjectId addFieldWithBoundaries(core::CaseProject& project,
                                      const std::string& name,
                                      core::FieldClass fieldClass,
                                      const core::FieldDefinition::Dimensions& dimensions)
{
    const core::ObjectId id = project.addField(name, fieldClass, dimensions);
    core::FieldDefinition* field = project.findField(id);
    if (field == nullptr)
    {
        return id;
    }
    if (fieldClass == core::FieldClass::VolScalarField)
    {
        field->setInternalField(core::FieldValue::uniformScalar(0.0));
    }
    else
    {
        field->setInternalField(core::FieldValue::uniformVector(core::Vec3d(1.0, 0.0, 0.0)));
    }
    for (const core::BoundaryPatch& patch : project.patches())
    {
        field->setBoundaryCondition(patch.id(), core::BoundaryCondition("zeroGradient"));
    }
    return id;
}

} // namespace

class FieldEditorTest final : public QObject
{
    Q_OBJECT

private slots:
    void showsFieldFromModel();
    void editingInternalFieldUpdatesModelAndPreview();
    void editingBoundaryTypeAndValueUpdatesCondition();
    void invalidBoundaryValueIsRejected();
    void importedUnknownBoundaryTypeStaysReadOnly();
    void addsAndRemovesPredefinedFields();
    void exportPreviewMatchesFieldWriter();
    void inspectorRoutesFieldAndPreviewNodes();
};

void FieldEditorTest::showsFieldFromModel()
{
    CaseDocumentController document;
    core::CaseProject& project = document.project();
    project = core::CaseProject("test");
    project.addPatch("wall", core::PatchType::Wall);
    project.addPatch("inlet", core::PatchType::Patch);
    const core::ObjectId fieldId = addFieldWithBoundaries(
        project, "U", core::FieldClass::VolVectorField, {0, 1, -1, 0, 0, 0, 0});

    FieldEditor editor;
    editor.setDocument(&document);
    editor.showField(fieldId);

    QCOMPARE(editor.fieldId(), fieldId);
    QCOMPARE(editor.boundaryTable()->table()->rowCount(), 2);
    const auto* spin = editor.findChild<QSpinBox*>(QStringLiteral("dimensionSpin1"));
    QVERIFY(spin != nullptr);
    QCOMPARE(spin->value(), 1);
    QCOMPARE(editor.internalFieldEditor()->rawText(), QStringLiteral("uniform (1 0 0)"));
    QVERIFY(editor.previewText().contains(QStringLiteral("volVectorField")));
}

void FieldEditorTest::editingInternalFieldUpdatesModelAndPreview()
{
    CaseDocumentController document;
    core::CaseProject& project = document.project();
    project = core::CaseProject("test");
    project.addPatch("wall", core::PatchType::Wall);
    const core::ObjectId fieldId = addFieldWithBoundaries(
        project, "p", core::FieldClass::VolScalarField, {0, 2, -2, 0, 0, 0, 0});
    document.notifyProjectEdited();
    QVERIFY(document.isDirty());

    FieldEditor editor;
    editor.setDocument(&document);
    editor.showField(fieldId);

    auto* spin = editor.findChild<QDoubleSpinBox*>(QStringLiteral("uniformScalarEdit"));
    QVERIFY(spin != nullptr);
    spin->setValue(2.5);
    QVERIFY(QMetaObject::invokeMethod(editor.internalFieldEditor(), "editingFinished"));

    QCOMPARE(project.findField(fieldId)->internalField().scalar(), 2.5);
    QVERIFY(editor.previewText().contains(QStringLiteral("uniform 2.5")));
    QVERIFY(document.isDirty());
}

void FieldEditorTest::editingBoundaryTypeAndValueUpdatesCondition()
{
    CaseDocumentController document;
    core::CaseProject& project = document.project();
    project = core::CaseProject("test");
    const core::ObjectId wallId = project.addPatch("wall", core::PatchType::Wall);
    project.addPatch("inlet", core::PatchType::Patch);
    const core::ObjectId fieldId = addFieldWithBoundaries(
        project, "p", core::FieldClass::VolScalarField, {0, 2, -2, 0, 0, 0, 0});

    FieldEditor editor;
    editor.setDocument(&document);
    editor.showField(fieldId);

    FieldBoundaryTable* table = editor.boundaryTable();
    QComboBox* combo = table->typeCombo(0);
    QLineEdit* value = table->valueEdit(0);
    QVERIFY(combo != nullptr);
    QVERIFY(value != nullptr);

    combo->setCurrentText(QStringLiteral("fixedValue"));
    value->setText(QStringLiteral("uniform 0"));
    QVERIFY(QMetaObject::invokeMethod(value, "editingFinished"));

    const core::FieldDefinition* field = project.findField(fieldId);
    QVERIFY(field != nullptr);
    const core::BoundaryCondition* condition = field->findBoundaryCondition(wallId);
    QVERIFY(condition != nullptr);
    QCOMPARE(QString::fromStdString(condition->type()), QStringLiteral("fixedValue"));
    QCOMPARE(QString::fromStdString(condition->parameters().at("value")), QStringLiteral("uniform 0"));
    QVERIFY(table->errorText().isEmpty());
    QVERIFY(editor.previewText().contains(QStringLiteral("fixedValue")));
}

void FieldEditorTest::invalidBoundaryValueIsRejected()
{
    CaseDocumentController document;
    core::CaseProject& project = document.project();
    project = core::CaseProject("test");
    const core::ObjectId wallId = project.addPatch("wall", core::PatchType::Wall);
    const core::ObjectId fieldId = addFieldWithBoundaries(
        project, "p", core::FieldClass::VolScalarField, {0, 2, -2, 0, 0, 0, 0});

    FieldEditor editor;
    editor.setDocument(&document);
    editor.showField(fieldId);

    FieldBoundaryTable* table = editor.boundaryTable();
    table->typeCombo(0)->setCurrentText(QStringLiteral("fixedValue"));
    QLineEdit* value = table->valueEdit(0);
    value->setText(QStringLiteral("not-a-number"));
    QVERIFY(QMetaObject::invokeMethod(value, "editingFinished"));

    const core::BoundaryCondition* condition = project.findField(fieldId)->findBoundaryCondition(wallId);
    QVERIFY(condition != nullptr);
    QCOMPARE(QString::fromStdString(condition->type()), QStringLiteral("zeroGradient"));
    QVERIFY(!table->errorText().isEmpty());
}

void FieldEditorTest::importedUnknownBoundaryTypeStaysReadOnly()
{
    const std::filesystem::path caseRoot =
        std::filesystem::path(OFGUI_FOAM_REFERENCE_DIR) / kCavityRelative;
    if (!std::filesystem::exists(caseRoot / "system" / "controlDict"))
    {
        QSKIP("OpenFOAM v2206 reference tree is not available.");
    }

    CaseDocumentController document;
    QVERIFY2(document.openCaseDirectory(QString::fromStdString(caseRoot.string())),
             qPrintable(document.lastError()));

    const core::FieldDefinition* uField = document.project().findFieldByName("U");
    QVERIFY(uField != nullptr);
    const core::ObjectId fieldId = uField->id();

    core::FieldDefinition* editableU = document.project().findField(fieldId);
    const core::BoundaryPatch& firstPatch = document.project().patches().front();
    core::BoundaryCondition unknown(QStringLiteral("groovyBC").toStdString());
    editableU->setBoundaryCondition(firstPatch.id(), unknown);

    FieldEditor editor;
    editor.setDocument(&document);
    editor.showField(fieldId);

    FieldBoundaryTable* table = editor.boundaryTable();
    QVERIFY(table->table()->rowCount() >= 3);
    bool foundUnknown = false;
    for (int row = 0; row < table->table()->rowCount(); ++row)
    {
        QComboBox* combo = table->typeCombo(row);
        if (combo != nullptr && combo->currentData().toString() == QStringLiteral("groovyBC"))
        {
            foundUnknown = true;
            QVERIFY(!combo->isEnabled());
            QVERIFY(!table->valueEdit(row)->isEnabled());
        }
    }
    QVERIFY(foundUnknown);
    QVERIFY(editor.previewText().contains(QStringLiteral("groovyBC")));
}

void FieldEditorTest::addsAndRemovesPredefinedFields()
{
    CaseDocumentController document;
    core::CaseProject& project = document.project();
    project = core::CaseProject("test");
    project.addPatch("wall", core::PatchType::Wall);

    FieldEditor editor;
    editor.setDocument(&document);

    QVERIFY(editor.addPredefinedField(QStringLiteral("p")));
    const core::FieldDefinition* field = project.findFieldByName("p");
    QVERIFY(field != nullptr);
    QCOMPARE(field->dimensions()[1], 2);
    QCOMPARE(field->dimensions()[2], -2);
    QCOMPARE(project.patches().size(), std::size_t{1});
    QVERIFY(field->hasBoundaryCondition(project.patches().front().id()));

    QVERIFY(!editor.addPredefinedField(QStringLiteral("p")));
    QVERIFY(!editor.errorText().isEmpty());
    QVERIFY(!editor.addPredefinedField(QStringLiteral("k")));

    QVERIFY(editor.removeCurrentField());
    QVERIFY(project.findFieldByName("p") == nullptr);
}

void FieldEditorTest::exportPreviewMatchesFieldWriter()
{
    CaseDocumentController document;
    core::CaseProject& project = document.project();
    project = core::CaseProject("test");
    project.addPatch("wall", core::PatchType::Wall);
    const core::ObjectId pressureId = addFieldWithBoundaries(
        project, "p", core::FieldClass::VolScalarField, {0, 2, -2, 0, 0, 0, 0});
    addFieldWithBoundaries(project, "U", core::FieldClass::VolVectorField, {0, 1, -1, 0, 0, 0, 0});

    ExportPreviewPanel preview;
    preview.setDocument(&document);
    QCOMPARE(preview.availableFiles(), QStringList({QStringLiteral("0/p"), QStringLiteral("0/U")}));

    preview.selectField(pressureId);
    QVERIFY(preview.currentText().contains(QStringLiteral("dimensions")));
    QVERIFY(preview.currentText().contains(QStringLiteral("[0 2 -2 0 0 0 0]")));
    QVERIFY(preview.currentText().contains(QStringLiteral("object")));
    QVERIFY(preview.currentText().contains(QStringLiteral(" p;")));
}

void FieldEditorTest::inspectorRoutesFieldAndPreviewNodes()
{
    CaseDocumentController document;
    core::CaseProject& project = document.project();
    project = core::CaseProject("test");
    project.addPatch("wall", core::PatchType::Wall);
    const core::ObjectId fieldId = addFieldWithBoundaries(
        project, "p", core::FieldClass::VolScalarField, {0, 2, -2, 0, 0, 0, 0});

    InspectorPanel inspector;
    inspector.setDocument(&document);

    inspector.showNode(ProjectNodeRef::forField(fieldId));
    QVERIFY(inspector.isEditingField());
    QCOMPARE(inspector.fieldEditor()->fieldId(), fieldId);

    inspector.showNode(ProjectNodeRef::section(NodeKind::ExportPreview));
    QVERIFY(!inspector.isEditingField());
    QVERIFY(inspector.exportPreview()->currentText().contains(QStringLiteral("object      p;")));

    inspector.showNode(ProjectNodeRef::forFieldBoundary(fieldId, project.patches().front().id()));
    QVERIFY(inspector.isEditingField());
    QCOMPARE(inspector.fieldEditor()->boundaryTable()->currentPatchId(), project.patches().front().id());
}

QTEST_MAIN(FieldEditorTest)
#include "FieldEditorTest.moc"
