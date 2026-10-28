#pragma once

#include "core/Basics.h"
#include "core/FaceId.h"
#include "core/ObjectId.h"

#include <array>
#include <cstddef>
#include <optional>
#include <set>
#include <string>
#include <string_view>

namespace ofgui::core {

/// OpenFOAM patch type of a boundary face group.
///
/// The enumeration intentionally covers only the types the GUI can create; every
/// other `type` string from an imported case is kept as `Patch` plus a warning
/// (see `BoundaryCondition::type` for field level types).
enum class PatchType
{
    Patch,
    Wall,
    Empty,
    Symmetry,
    SymmetryPlane
};

/// OpenFOAM spelling of the patch type: `patch`, `wall`, `empty`, `symmetry`,
/// `symmetryPlane`.
[[nodiscard]] std::string_view patchTypeName(PatchType type) noexcept;

/// Parses an OpenFOAM patch `type` word. Unknown words return `std::nullopt`.
[[nodiscard]] std::optional<PatchType> patchTypeFromName(std::string_view name) noexcept;

/// One boundary patch: stable id + display name + the faces it owns.
///
/// The id never changes when the user renames the patch; `faces` is a plain set
/// so duplicate/overlapping assignments stay representable and are reported by
/// `FaceAssignmentValidator` instead of being silently dropped.
class BoundaryPatch
{
public:
    BoundaryPatch() = default;

    explicit BoundaryPatch(std::string name, PatchType type = PatchType::Patch);

    /// Named constructor that assigns a fresh `ObjectId`.
    [[nodiscard]] static BoundaryPatch create(
        std::string name,
        PatchType type = PatchType::Patch);

    [[nodiscard]] const ObjectId& id() const noexcept;
    void setId(const ObjectId& id) noexcept;

    [[nodiscard]] const std::string& name() const noexcept;

    /// Renames the patch. The id is untouched; the name is not validated here so
    /// the editor can show an invalid name while the user is typing.
    void setName(std::string name);

    [[nodiscard]] PatchType type() const noexcept;
    void setType(PatchType type) noexcept;

    [[nodiscard]] const std::set<FaceId>& faces() const noexcept;

    /// @return true when the face was not assigned to this patch before.
    bool addFace(FaceId face);

    /// @return true when the face was removed.
    bool removeFace(FaceId face);

    void setFaces(std::set<FaceId> faces);

    [[nodiscard]] bool hasFace(FaceId face) const noexcept;
    [[nodiscard]] std::size_t faceCount() const noexcept;

    /// Display color, components in `[0, 1]`.
    [[nodiscard]] const std::array<Real, 3>& color() const noexcept;
    void setColor(const std::array<Real, 3>& color) noexcept;

    [[nodiscard]] bool isVisible() const noexcept;
    void setVisible(bool visible) noexcept;

    /// Legal patch name: starts with an ASCII letter, then only ASCII letters,
    /// digits and underscores. Empty or non ASCII names are invalid.
    [[nodiscard]] static bool isValidName(std::string_view name) noexcept;

private:
    ObjectId id_;
    std::string name_;
    PatchType type_ = PatchType::Patch;
    std::set<FaceId> faces_;
    std::array<Real, 3> color_{0.8, 0.8, 0.8};
    bool visible_ = true;
};

} // namespace ofgui::core
