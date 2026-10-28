#include "io/ProjectJsonMapping.h"

#include "core/BoundaryPatch.h"
#include "core/CaseProject.h"
#include "core/DomainBox.h"
#include "core/FieldDefinition.h"
#include "core/FieldValue.h"
#include "core/MeshSettings.h"
#include "core/ProjectSchemaVersion.h"
#include "core/SolverSettings.h"
#include "core/Vec3d.h"
#include "io/ProjectJsonSchema.h"
#include "io/ProjectJsonSource.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include <array>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <utility>

namespace ofgui::io {
namespace {

using projectjson::key;

QString toQString(std::string_view text)
{
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

QString toQString(const std::string& text)
{
    return QString::fromStdString(text);
}

QJsonArray realArrayToJson(const std::array<core::Real, 3>& values)
{
    return QJsonArray{values[0], values[1], values[2]};
}

QJsonArray vectorToJson(const core::Vec3d& vector)
{
    return QJsonArray{vector.x, vector.y, vector.z};
}

QJsonArray dimensionsToJson(const core::FieldDefinition::Dimensions& dimensions)
{
    QJsonArray array;
    for (const int exponent : dimensions)
    {
        array.append(exponent);
    }
    return array;
}

QJsonObject internalFieldToJson(const core::FieldValue& value)
{
    QJsonObject object;

    if (!value.isValid())
    {
        object[key(projectjson::kKind)] = toQString(std::string_view{projectjson::kUnsetValueKind});
        return object;
    }

    object[key(projectjson::kKind)] = toQString(projectjson::fieldValueKindName(value.kind()));
    switch (value.kind())
    {
    case core::FieldValueKind::UniformScalar:
        object[key(projectjson::kValue)] = value.scalar();
        break;
    case core::FieldValueKind::UniformVector:
        object[key(projectjson::kValue)] = vectorToJson(value.vector());
        break;
    case core::FieldValueKind::NonUniformRaw:
        object[key(projectjson::kRaw)] = toQString(value.rawText());
        break;
    }

    return object;
}

QJsonObject boundaryConditionToJson(
    const core::ObjectId& fieldId,
    const core::ObjectId& patchId,
    const core::BoundaryCondition& condition,
    const ProjectJsonExtras& extras)
{
    const QString lookupKey =
        toQString(fieldId.toString()) + QLatin1Char('/') + toQString(patchId.toString());
    QJsonObject object = extras.boundaryConditions.value(lookupKey);

    object[key(projectjson::kPatchId)] = toQString(patchId.toString());
    object[key(projectjson::kType)] = toQString(condition.type());

    QJsonObject parameters;
    for (const auto& [name, value] : condition.parameters())
    {
        parameters[toQString(name)] = toQString(value);
    }
    object[key(projectjson::kParameters)] = parameters;

    return object;
}

QJsonObject patchToJson(const core::BoundaryPatch& patch, const ProjectJsonExtras& extras)
{
    const QString idText = toQString(patch.id().toString());
    QJsonObject object = extras.patches.value(idText);

    object[key(projectjson::kId)] = idText;
    object[key(projectjson::kName)] = toQString(patch.name());
    object[key(projectjson::kType)] = toQString(core::patchTypeName(patch.type()));

    QJsonArray faces;
    for (const core::FaceId face : patch.faces())
    {
        faces.append(QString::fromLatin1(core::faceName(face)));
    }
    object[key(projectjson::kFaces)] = faces;

    object[key(projectjson::kColor)] = realArrayToJson(patch.color());
    object[key(projectjson::kVisible)] = patch.isVisible();

    return object;
}

QJsonObject fieldToJson(
    const core::CaseProject& project,
    const core::FieldDefinition& field,
    const ProjectJsonExtras& extras)
{
    const QString idText = toQString(field.id().toString());
    QJsonObject object = extras.fields.value(idText);

    object[key(projectjson::kId)] = idText;
    object[key(projectjson::kName)] = toQString(field.name());
    object[key(projectjson::kClass)] = toQString(core::fieldClassName(field.fieldClass()));
    object[key(projectjson::kDimensions)] = dimensionsToJson(field.dimensions());
    object[key(projectjson::kInternalField)] = internalFieldToJson(field.internalField());

    // Patch order first (readable, stable), then conditions whose patch no
    // longer exists; those are preserved so the validator can report them.
    QJsonArray boundaryField;
    std::set<core::ObjectId> written;
    for (const core::BoundaryPatch& patch : project.patches())
    {
        const auto condition = field.boundaryConditions().find(patch.id());
        if (condition == field.boundaryConditions().end())
        {
            continue;
        }

        boundaryField.append(
            boundaryConditionToJson(field.id(), patch.id(), condition->second, extras));
        written.insert(patch.id());
    }

    for (const auto& [patchId, condition] : field.boundaryConditions())
    {
        if (written.find(patchId) != written.end())
        {
            continue;
        }
        boundaryField.append(boundaryConditionToJson(field.id(), patchId, condition, extras));
    }

    object[key(projectjson::kBoundaryField)] = boundaryField;
    return object;
}

void appendWarning(
    std::vector<ProjectJsonDiagnostic>& diagnostics,
    const char* code,
    std::string path,
    std::string message)
{
    if (diagnostics.size() >= projectjson::kMaxDiagnostics)
    {
        return;
    }
    diagnostics.push_back(
        ProjectJsonDiagnostic{ProjectJsonSeverity::Warning, code, std::move(path), std::move(message), {}});
}

} // namespace

QJsonObject projectJsonFromModel(
    const core::CaseProject& project,
    const ProjectJsonExtras& extras,
    std::vector<ProjectJsonDiagnostic>& diagnostics)
{
    QJsonObject projectObject = extras.project;
    projectObject[key(projectjson::kName)] = toQString(project.name());
    projectObject[key(projectjson::kCaseDirectory)] = toQString(project.caseDirectory());

    if (!project.caseDirectory().empty())
    {
        const std::filesystem::path casePath{project.caseDirectory()};
        if (casePath.is_absolute() || casePath.has_root_directory())
        {
            appendWarning(
                diagnostics,
                projectjson::kAbsoluteCaseDirectory,
                projectjson::kCaseDirectory,
                "caseDirectory is an absolute path; a .ofgproj should store a path relative to the "
                "project file so the project survives moving (see D-025)");
        }
    }

    QJsonObject domainObject = extras.domain;
    domainObject[key(projectjson::kMin)] = vectorToJson(project.domain().min());
    domainObject[key(projectjson::kMax)] = vectorToJson(project.domain().max());
    domainObject[key(projectjson::kMode)] = toQString(projectjson::domainModeName(project.domain().mode()));
    domainObject[key(projectjson::kUnit)] = toQString(project.domain().unit());

    QJsonObject meshObject = extras.mesh;
    meshObject[key(projectjson::kCells)] =
        QJsonArray{project.mesh().cells()[0], project.mesh().cells()[1], project.mesh().cells()[2]};
    meshObject[key(projectjson::kGrading)] = realArrayToJson(project.mesh().grading());
    meshObject[key(projectjson::kScale)] = project.mesh().scale();

    QJsonObject solverObject = extras.solver;
    solverObject[key(projectjson::kApplication)] = toQString(project.solver().application());
    solverObject[key(projectjson::kStartTime)] = project.solver().startTime();
    solverObject[key(projectjson::kEndTime)] = project.solver().endTime();
    solverObject[key(projectjson::kDeltaT)] = project.solver().deltaT();
    solverObject[key(projectjson::kWriteInterval)] = project.solver().writeInterval();

    QJsonArray patches;
    for (const core::BoundaryPatch& patch : project.patches())
    {
        patches.append(patchToJson(patch, extras));
    }

    QJsonArray fields;
    for (const core::FieldDefinition& field : project.fields())
    {
        fields.append(fieldToJson(project, field, extras));
    }

    QJsonObject root = extras.root;
    root[key(projectjson::kSchemaVersion)] = projectjson::kCurrentProjectSchemaVersion;
    root[key(projectjson::kProject)] = projectObject;
    root[key(projectjson::kDomain)] = domainObject;
    root[key(projectjson::kMesh)] = meshObject;
    root[key(projectjson::kSolver)] = solverObject;
    root[key(projectjson::kPatches)] = patches;
    root[key(projectjson::kFields)] = fields;
    root[key(projectjson::kSource)] = projectJsonSourceToJson(project.source(), extras, diagnostics);
    return root;
}

} // namespace ofgui::io
