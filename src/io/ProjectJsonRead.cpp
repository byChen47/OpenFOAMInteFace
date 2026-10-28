#include "io/ProjectJsonMapping.h"

#include "core/BoundaryPatch.h"
#include "core/CaseProject.h"
#include "core/DomainBox.h"
#include "core/FieldDefinition.h"
#include "core/MeshSettings.h"
#include "core/SolverSettings.h"
#include "core/Vec3d.h"
#include "io/ProjectJsonReadSections.h"
#include "io/ProjectJsonSchema.h"
#include "io/ProjectJsonSource.h"
#include "io/ProjectJsonValueReader.h"

#include <QJsonObject>
#include <QJsonValue>

#include <string>
#include <utility>
#include <vector>

namespace ofgui::io {
namespace {

using projectjson::key;

void readProjectSection(ProjectJsonValueReader& reader, const QJsonObject& root, core::CaseProject& project)
{
    if (!root.contains(key(projectjson::kProject)))
    {
        return;
    }

    const std::string path = projectjson::kProject;
    QJsonObject object;
    if (!reader.readObject(root.value(key(projectjson::kProject)), path, object))
    {
        return;
    }

    if (object.contains(key(projectjson::kName)))
    {
        std::string name;
        if (reader.readString(
                object.value(key(projectjson::kName)),
                projectJsonChildPath(path, projectjson::kName),
                name))
        {
            project.setName(std::move(name));
        }
    }

    if (object.contains(key(projectjson::kCaseDirectory)))
    {
        std::string caseDirectory;
        if (reader.readString(
                object.value(key(projectjson::kCaseDirectory)),
                projectJsonChildPath(path, projectjson::kCaseDirectory),
                caseDirectory))
        {
            project.setCaseDirectory(std::move(caseDirectory));
        }
    }
}

void readDomainSection(ProjectJsonValueReader& reader, const QJsonObject& root, core::CaseProject& project)
{
    if (!root.contains(key(projectjson::kDomain)))
    {
        return;
    }

    const std::string path = projectjson::kDomain;
    QJsonObject object;
    if (!reader.readObject(root.value(key(projectjson::kDomain)), path, object))
    {
        return;
    }

    core::Vec3d minimum;
    core::Vec3d maximum;
    if (reader.requireKey(object, projectjson::kMin, path)
        && reader.readVec3(
            object.value(key(projectjson::kMin)),
            projectJsonChildPath(path, projectjson::kMin),
            minimum))
    {
        project.domain().setMin(minimum);
    }
    if (reader.requireKey(object, projectjson::kMax, path)
        && reader.readVec3(
            object.value(key(projectjson::kMax)),
            projectJsonChildPath(path, projectjson::kMax),
            maximum))
    {
        project.domain().setMax(maximum);
    }

    if (object.contains(key(projectjson::kMode)))
    {
        const std::string modePath = projectJsonChildPath(path, projectjson::kMode);
        std::string modeWord;
        if (reader.readString(object.value(key(projectjson::kMode)), modePath, modeWord))
        {
            const auto mode = projectjson::domainModeFromName(modeWord);
            if (mode)
            {
                project.domain().setMode(*mode);
            }
            else
            {
                reader.warning(
                    projectjson::kUnknownEnumWord,
                    modePath,
                    "unknown domain mode '" + modeWord + "'; keeping 'threeD'");
            }
        }
    }

    if (object.contains(key(projectjson::kUnit)))
    {
        std::string unit;
        if (reader.readString(
                object.value(key(projectjson::kUnit)),
                projectJsonChildPath(path, projectjson::kUnit),
                unit))
        {
            project.domain().setUnit(std::move(unit));
        }
    }
}

void readMeshSection(ProjectJsonValueReader& reader, const QJsonObject& root, core::CaseProject& project)
{
    if (!root.contains(key(projectjson::kMesh)))
    {
        return;
    }

    const std::string path = projectjson::kMesh;
    QJsonObject object;
    if (!reader.readObject(root.value(key(projectjson::kMesh)), path, object))
    {
        return;
    }

    if (reader.requireKey(object, projectjson::kCells, path))
    {
        std::vector<int> cells;
        if (reader.readIntArray(
                object.value(key(projectjson::kCells)),
                projectJsonChildPath(path, projectjson::kCells),
                3,
                cells))
        {
            project.mesh().setCells(core::MeshSettings::CellCounts{cells[0], cells[1], cells[2]});
        }
    }

    if (object.contains(key(projectjson::kGrading)))
    {
        std::vector<core::Real> grading;
        if (reader.readRealArray(
                object.value(key(projectjson::kGrading)),
                projectJsonChildPath(path, projectjson::kGrading),
                3,
                grading))
        {
            project.mesh().setGrading(core::MeshSettings::Grading{grading[0], grading[1], grading[2]});
        }
    }

    if (object.contains(key(projectjson::kScale)))
    {
        core::Real scale = 1.0;
        if (reader.readReal(
                object.value(key(projectjson::kScale)),
                projectJsonChildPath(path, projectjson::kScale),
                scale))
        {
            project.mesh().setScale(scale);
        }
    }
}

void readSolverSection(ProjectJsonValueReader& reader, const QJsonObject& root, core::CaseProject& project)
{
    if (!root.contains(key(projectjson::kSolver)))
    {
        return;
    }

    const std::string path = projectjson::kSolver;
    QJsonObject object;
    if (!reader.readObject(root.value(key(projectjson::kSolver)), path, object))
    {
        return;
    }

    core::SolverSettings solver = project.solver();

    if (reader.requireKey(object, projectjson::kApplication, path))
    {
        std::string application;
        if (reader.readString(
                object.value(key(projectjson::kApplication)),
                projectJsonChildPath(path, projectjson::kApplication),
                application))
        {
            solver.setApplication(std::move(application));
        }
    }

    core::Real startTime = solver.startTime();
    core::Real endTime = solver.endTime();
    core::Real deltaT = solver.deltaT();
    const auto readTime = [&reader, &object, &path](const char* name, core::Real& target)
    {
        if (!reader.requireKey(object, name, path))
        {
            return;
        }
        core::Real value = 0.0;
        if (reader.readReal(object.value(key(name)), projectJsonChildPath(path, name), value))
        {
            target = value;
        }
    };
    readTime(projectjson::kStartTime, startTime);
    readTime(projectjson::kEndTime, endTime);
    readTime(projectjson::kDeltaT, deltaT);
    solver.setStartTime(startTime);
    solver.setEndTime(endTime);
    solver.setDeltaT(deltaT);

    if (reader.requireKey(object, projectjson::kWriteInterval, path))
    {
        int writeInterval = 0;
        if (reader.readInt(
                object.value(key(projectjson::kWriteInterval)),
                projectJsonChildPath(path, projectjson::kWriteInterval),
                writeInterval))
        {
            solver.setWriteInterval(writeInterval);
        }
    }

    project.setSolver(std::move(solver));
}

} // namespace

bool projectJsonToModel(
    const QJsonObject& root,
    core::CaseProject& project,
    ProjectJsonExtras& extras,
    std::vector<ProjectJsonDiagnostic>& diagnostics,
    const std::string& source,
    int schemaVersion)
{
    ProjectJsonValueReader reader(source, diagnostics);

    readProjectSection(reader, root, project);
    readDomainSection(reader, root, project);
    readMeshSection(reader, root, project);
    readSolverSection(reader, root, project);
    projectJsonReadPatches(reader, root, project);
    projectJsonReadFields(reader, root, project);
    if (schemaVersion >= projectjson::kCurrentProjectSchemaVersion)
    {
        // A schemaVersion 1 document has no `source` section; reading it anyway
        // would import data the older format never promised to carry.
        projectJsonSourceFromJson(root, project.source(), reader);
    }

    projectJsonCollectExtras(root, extras);
    if (!extras.isEmpty())
    {
        reader.info(
            projectjson::kPreservedUnknownFields,
            {},
            "unknown project fields were preserved and will be written back on the next save");
    }

    return !reader.hasError();
}

} // namespace ofgui::io
