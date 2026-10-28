#pragma once

#include <core/CaseProject.h>
#include <core/FieldDefinition.h>

#include <string>

namespace ofgui::io {

/// Writes a `0/<field>` file (for example `0/p` or `0/U`) for one field of a
/// project. Boundary conditions are emitted in project patch order; the `type`
/// parameter always comes first, the remaining parameters follow in key order.
class FieldFileWriter
{
public:
    static std::string write(const core::FieldDefinition& field, const core::CaseProject& project);
};

} // namespace ofgui::io
