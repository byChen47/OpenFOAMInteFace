#pragma once

#include "core/CaseProject.h"
#include "core/ValidationResult.h"

#include "io/CaseImportResult.h"
#include "io/ImportReport.h"
#include "io/ProjectJsonExtras.h"
#include "io/writers/TransportModelSettings.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

#include <optional>
#include <vector>

namespace ofgui {

/// Owns the document the whole GUI edits: one `CaseProject`, the unknown
/// `.ofgproj` keys that must survive a save, and the file it was loaded from.
///
/// Responsibilities (docs/ARCHITECTURE.md §5.2):
///  - creating an empty project, importing a case directory, loading/saving the
///    project file;
///  - the dirty flag and the validation result derived from the current model;
///  - reporting failures as text the window can show, never as silent state.
///
/// The controller performs file I/O through `ofgui_io` only; it never parses a
/// dictionary or builds UI widgets.
class CaseDocumentController final : public QObject
{
    Q_OBJECT

public:
    explicit CaseDocumentController(QObject* parent = nullptr);

    [[nodiscard]] const core::CaseProject& project() const noexcept;
    [[nodiscard]] core::CaseProject& project() noexcept;

    [[nodiscard]] const core::ValidationResult& validation() const noexcept;

    /// Import report of the last `openCaseDirectory()`; `nullptr` for a new
    /// project or after loading a `.ofgproj`.
    [[nodiscard]] const io::ImportReport* importReport() const noexcept;

    /// Settings the importer kept verbatim (`fvSchemes`, `nu`, ...).
    [[nodiscard]] const std::vector<io::ImportedSetting>& readOnlySettings() const noexcept;

    [[nodiscard]] const io::TransportPropertiesProjection& transport() const noexcept;

    /// Editable `constant/transportProperties` values (`transportModel`, `nu`).
    /// The exporter consumes this object, so a preview built from it cannot
    /// drift from the written file.
    [[nodiscard]] io::TransportModelSettings& transportSettings() noexcept;
    [[nodiscard]] const io::TransportModelSettings& transportSettings() const noexcept;

    /// True when `nu` can be edited as a plain number. A dimensioned `nu`
    /// (`nu [0 2 -1 0 0 0 0] 1e-05`) or a `$variable` value stays read only,
    /// because rewriting it as a bare number would change its meaning.
    [[nodiscard]] bool isKinematicViscosityEditable() const noexcept;

    /// `simulationType` of `constant/turbulenceProperties` when the source case
    /// defined it; empty otherwise. Only `laminar` is supported by the current
    /// exporter, every other value is shown read only.
    [[nodiscard]] QString turbulenceSimulationType() const;

    /// Read-only source text of a case dictionary, keyed by its case relative
    /// POSIX path (`system/fvSchemes`, ...). Empty when the imported case does
    /// not contain it.
    [[nodiscard]] QString sourceDictionaryText(const QString& caseRelativePath) const;
    [[nodiscard]] QStringList availableSourceDictionaries() const;

    [[nodiscard]] QString projectFilePath() const;
    [[nodiscard]] QString displayName() const;
    [[nodiscard]] QString caseDirectory() const;
    [[nodiscard]] bool isDirty() const noexcept;

    /// Human readable explanation of the last failed operation.
    [[nodiscard]] QString lastError() const;

public slots:
    /// Replaces the document with an empty project (no file path, not dirty).
    void newProject();

    /// Imports an OpenFOAM case directory. The directory is only read.
    /// @return false and sets `lastError()` when the import is blocking.
    bool openCaseDirectory(const QString& directory);

    /// Loads a `.ofgproj` file.
    bool openProjectFile(const QString& path);

    /// Saves to the current project file. Fails when no path is known.
    bool saveProject();

    /// Saves to `path` (appends `.ofgproj` when it has no suffix) and remembers it.
    bool saveProjectAs(const QString& path);

    /// Marks the document dirty after an editor changed the model, refreshes the
    /// validation result and republishes the project.
    void notifyProjectEdited();

    /// Marks the document dirty after `transportSettings()` changed. The
    /// geometric project is untouched, so only the document state is emitted.
    void notifyTransportEdited();

signals:
    void projectChanged();
    void documentStateChanged();
    void importReportChanged();
    void validationChanged();
    void errorOccurred(const QString& message);

private:
    void refreshValidation();
    void setDirty(bool dirty);
    void setError(const QString& message);
    /// Replaces the document with the bundled cavity template (t12): a new
    /// project must be exportable without any manual edit.
    void applyDefaultTemplate(const QString& projectName);

    /// Reads `system/controlDict`, `system/fvSchemes`, `system/fvSolution`,
    /// `constant/transportProperties` and `constant/turbulenceProperties` from
    /// `caseRoot` (read only) and caches their source text plus the derived
    /// transport/turbulence values. Missing or unreadable files are skipped; the
    /// import report already explains them.
    void loadCaseDictionaries(const QString& caseRoot);
    void clearCaseDictionaries();

    core::CaseProject m_project;
    io::ProjectJsonExtras m_extras;
    std::optional<io::ImportReport> m_importReport;
    std::vector<io::ImportedSetting> m_readOnlySettings;
    io::TransportPropertiesProjection m_transport;
    io::TransportModelSettings m_transportSettings;
    bool m_kinematicViscosityEditable = true;
    QString m_turbulenceSimulationType;
    QHash<QString, QString> m_sourceDictionaries;
    core::ValidationResult m_validation;
    QString m_projectFilePath;
    QString m_lastError;
    bool m_dirty = false;
};

} // namespace ofgui
