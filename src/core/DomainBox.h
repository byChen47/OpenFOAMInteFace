#pragma once

#include "core/Basics.h"
#include "core/Vec3d.h"

#include <string>

namespace ofgui::core {

/// Spatial mode of the case.
enum class DomainMode
{
    TwoD,
    ThreeD
};

/// Axis used as the thickness direction in 2D mode (z, i.e. axis index 2).
inline constexpr int kTwoDThicknessAxis = 2;

/// Axis aligned bounding box of the (single block) computational domain.
///
/// The class is a pure value holder: it exposes the raw values plus the
/// predicates the validator needs (finite, positive extent). It deliberately
/// does not clamp or reorder `min`/`max`; an inverted axis is a user error that
/// `CaseValidator` reports so the GUI can point at the offending spin box.
class DomainBox
{
public:
    DomainBox() = default;

    DomainBox(const Vec3d& min, const Vec3d& max);

    [[nodiscard]] const Vec3d& min() const noexcept;
    [[nodiscard]] const Vec3d& max() const noexcept;

    void setMin(const Vec3d& min) noexcept;
    void setMax(const Vec3d& max) noexcept;

    [[nodiscard]] DomainMode mode() const noexcept;
    void setMode(DomainMode mode) noexcept;

    /// Unit of `min`/`max`. v2206 dictionaries carry no unit information, so the
    /// first version always reports `"m"`.
    [[nodiscard]] const std::string& unit() const noexcept;
    void setUnit(std::string unit);

    /// `max - min`; negative components mean the axis is inverted.
    [[nodiscard]] Vec3d size() const noexcept;

    [[nodiscard]] bool isFinite() const noexcept;

    /// True when every axis is strictly positive (`min < max` on x, y and z).
    [[nodiscard]] bool hasPositiveExtent() const noexcept;

    /// Product of the extents, or `0` when the box is empty or inverted.
    [[nodiscard]] Real volume() const noexcept;

    [[nodiscard]] Real largestExtent() const noexcept;
    [[nodiscard]] Real smallestExtent() const noexcept;

    /// Heuristic used by the import report and the "2D?" hint in the GUI: a box
    /// whose thinnest extent is at most `relativeThickness` of the thickest one
    /// looks like a 2D case. Never changes `mode()` by itself.
    [[nodiscard]] DomainMode suggestMode(Real relativeThickness = 0.01) const noexcept;

private:
    Vec3d min_;
    Vec3d max_;
    DomainMode mode_ = DomainMode::ThreeD;
    std::string unit_ = "m";
};

} // namespace ofgui::core
