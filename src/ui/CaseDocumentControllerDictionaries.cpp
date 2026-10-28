// Dictionary cache concern of CaseDocumentController: the raw source text of
// system/ and constant/ dictionaries plus the transport projection derived from
// constant/transportProperties. Kept in its own translation unit (D-021).
#include "ui/CaseDocumentController.h"

#include "io/FoamDictionary.h"
#include "io/TransportPropertiesProjector.h"

#include "ui/CaseDocumentControllerPaths.h"

#include <QFileInfo>
#include <QStringList>

#include <filesystem>
#include <string>
#include <vector>

namespace ofgui {
namespace {

/// Paths of the dictionaries the solver editor shows. `controlDict` and
/// `transportProperties` are editable through the model, the remaining two are
/// shown as their imported source text.
constexpr const char* kControlDictPath = "system/controlDict";
constexpr const char* kTransportPath = "constant/transportProperties";
constexpr const char* kTurbulencePath = "constant/turbulenceProperties";
constexpr const char* kSchemesPath = "system/fvSchemes";
constexpr const char* kSolutionPath = "system/fvSolution";

/// Maps the importer projection onto the settings the writer consumes.
io::TransportModelSettings toTransportSettings(
    const io::TransportPropertiesProjection& projection,
    bool& kinematicViscosityEditable)
{
    io::TransportModelSettings settings;
    settings.transportModel = projection.transportModel;
    settings.kinematicViscosity = projection.hasKinematicViscosity
        ? projection.kinematicViscosity
        : settings.kinematicViscosity;

    kinematicViscosityEditable = projection.hasKinematicViscosity
        && !projection.kinematicViscosityHasDimensions
        && projection.kinematicViscosityText.empty();
    if (!kinematicViscosityEditable)
    {
        // Keep the original spelling so a dimensioned `nu` survives the export.
        if (!projection.kinematicViscosityText.empty())
        {
            settings.kinematicViscosityText = projection.kinematicViscosityText;
        }
        else
        {
            for (const io::ImportedSetting& setting : projection.settings)
            {
                if (setting.path.size() >= 3
                    && setting.path.compare(setting.path.size() - 3, 3, ".nu") == 0)
                {
                    settings.kinematicViscosityText = setting.text;
                    break;
                }
            }
        }
    }
    return settings;
}

} // namespace

QString CaseDocumentController::sourceDictionaryText(const QString& caseRelativePath) const
{
    return m_sourceDictionaries.value(caseRelativePath);
}

QStringList CaseDocumentController::availableSourceDictionaries() const
{
    QStringList paths = m_sourceDictionaries.keys();
    paths.sort();
    return paths;
}

void CaseDocumentController::clearCaseDictionaries()
{
    m_sourceDictionaries.clear();
    m_transportSettings = {};
    m_kinematicViscosityEditable = true;
    m_turbulenceSimulationType.clear();
}

void CaseDocumentController::loadCaseDictionaries(const QString& caseRoot)
{
    clearCaseDictionaries();
    if (caseRoot.trimmed().isEmpty() || !QFileInfo(caseRoot).isDir())
    {
        return;
    }

    const std::filesystem::path root = detail::toPath(caseRoot);
    for (const char* relative : {kControlDictPath, kSchemesPath, kSolutionPath, kTransportPath, kTurbulencePath})
    {
        const std::filesystem::path file = root / std::filesystem::path(relative);
        if (!std::filesystem::is_regular_file(file))
        {
            continue;
        }
        const io::FoamDictionary dictionary = io::FoamDictionary::parseFile(file);
        if (!dictionary.sourceText().empty())
        {
            m_sourceDictionaries.insert(QString::fromLatin1(relative),
                                        QString::fromStdString(dictionary.sourceText()));
        }

        if (relative == kTransportPath)
        {
            std::vector<core::ValidationIssue> ignoredIssues;
            const io::CaseImportOptions options;
            const io::CaseProjectionContext context{std::string{relative}, ignoredIssues, options};
            m_transport = io::TransportPropertiesProjector::project(dictionary, context);
            m_transportSettings = toTransportSettings(m_transport, m_kinematicViscosityEditable);
        }
        else if (relative == kTurbulencePath)
        {
            if (const io::FoamNode* node = dictionary.find(std::string_view{"simulationType"}))
            {
                m_turbulenceSimulationType = QString::fromStdString(node->rawText());
            }
        }
    }
}

} // namespace ofgui
