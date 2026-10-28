#include "io/ProjectJsonReadSections.h"

#include "core/BoundaryCondition.h"
#include "core/CaseProject.h"
#include "core/FieldDefinition.h"
#include "core/FieldValue.h"
#include "core/ObjectId.h"
#include "io/ProjectJsonSchema.h"
#include "io/ProjectJsonValueReader.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include <array>
#include <cstddef>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ofgui::io {
namespace {

using projectjson::key;

// Reads the "kind" discriminator of an internal field value. Returns false when
// the value is the explicit "unset" placeholder, which keeps the field's
// default value intact.
bool readInternalFieldKind(
    ProjectJsonValueReader& reader,
    const QJsonObject& object,
    const std::string& objectPath,
    core::FieldValueKind& kind)
{
    const std::string kindPath = projectJsonChildPath(objectPath, projectjson::kKind);
    std::string word;
    if (!reader.readString(object.value(key(projectjson::kKind)), kindPath, word))
    {
        return false;
    }
    if (word == projectjson::kUnsetValueKind)
    {
        return false;
    }
    const auto parsed = projectjson::fieldValueKindFromName(word);
    if (!parsed)
    {
        reader.error(
            projectjson::kInvalidValue,
            kindPath,
            "unknown field value kind '" + word + "'");
        return false;
    }
    kind = *parsed;
    return true;
}

bool readInternalField(
    ProjectJsonValueReader& reader,
    const QJsonObject& object,
    const std::string& objectPath,
    core::FieldValue& value)
{
    const std::string path = projectJsonChildPath(objectPath, projectjson::kInternalField);
    QJsonObject valueObject;
    if (!reader.readObject(object.value(key(projectjson::kInternalField)), path, valueObject))
    {
        return false;
    }

    core::FieldValueKind kind = core::FieldValueKind::UniformScalar;
    if (!readInternalFieldKind(reader, valueObject, path, kind))
    {
        return false;
    }

    switch (kind)
    {
    case core::FieldValueKind::UniformScalar:
    {
        core::Real scalar = 0.0;
        if (!reader.readReal(
                valueObject.value(key(projectjson::kValue)),
                projectJsonChildPath(path, projectjson::kValue),
                scalar))
        {
            return false;
        }
        value = core::FieldValue::uniformScalar(scalar);
        return true;
    }
    case core::FieldValueKind::UniformVector:
    {
        core::Vec3d vector;
        if (!reader.readVec3(
                valueObject.value(key(projectjson::kValue)),
                projectJsonChildPath(path, projectjson::kValue),
                vector))
        {
            return false;
        }
        value = core::FieldValue::uniformVector(vector);
        return true;
    }
    case core::FieldValueKind::NonUniformRaw:
    {
        std::string raw;
        if (!reader.readString(
                valueObject.value(key(projectjson::kRaw)),
                projectJsonChildPath(path, projectjson::kRaw),
                raw))
        {
            return false;
        }
        value = core::FieldValue::nonUniformRaw(std::move(raw));
        return true;
    }
    }
    return false;
}

struct LoadedCondition
{
    std::string patchText;
    core::BoundaryCondition condition;
};

bool readCondition(
    ProjectJsonValueReader& reader,
    const QJsonObject& object,
    const std::string& objectPath,
    LoadedCondition& loaded)
{
    if (!reader.requireKey(object, projectjson::kPatchId, objectPath))
    {
        return false;
    }
    if (!reader.readString(
            object.value(key(projectjson::kPatchId)),
            projectJsonChildPath(objectPath, projectjson::kPatchId),
            loaded.patchText))
    {
        return false;
    }

    std::string type;
    if (reader.requireKey(object, projectjson::kType, objectPath)
        && reader.readString(
            object.value(key(projectjson::kType)),
            projectJsonChildPath(objectPath, projectjson::kType),
            type))
    {
        loaded.condition.setType(std::move(type));
    }

    if (!object.contains(key(projectjson::kParameters)))
    {
        return true;
    }

    const std::string parametersPath = projectJsonChildPath(objectPath, projectjson::kParameters);
    QJsonObject parameters;
    if (!reader.readObject(object.value(key(projectjson::kParameters)), parametersPath, parameters))
    {
        return false;
    }

    for (auto it = parameters.constBegin(); it != parameters.constEnd(); ++it)
    {
        std::string text;
        const std::string entryPath = projectJsonChildPath(parametersPath, it.key().toUtf8().constData());
        if (reader.readString(it.value(), entryPath, text))
        {
            loaded.condition.setParameter(it.key().toStdString(), std::move(text));
        }
    }
    return true;
}

bool readConditions(
    ProjectJsonValueReader& reader,
    const QJsonObject& fieldObject,
    const std::string& fieldPath,
    std::vector<LoadedCondition>& conditions)
{
    if (!fieldObject.contains(key(projectjson::kBoundaryField)))
    {
        return true;
    }

    const std::string path = projectJsonChildPath(fieldPath, projectjson::kBoundaryField);
    QJsonArray array;
    if (!reader.readArray(fieldObject.value(key(projectjson::kBoundaryField)), path, array))
    {
        return false;
    }

    for (int index = 0; index < array.size(); ++index)
    {
        const std::string elementPath = projectJsonIndexPath(path, static_cast<std::size_t>(index));
        QJsonObject object;
        if (!reader.readObject(array.at(index), elementPath, object))
        {
            continue;
        }
        LoadedCondition loaded;
        if (readCondition(reader, object, elementPath, loaded))
        {
            conditions.push_back(std::move(loaded));
        }
    }
    return true;
}

} // namespace

void projectJsonReadFields(
    ProjectJsonValueReader& reader,
    const QJsonObject& root,
    core::CaseProject& project)
{
    if (!root.contains(key(projectjson::kFields)))
    {
        return;
    }

    const std::string path = projectjson::kFields;
    QJsonArray array;
    if (!reader.readArray(root.value(key(projectjson::kFields)), path, array))
    {
        return;
    }

    std::set<std::string> seenIds;
    for (int index = 0; index < array.size(); ++index)
    {
        const std::string elementPath = projectJsonIndexPath(path, static_cast<std::size_t>(index));
        QJsonObject object;
        if (!reader.readObject(array.at(index), elementPath, object))
        {
            continue;
        }

        if (!reader.requireKey(object, projectjson::kId, elementPath))
        {
            continue;
        }
        const std::string idPath = projectJsonChildPath(elementPath, projectjson::kId);
        std::string idText;
        if (!reader.readString(object.value(key(projectjson::kId)), idPath, idText))
        {
            continue;
        }
        const auto id = core::ObjectId::fromString(idText);
        if (!id)
        {
            reader.error(projectjson::kInvalidValue, idPath, "'" + idText + "' is not a valid object id");
            continue;
        }
        if (!seenIds.insert(id->toString()).second)
        {
            reader.error(projectjson::kDuplicateId, idPath, "duplicate field id '" + idText + "'");
            continue;
        }

        std::string name;
        if (!reader.requireKey(object, projectjson::kName, elementPath)
            || !reader.readString(
                object.value(key(projectjson::kName)),
                projectJsonChildPath(elementPath, projectjson::kName),
                name))
        {
            continue;
        }

        core::FieldClass fieldClass = core::FieldClass::VolScalarField;
        if (reader.requireKey(object, projectjson::kClass, elementPath))
        {
            const std::string classPath = projectJsonChildPath(elementPath, projectjson::kClass);
            std::string classWord;
            if (reader.readString(object.value(key(projectjson::kClass)), classPath, classWord))
            {
                const auto parsed = core::fieldClassFromName(classWord);
                if (parsed)
                {
                    fieldClass = *parsed;
                }
                else
                {
                    reader.warning(
                        projectjson::kUnknownEnumWord,
                        classPath,
                        "unknown field class '" + classWord + "'; keeping 'volScalarField'");
                }
            }
        }

        core::FieldDefinition::Dimensions dimensions{};
        if (reader.requireKey(object, projectjson::kDimensions, elementPath))
        {
            std::vector<int> values;
            if (reader.readIntArray(
                    object.value(key(projectjson::kDimensions)),
                    projectJsonChildPath(elementPath, projectjson::kDimensions),
                    7,
                    values))
            {
                for (std::size_t i = 0; i < dimensions.size(); ++i)
                {
                    dimensions[i] = values[i];
                }
            }
        }

        core::FieldValue internalField;
        if (object.contains(key(projectjson::kInternalField)))
        {
            readInternalField(reader, object, elementPath, internalField);
        }

        std::vector<LoadedCondition> conditions;
        readConditions(reader, object, elementPath, conditions);

        const core::ObjectId created = project.addField(name, fieldClass, dimensions);
        core::FieldDefinition* field = project.findField(created);
        if (field == nullptr)
        {
            continue;
        }

        field->setId(*id);
        field->setInternalField(std::move(internalField));

        for (LoadedCondition& loaded : conditions)
        {
            const auto patchId = core::ObjectId::fromString(loaded.patchText);
            if (!patchId)
            {
                reader.error(
                    projectjson::kInvalidValue,
                    projectJsonChildPath(elementPath, projectjson::kBoundaryField),
                    "'" + loaded.patchText + "' is not a valid patch id");
                continue;
            }
            if (project.findPatch(*patchId) == nullptr)
            {
                reader.warning(
                    projectjson::kUnknownPatchReference,
                    projectJsonChildPath(elementPath, projectjson::kBoundaryField),
                    "boundary condition refers to unknown patch '" + loaded.patchText
                        + "'; the condition is kept so it can be repaired");
            }
            field->setBoundaryCondition(*patchId, std::move(loaded.condition));
        }
    }
}

} // namespace ofgui::io
