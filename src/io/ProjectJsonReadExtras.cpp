#include "io/ProjectJsonReadSections.h"

#include "io/ProjectJsonExtras.h"
#include "io/ProjectJsonSchema.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <functional>
#include <initializer_list>

namespace ofgui::io {
namespace {

using projectjson::key;

QJsonObject stripKeys(const QJsonObject& object, std::initializer_list<const char*> keys)
{
    QJsonObject remaining = object;
    for (const char* name : keys)
    {
        remaining.remove(key(name));
    }
    return remaining;
}

} // namespace

void projectJsonCollectExtras(const QJsonObject& root, ProjectJsonExtras& extras)
{
    extras.root = stripKeys(
        root,
        {projectjson::kSchemaVersion,
         projectjson::kProject,
         projectjson::kDomain,
         projectjson::kMesh,
         projectjson::kSolver,
         projectjson::kPatches,
         projectjson::kFields,
         projectjson::kSource});

    if (root.value(key(projectjson::kProject)).isObject())
    {
        extras.project = stripKeys(
            root.value(key(projectjson::kProject)).toObject(),
            {projectjson::kName, projectjson::kCaseDirectory});
    }
    if (root.value(key(projectjson::kDomain)).isObject())
    {
        extras.domain = stripKeys(
            root.value(key(projectjson::kDomain)).toObject(),
            {projectjson::kMin, projectjson::kMax, projectjson::kMode, projectjson::kUnit});
    }
    if (root.value(key(projectjson::kMesh)).isObject())
    {
        extras.mesh = stripKeys(
            root.value(key(projectjson::kMesh)).toObject(),
            {projectjson::kCells, projectjson::kGrading, projectjson::kScale});
    }
    if (root.value(key(projectjson::kSolver)).isObject())
    {
        extras.solver = stripKeys(
            root.value(key(projectjson::kSolver)).toObject(),
            {projectjson::kApplication,
             projectjson::kStartTime,
             projectjson::kEndTime,
             projectjson::kDeltaT,
             projectjson::kWriteInterval});
    }

    // Unknown keys per array element, keyed by the element's object id. Elements
    // without a readable id cannot be matched on save and are not preserved.
    const auto collectPerId = [](const QJsonValue& section,
                                 std::initializer_list<const char*> knownKeys,
                                 QHash<QString, QJsonObject>& target,
                                 const std::function<void(const QJsonObject&, const QString&)>& perElement)
    {
        if (!section.isArray())
        {
            return;
        }
        for (const QJsonValue& element : section.toArray())
        {
            if (!element.isObject())
            {
                continue;
            }
            const QJsonObject object = element.toObject();
            const QJsonValue idValue = object.value(key(projectjson::kId));
            if (!idValue.isString())
            {
                continue;
            }
            const QString id = idValue.toString();
            const QJsonObject remainder = stripKeys(object, knownKeys);
            if (!remainder.isEmpty())
            {
                target.insert(id, remainder);
            }
            if (perElement)
            {
                perElement(object, id);
            }
        }
    };

    collectPerId(
        root.value(key(projectjson::kPatches)),
        {projectjson::kId,
         projectjson::kName,
         projectjson::kType,
         projectjson::kFaces,
         projectjson::kColor,
         projectjson::kVisible},
        extras.patches,
        {});

    collectPerId(
        root.value(key(projectjson::kFields)),
        {projectjson::kId,
         projectjson::kName,
         projectjson::kClass,
         projectjson::kDimensions,
         projectjson::kInternalField,
         projectjson::kBoundaryField},
        extras.fields,
        [&extras](const QJsonObject& fieldObject, const QString& fieldId)
        {
            const QJsonValue boundaryField = fieldObject.value(key(projectjson::kBoundaryField));
            if (!boundaryField.isArray())
            {
                return;
            }
            for (const QJsonValue& element : boundaryField.toArray())
            {
                if (!element.isObject())
                {
                    continue;
                }
                const QJsonObject condition = element.toObject();
                const QJsonValue patchIdValue = condition.value(key(projectjson::kPatchId));
                if (!patchIdValue.isString())
                {
                    continue;
                }
                const QJsonObject remainder = stripKeys(
                    condition,
                    {projectjson::kPatchId, projectjson::kType, projectjson::kParameters});
                if (!remainder.isEmpty())
                {
                    extras.boundaryConditions.insert(
                        fieldId + QLatin1Char('/') + patchIdValue.toString(),
                        remainder);
                }
            }
        });

    // `source` (schemaVersion 2): unknown keys of the section and of each file
    // entry, keyed by the entry's `relativePath`.
    const QJsonValue sourceValue = root.value(key(projectjson::kSource));
    if (sourceValue.isObject())
    {
        const QJsonObject sourceObject = sourceValue.toObject();
        extras.source = stripKeys(
            sourceObject,
            {projectjson::kCaseRelativePath,
             projectjson::kStartDirectory,
             projectjson::kImportedAt,
             projectjson::kFiles});

        const QJsonValue filesValue = sourceObject.value(key(projectjson::kFiles));
        if (filesValue.isArray())
        {
            for (const QJsonValue& element : filesValue.toArray())
            {
                if (!element.isObject())
                {
                    continue;
                }
                const QJsonObject entry = element.toObject();
                const QJsonValue pathValue = entry.value(key(projectjson::kRelativePath));
                if (!pathValue.isString())
                {
                    continue;
                }
                const QJsonObject remainder =
                    stripKeys(entry, {projectjson::kRelativePath, projectjson::kSha256});
                if (!remainder.isEmpty())
                {
                    extras.sourceFiles.insert(pathValue.toString(), remainder);
                }
            }
        }
    }
}

} // namespace ofgui::io
