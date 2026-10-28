#pragma once

#include "io/writers/CaseExportOptions.h"
#include "io/writers/CaseExportPlan.h"
#include "io/writers/CaseExporter.h"

#include <QObject>
#include <QString>

#include <vector>

namespace ofgui {

class CaseDocumentController;

/// What the export preview shows: the complete plan and every preflight issue.
struct ExportPreview
{
    bool ok{false};
    QString errorText;
    io::CaseExportPlan plan;
    std::vector<core::ValidationIssue> issues;
    bool targetHasContent{false};
};

/// Outcome of one export run, including the manifest counts read back from disk.
struct ExportOutcome
{
    bool ok{false};
    QString message;
    io::CaseExportResult result;
    std::vector<core::ValidationIssue> issues;
    int manifestFiles{0};
    bool manifestVerified{false};
    QString backupDirectory;
};

/// Runs the export workflow for the window: model validation, dictionary
/// preflight, atomic write through `io::CaseExporter`, then a manifest read-back.
///
/// The controller is deliberately free of widgets so the whole workflow can be
/// driven by tests (`tests/ui/ExportWorkflowTest.cpp`). All business data comes
/// from `CaseDocumentController`; this class only assembles `CaseExportOptions`.
class ExportController final : public QObject
{
    Q_OBJECT

public:
    explicit ExportController(CaseDocumentController* document, QObject* parent = nullptr);

    [[nodiscard]] CaseDocumentController* document() const noexcept;

    /// Builds the plan without touching the file system. `targetDirectory` may be
    /// empty, in which case only the plan is computed.
    [[nodiscard]] ExportPreview preview(const QString& targetDirectory) const;

    /// Preflight, then write the case. `allowExistingTarget` must be true only
    /// after the user confirmed replacing a non-empty directory.
    [[nodiscard]] ExportOutcome exportTo(const QString& targetDirectory, bool allowExistingTarget);

signals:
    /// Human readable progress line for the status bar.
    void progressMessage(const QString& message);

private:
    [[nodiscard]] io::CaseExportOptions makeOptions(
        const QString& targetDirectory,
        bool allowExistingTarget) const;

    [[nodiscard]] QString failureText(const std::vector<core::ValidationIssue>& issues) const;

    CaseDocumentController* m_document = nullptr;
};

} // namespace ofgui
