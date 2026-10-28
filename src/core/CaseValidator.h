#pragma once

#include "core/CaseProject.h"
#include "core/ValidationResult.h"

namespace ofgui::core {

/// Model level validation of a `CaseProject`: geometry, mesh, patch coverage,
/// names, field completeness and solver settings.
///
/// The validator never touches the file system and never mutates the project;
/// it only reports what is wrong so the panels can show inline hints and the
/// export button can be disabled while a `BlockingError` exists.
class CaseValidator
{
public:
    CaseValidator() = default;

    [[nodiscard]] ValidationResult validate(const CaseProject& project) const;

private:
    void validateDomain(const CaseProject& project, ValidationResult& result) const;
    void validateMesh(const CaseProject& project, ValidationResult& result) const;
    void validatePatches(const CaseProject& project, ValidationResult& result) const;
    void validateFields(const CaseProject& project, ValidationResult& result) const;
    void validateSolver(const CaseProject& project, ValidationResult& result) const;
};

} // namespace ofgui::core
