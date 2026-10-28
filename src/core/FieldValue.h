#pragma once

#include "core/Basics.h"
#include "core/Vec3d.h"

#include <optional>
#include <stdexcept>
#include <string>

namespace ofgui::core {

/// How the interior value of a field is stored.
enum class FieldValueKind
{
    /// `uniform <scalar>`; editable in the GUI.
    UniformScalar,
    /// `uniform (<x> <y> <z>)`; editable in the GUI.
    UniformVector,
    /// `nonuniform List<...>` or any expression the first version cannot edit;
    /// kept verbatim and written back unchanged.
    NonUniformRaw
};

/// Interior value of a field (`internalField`) in three flavours.
///
/// The accessors throw `std::logic_error` when asked for a representation the
/// value does not hold, so a field/BC mismatch is a programming error rather
/// than a silent zero.
class FieldValue
{
public:
    FieldValue() = default;

    [[nodiscard]] static FieldValue uniformScalar(Real value);
    [[nodiscard]] static FieldValue uniformVector(const Vec3d& value);
    [[nodiscard]] static FieldValue nonUniformRaw(std::string rawText);

    [[nodiscard]] FieldValueKind kind() const noexcept;

    /// False for a default constructed value (field not set yet).
    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] bool isUniformScalar() const noexcept;
    [[nodiscard]] bool isUniformVector() const noexcept;
    [[nodiscard]] bool isNonUniformRaw() const noexcept;

    /// @throws std::logic_error when `kind() != UniformScalar`.
    [[nodiscard]] Real scalar() const;

    /// @throws std::logic_error when `kind() != UniformVector`.
    [[nodiscard]] const Vec3d& vector() const;

    /// @throws std::logic_error when `kind() != NonUniformRaw`.
    [[nodiscard]] const std::string& rawText() const;

private:
    FieldValueKind kind_ = FieldValueKind::UniformScalar;
    bool assigned_ = false;
    Real scalar_ = 0.0;
    Vec3d vector_;
    std::string rawText_;
};

} // namespace ofgui::core
