#include "io/ProjectJsonReadSections.h"

#include "core/BoundaryPatch.h"
#include "core/CaseProject.h"
#include "core/FaceId.h"
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

} // namespace

void projectJsonReadPatches(
    ProjectJsonValueReader& reader,
    const QJsonObject& root,
    core::CaseProject& project)
{
    if (!root.contains(key(projectjson::kPatches)))
    {
        return;
    }

    const std::string path = projectjson::kPatches;
    QJsonArray array;
    if (!reader.readArray(root.value(key(projectjson::kPatches)), path, array))
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
            reader.error(projectjson::kDuplicateId, idPath, "duplicate patch id '" + idText + "'");
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

        core::PatchType type = core::PatchType::Patch;
        if (reader.requireKey(object, projectjson::kType, elementPath))
        {
            const std::string typePath = projectJsonChildPath(elementPath, projectjson::kType);
            std::string typeWord;
            if (reader.readString(object.value(key(projectjson::kType)), typePath, typeWord))
            {
                const auto parsed = core::patchTypeFromName(typeWord);
                if (parsed)
                {
                    type = *parsed;
                }
                else
                {
                    reader.warning(
                        projectjson::kUnknownEnumWord,
                        typePath,
                        "unknown patch type '" + typeWord + "'; keeping 'patch'");
                }
            }
        }

        std::set<core::FaceId> faces;
        if (object.contains(key(projectjson::kFaces)))
        {
            const std::string facesPath = projectJsonChildPath(elementPath, projectjson::kFaces);
            QJsonArray faceArray;
            if (reader.readArray(object.value(key(projectjson::kFaces)), facesPath, faceArray))
            {
                for (int faceIndex = 0; faceIndex < faceArray.size(); ++faceIndex)
                {
                    const std::string facePath =
                        projectJsonIndexPath(facesPath, static_cast<std::size_t>(faceIndex));
                    std::string faceWord;
                    if (!reader.readString(faceArray.at(faceIndex), facePath, faceWord))
                    {
                        continue;
                    }
                    const auto face = projectjson::faceIdFromName(faceWord);
                    if (!face)
                    {
                        reader.warning(
                            projectjson::kUnknownEnumWord,
                            facePath,
                            "unknown face '" + faceWord + "'; ignored");
                        continue;
                    }
                    faces.insert(*face);
                }
            }
        }

        std::array<core::Real, 3> color{0.8, 0.8, 0.8};
        if (object.contains(key(projectjson::kColor)))
        {
            std::vector<core::Real> values;
            if (reader.readRealArray(
                    object.value(key(projectjson::kColor)),
                    projectJsonChildPath(elementPath, projectjson::kColor),
                    3,
                    values))
            {
                color = {values[0], values[1], values[2]};
            }
        }

        bool visible = true;
        if (object.contains(key(projectjson::kVisible)))
        {
            bool parsedVisible = visible;
            if (reader.readBool(
                    object.value(key(projectjson::kVisible)),
                    projectJsonChildPath(elementPath, projectjson::kVisible),
                    parsedVisible))
            {
                visible = parsedVisible;
            }
        }

        const core::ObjectId created = project.addPatch(name, type);
        if (core::BoundaryPatch* patch = project.findPatch(created))
        {
            patch->setId(*id);
            patch->setFaces(std::move(faces));
            patch->setColor(color);
            patch->setVisible(visible);
        }
    }
}

} // namespace ofgui::io
