#include "core/CaseValidator.h"

// Entry point only: the per-family rules live in CaseValidatorDomain.cpp,
// CaseValidatorMesh.cpp, CaseValidatorPatches.cpp, CaseValidatorFields.cpp and
// CaseValidatorSolver.cpp (docs/CASE_IMPORT_SPEC.md §10.9, D-021).

namespace ofgui::core {

ValidationResult CaseValidator::validate(const CaseProject& project) const
{
    ValidationResult result;
    validateDomain(project, result);
    validateMesh(project, result);
    validatePatches(project, result);
    validateFields(project, result);
    validateSolver(project, result);
    return result;
}

} // namespace ofgui::core
