#include "io/writers/ExportPlanBuilder.h"

#include "io/FoamDictionary.h"
#include "io/FoamParseOptions.h"
#include "io/ImportCodes.h"
#include "io/ImportIssueFactory.h"
#include "io/writers/BlockMeshDictWriter.h"
#include "io/writers/CaseMirrorPlanner.h"
#include "io/writers/ControlDictWriter.h"
#include "io/writers/FieldFileWriter.h"
#include "io/writers/FvSchemesWriter.h"
#include "io/writers/FvSolutionWriter.h"
#include "io/writers/TransportPropertiesWriter.h"

#include <string>
#include <string_view>
#include <utility>

namespace ofgui::io {
namespace {

constexpr std::string_view kDefaultFvSchemes = R"(ddtSchemes
{
    default         Euler;
}

gradSchemes
{
    default         Gauss linear;
    grad(p)         Gauss linear;
}

divSchemes
{
    default         none;
    div(phi,U)      Gauss linear;
}

laplacianSchemes
{
    default         Gauss linear orthogonal;
}

interpolationSchemes
{
    default         linear;
}

snGradSchemes
{
    default         orthogonal;
}
)";

constexpr std::string_view kDefaultFvSolution = R"(solvers
{
    p
    {
        solver          PCG;
        preconditioner  DIC;
        tolerance       1e-06;
        relTol          0.05;
    }

    pFinal
    {
        $p;
        relTol          0;
    }

    U
    {
        solver          smoothSolver;
        smoother        symGaussSeidel;
        tolerance       1e-05;
        relTol          0;
    }
}

PISO
{
    nCorrectors     2;
    nNonOrthogonalCorrectors 0;
    pRefCell        0;
    pRefValue       0;
}
)";

std::string startDirectoryName(const CaseExportOptions& options)
{
    return options.startDirectory.empty() ? std::string{"0"} : options.startDirectory;
}

std::string joinPath(const std::string& directory, const std::string& name)
{
    if (directory.empty())
    {
        return name;
    }
    return directory + '/' + name;
}

bool containsNonAscii(std::string_view text)
{
    for (const char character : text)
    {
        if (static_cast<unsigned char>(character) > 0x7F)
        {
            return true;
        }
    }
    return false;
}

void appendFile(
    CaseExportPlan& plan,
    std::string relativePath,
    std::string text,
    std::vector<core::ValidationIssue>& issues)
{
    if (containsNonAscii(text))
    {
        issues.push_back(makeImportIssue(
            importcodes::kExportNonAsciiText, relativePath, 0, 0, relativePath, {}, {}));
    }
    ExportFile file;
    file.relativePath = std::move(relativePath);
    file.byteSize = text.size();
    file.text = std::move(text);
    plan.files.push_back(std::move(file));
}

} // namespace

CaseExportPlan ExportPlanBuilder::build(
    const core::CaseProject& project,
    const CaseExportOptions& options,
    std::vector<core::ValidationIssue>& issues)
{
    CaseExportPlan plan;
    const std::string start = startDirectoryName(options);

    for (const core::FieldDefinition& field : project.fields())
    {
        appendFile(
            plan,
            joinPath(start, field.name()),
            FieldFileWriter::write(field, project),
            issues);
    }

    appendFile(plan, "system/blockMeshDict", BlockMeshDictWriter::write(project), issues);
    appendFile(plan, "system/controlDict", ControlDictWriter::write(project), issues);

    // The built-in defaults are header-less fragments: the writers add the
    // `FoamFile` block themselves, so requiring a header here would reject our
    // own templates. Caller supplied text must be a complete dictionary.
    FoamParseOptions defaultOptions;
    defaultOptions.requireFoamFileHeader = false;

    const bool usesDefaultSchemes = options.fvSchemesText.empty();
    const std::string schemesText =
        usesDefaultSchemes ? std::string{kDefaultFvSchemes} : options.fvSchemesText;
    const FoamDictionary schemes = FoamDictionary::parse(
        schemesText, "system/fvSchemes", usesDefaultSchemes ? defaultOptions : FoamParseOptions{});
    if (schemes.ok())
    {
        appendFile(plan, "system/fvSchemes", FvSchemesWriter::write(schemes), issues);
    }
    else
    {
        // Never export an empty dictionary behind a success report: keep the
        // caller's text verbatim and surface the parser diagnostics. They are
        // Error severity, so CaseExporter refuses to write the case at all.
        appendDictionaryDiagnostics(issues, schemes.diagnostics(), "system/fvSchemes", "fvSchemes");
        appendFile(plan, "system/fvSchemes", schemesText, issues);
    }

    const bool usesDefaultSolution = options.fvSolutionText.empty();
    const std::string solutionText =
        usesDefaultSolution ? std::string{kDefaultFvSolution} : options.fvSolutionText;
    const FoamDictionary solution = FoamDictionary::parse(
        solutionText, "system/fvSolution", usesDefaultSolution ? defaultOptions : FoamParseOptions{});
    if (solution.ok())
    {
        appendFile(plan, "system/fvSolution", FvSolutionWriter::write(solution), issues);
    }
    else
    {
        appendDictionaryDiagnostics(issues, solution.diagnostics(), "system/fvSolution", "fvSolution");
        appendFile(plan, "system/fvSolution", solutionText, issues);
    }

    if (options.writePlaceholderDictionaries)
    {
        appendFile(
            plan,
            "constant/transportProperties",
            TransportPropertiesWriter::write(options.transport),
            issues);
    }

    if (options.mirrorSourceFiles)
    {
        CaseMirrorPlanner::collect(project, options, plan, issues);
    }

    return plan;
}

} // namespace ofgui::io
