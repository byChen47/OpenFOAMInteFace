#include "io/writers/FieldFileWriter.h"

#include "io/writers/FoamLayout.h"
#include "io/writers/FoamNumberFormat.h"

#include <string>
#include <utility>
#include <vector>

namespace ofgui::io {
namespace {

constexpr int kTopLevel = 0;
constexpr int kPatchIndent = 1;
constexpr int kConditionIndent = 2;

std::string internalFieldText(const core::FieldValue& value)
{
    switch (value.kind())
    {
    case core::FieldValueKind::UniformScalar:
        return "uniform " + formatFoamReal(value.scalar());
    case core::FieldValueKind::UniformVector:
        return "uniform " + formatFoamVector(value.vector());
    case core::FieldValueKind::NonUniformRaw:
        return "nonuniform " + value.rawText();
    }
    return {};
}

void appendCondition(std::string& text, const core::FieldDefinition& field, const core::BoundaryPatch& patch)
{
    const core::BoundaryCondition* condition = field.findBoundaryCondition(patch.id());
    if (condition == nullptr || condition->isEmpty())
    {
        return;
    }

    text += foamIndent(kPatchIndent);
    text += patch.name();
    text += '\n';
    text += foamIndent(kPatchIndent);
    text += "{\n";
    text += foamEntryLine(kConditionIndent, "type", condition->type());
    text += '\n';

    for (const auto& parameter : condition->parameters())
    {
        if (parameter.first == "type")
        {
            continue;
        }
        text += foamEntryLine(kConditionIndent, parameter.first, parameter.second);
        text += '\n';
    }

    text += foamIndent(kPatchIndent);
    text += "}\n";
}

} // namespace

std::string FieldFileWriter::write(const core::FieldDefinition& field, const core::CaseProject& project)
{
    std::string text = foamFileHeader(core::fieldClassName(field.fieldClass()), field.name());
    text += foamEntryLine(kTopLevel, "dimensions", formatFoamDimensions(field.dimensions()));
    text += "\n\n";
    text += foamEntryLine(kTopLevel, "internalField", internalFieldText(field.internalField()));
    text += "\n\n";

    text += "boundaryField\n{\n";
    bool first = true;
    for (const core::BoundaryPatch& patch : project.patches())
    {
        const core::BoundaryCondition* condition = field.findBoundaryCondition(patch.id());
        if (condition == nullptr || condition->isEmpty())
        {
            continue;
        }
        if (!first)
        {
            text += '\n';
        }
        appendCondition(text, field, patch);
        first = false;
    }
    text += "}\n";
    text += "\n\n";
    text += foamTrailer();
    return text;
}

} // namespace ofgui::io
