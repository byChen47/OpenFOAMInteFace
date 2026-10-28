#pragma once

#include "core/DomainBox.h"
#include "core/MeshSettings.h"

#include <QStringList>
#include <QWidget>

#include <array>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QSpinBox;

namespace ofgui {

class CaseDocumentController;

/// Editor for the computational domain and the block mesh resolution.
///
/// The editor is the "input -> model -> 3D view" link of t9:
///  - every committed spin box value is validated against the model invariants
///    (`max > min` per axis, `cells > 0`, 2D forces a single z layer);
///  - invalid input shows a message and never reaches `CaseProject`, so the
///    scene keeps the last valid geometry;
///  - a valid edit writes domain + mesh and calls
///    `CaseDocumentController::notifyProjectEdited()`, which rebuilds the tree,
///    re-runs validation and lets the viewport update the six face actors.
class DomainEditor final : public QWidget
{
    Q_OBJECT

public:
    explicit DomainEditor(QWidget* parent = nullptr);

    void setDocument(CaseDocumentController* document);

    /// Reloads the widgets from the document (no model writes, no dirty flag).
    void refresh();

    [[nodiscard]] QString errorText() const;
    [[nodiscard]] QString hintText() const;

public slots:
    void onValueChanged();
    void onModeChanged();

private:
    /// Domain + mesh read from the widgets plus the messages they produced.
    struct Candidate
    {
        core::DomainBox domain;
        core::MeshSettings mesh;
        QStringList errors;
        QStringList hints;
    };

    [[nodiscard]] Candidate readCandidate() const;
    void applyCandidate();
    void updateDerived(const core::DomainBox& domain, const core::MeshSettings& mesh) const;
    void showMessages(const QStringList& errors, const QStringList& hints);

    CaseDocumentController* m_document = nullptr;
    bool m_updating = false;

    QComboBox* m_mode = nullptr;
    std::array<QDoubleSpinBox*, 3> m_min{};
    std::array<QDoubleSpinBox*, 3> m_max{};
    std::array<QSpinBox*, 3> m_cells{};

    QLabel* m_sizeLabel = nullptr;
    QLabel* m_volumeLabel = nullptr;
    QLabel* m_cellsLabel = nullptr;
    QLabel* m_errorLabel = nullptr;
    QLabel* m_hintLabel = nullptr;
};

} // namespace ofgui
