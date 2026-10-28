#include "io/FieldProjector.h"

#include "io/FoamDictionary.h"
#include "io/FoamNode.h"
#include "io/FoamToken.h"
#include "io/FoamValue.h"
#include "io/ImportCodes.h"
#include "io/ImportIssueFactory.h"

#include <cstddef>
#include <filesystem>
#include <map>
#include <string>

namespace ofgui::io {
namespace {

constexpr std::size_t kNonUniformElementLimit = 5000000;

std::string fileNameOf(std::string_view relativePath)
{
    const std::size_t slash = relativePath.find_last_of('/');
    const std::string_view name = slash == std::string_view::npos ? relativePath : relativePath.substr(slash + 1);
    if (name.size() > 3 && name.compare(name.size() - 3, 3, ".gz") == 0)
    {
        return std::string(name.substr(0, name.size() - 3));
    }
    return std::string(name);
}

bool toReal(const FoamNode& node, double& value)
{
    return node.isLeaf() && parseFoamReal(node.rawText(), value);
}

std::string textOf(const FoamNode& node)
{
    return node.isLeaf() ? node.rawText() : FoamValue(node).text();
}

/// Warns about `$` constructs the importer keeps verbatim (spec section 5.4).
void reportVariableNotes(
    const std::string& text,
    std::string_view file,
    int line,
    int column,
    std::string_view nodePath,
    std::vector<core::ValidationIssue>& issues)
{
    if (text.find("${{") != std::string::npos)
    {
        issues.push_back(
            makeImportIssue(importcodes::kSynExpression, file, line, column, std::string(nodePath), text));
    }
    if (text.find("$..") != std::string::npos || text.find("$^") != std::string::npos)
    {
        issues.push_back(makeImportIssue(
            importcodes::kVarUnsupportedScope, file, line, column, std::string(nodePath), text));
    }
}

core::FieldValue readInternalValue(
    const FoamNode* node,
    std::string_view file,
    std::string_view nodePath,
    std::vector<core::ValidationIssue>& issues)
{
    if (node == nullptr)
    {
        return {};
    }
    const std::string text = textOf(*node);
    const int line = node->location().line;
    const int column = node->location().column;
    reportVariableNotes(text, file, line, column, nodePath, issues);

    if (node->isList())
    {
        std::array<double, 3> vector{};
        if (const FoamValue value(*node); value.asVector(vector))
        {
            return core::FieldValue::uniformVector(core::Vec3d(vector[0], vector[1], vector[2]));
        }
        return core::FieldValue::nonUniformRaw(text);
    }
    if (node->isSequence())
    {
        const FoamNode* first = node->childAt(0);
        const std::string head = first != nullptr ? first->rawText() : std::string{};
        if (head == "nonuniform")
        {
            if (node->childCount() > 1 && node->childAt(1) != nullptr && node->childAt(1)->isList())
            {
                const std::size_t count = node->childAt(1)->childCount();
                if (count > kNonUniformElementLimit)
                {
                    issues.push_back(makeImportIssue(
                        importcodes::kLimitNonUniform, file, line, column, std::string(nodePath)));
                }
            }
            return core::FieldValue::nonUniformRaw(text);
        }
        if (head == "uniform")
        {
            const FoamNode* data = node->childAt(1);
            if (data == nullptr)
            {
                return core::FieldValue::nonUniformRaw(text);
            }
            double scalar = 0.0;
            if (data->isLeaf() && toReal(*data, scalar))
            {
                return core::FieldValue::uniformScalar(scalar);
            }
            std::array<double, 3> vector{};
            if (const FoamValue value(*data); value.asVector(vector))
            {
                return core::FieldValue::uniformVector(core::Vec3d(vector[0], vector[1], vector[2]));
            }
            return core::FieldValue::nonUniformRaw(text);
        }
        return core::FieldValue::nonUniformRaw(text);
    }

    double scalar = 0.0;
    if (node->isLeaf() && toReal(*node, scalar))
    {
        return core::FieldValue::uniformScalar(scalar);
    }
    return core::FieldValue::nonUniformRaw(text);
}

} // namespace

ProjectedField FieldProjector::project(const FoamDictionary& dictionary, const CaseProjectionContext& context)
{
    ProjectedField field;
    const std::string& file = context.relativePath;

    const FoamNode* header = dictionary.header();
    if (header == nullptr)
    {
        field.headerMissing = true;
        field.name = fileNameOf(file);
        return field;
    }

    const FoamNode* classNode = header->findChild("class");
    field.classText = classNode != nullptr ? classNode->rawText() : std::string{};
    if (const std::optional<core::FieldClass> fieldClass = core::fieldClassFromName(field.classText))
    {
        field.fieldClass = fieldClass;
    }

    const FoamNode* objectNode = header->findChild("object");
    if (objectNode != nullptr)
    {
        field.name = objectNode->rawText();
    }
    else
    {
        field.name = fileNameOf(file);
        context.issues.push_back(makeImportIssue(importcodes::kParseObjectMissing,
                                                 file,
                                                 header->location().line,
                                                 header->location().column,
                                                 "FoamFile.object",
                                                 field.name));
    }

    // dimensions: exactly seven exponents, kept verbatim
    if (const FoamNode* dimensions = dictionary.find("dimensions");
        dimensions != nullptr && dimensions->isDimensionSet() && dimensions->childCount() == 7)
    {
        bool ok = true;
        for (std::size_t axis = 0; axis < 7; ++axis)
        {
            long long exponent = 0;
            const FoamNode* child = dimensions->childAt(axis);
            if (child == nullptr || !parseFoamInt(child->rawText(), exponent))
            {
                ok = false;
                break;
            }
            field.dimensions[axis] = static_cast<int>(exponent);
        }
        field.hasDimensions = ok;
        if (!ok)
        {
            context.issues.push_back(makeImportIssue(
                importcodes::kFieldMissingDimensions,
                file,
                dimensions->location().line,
                dimensions->location().column,
                "dimensions",
                FoamValue(*dimensions).text()));
        }
    }
    else
    {
        context.issues.push_back(makeImportIssue(importcodes::kFieldMissingDimensions,
                                                file,
                                                0,
                                                0,
                                                "dimensions",
                                                ""));
    }

    field.internalField = readInternalValue(
        dictionary.find("internalField"), file, "internalField", context.issues);

    // boundaryField
    if (const FoamNode* boundaryField = dictionary.find("boundaryField");
        boundaryField != nullptr && boundaryField->isDictionary())
    {
        for (const auto& patchNode : boundaryField->children())
        {
            if (patchNode == nullptr || !patchNode->isDictionary())
            {
                continue;
            }
            ProjectedBoundaryCondition projected;
            projected.patchName = patchNode->keyword();
            projected.regularExpression = patchNode->keywordIsString();
            projected.line = patchNode->location().line;
            projected.column = patchNode->location().column;
            if (projected.regularExpression)
            {
                context.issues.push_back(makeImportIssue(importcodes::kPatchRegexKey,
                                                        file,
                                                        projected.line,
                                                        projected.column,
                                                        projected.patchName,
                                                        projected.patchName));
                continue;
            }
            const std::string nodePath = "boundaryField." + projected.patchName;
            std::map<std::string, std::string> parameters;
            std::string typeText;
            for (const auto& valueNode : patchNode->children())
            {
                if (valueNode == nullptr)
                {
                    continue;
                }
                if (valueNode->keyword() == "type")
                {
                    typeText = textOf(*valueNode);
                    continue;
                }
                parameters.emplace(valueNode->keyword(), textOf(*valueNode));
            }
            if (typeText.empty())
            {
                context.issues.push_back(
                    makeImportIssue(importcodes::kBoundaryMissingType, file, projected.line, projected.column, nodePath + ".type"));
                continue;
            }
            projected.hasType = true;
            if (const std::optional<core::BoundaryCondition> condition =
                    BoundaryConditionFactory::create(typeText,
                                                     std::move(parameters),
                                                     file,
                                                     projected.line,
                                                     projected.column,
                                                     nodePath,
                                                     context.issues))
            {
                projected.condition = *condition;
                field.boundaryField.push_back(std::move(projected));
            }
        }
    }

    return field;
}

} // namespace ofgui::io
