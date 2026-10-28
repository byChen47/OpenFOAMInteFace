#pragma once

#include "core/Basics.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace ofgui::core {

/// Mesh resolution of the single block domain: cell counts, grading and the
/// blockMesh `scale` factor.
///
/// The class stores raw user input and only answers questions; clamping and
/// cross-checks (2D thickness, cell budget) belong to `CaseValidator`.
class MeshSettings
{
public:
    using CellCounts = std::array<int, 3>;
    using Grading = std::array<Real, 3>;

    /// Soft budget used for the "too many cells" hint (matches the 5e6 warning
    /// threshold of the case import specification).
    static constexpr std::int64_t kCellBudgetWarning = 5000000;

    MeshSettings() noexcept = default;

    explicit MeshSettings(
        const CellCounts& cells,
        const Grading& grading = Grading{1.0, 1.0, 1.0},
        Real scale = 1.0) noexcept;

    [[nodiscard]] const CellCounts& cells() const noexcept;
    void setCells(const CellCounts& cells) noexcept;

    /// Sets a single cell count. @return false when `axis` is not 0, 1 or 2.
    [[nodiscard]] bool setCellCount(std::size_t axis, int count) noexcept;

    /// Cell count on `axis`. @throws std::out_of_range when axis > 2.
    [[nodiscard]] int cellCount(std::size_t axis) const;

    [[nodiscard]] const Grading& grading() const noexcept;
    void setGrading(const Grading& grading) noexcept;

    /// Sets one grading factor. @return false when `axis` is not 0, 1 or 2.
    [[nodiscard]] bool setGradingFactor(std::size_t axis, Real factor) noexcept;

    [[nodiscard]] Real scale() const noexcept;
    void setScale(Real scale) noexcept;

    /// `nx * ny * nz` computed in 64 bit; `<= 0` when any axis is invalid.
    [[nodiscard]] std::int64_t totalCells() const noexcept;

    [[nodiscard]] bool hasPositiveCells() const noexcept;
    [[nodiscard]] bool hasPositiveGrading() const noexcept;

    /// Cells > 0, grading > 0 and finite, scale > 0 and finite.
    [[nodiscard]] bool isValid() const noexcept;

    /// True when the given axis has exactly one cell layer (2D requirement).
    [[nodiscard]] bool isSingleCellLayer(std::size_t axis) const;

    /// True when the total cell count exceeds `kCellBudgetWarning`.
    [[nodiscard]] bool exceedsCellBudget() const noexcept;

private:
    CellCounts cells_{10, 10, 10};
    Grading grading_{1.0, 1.0, 1.0};
    Real scale_ = 1.0;
};

} // namespace ofgui::core
