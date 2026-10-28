#pragma once

#include "io/FoamNode.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace ofgui::io {

/// Read-only, typed view over a value `FoamNode`.
///
/// A value node is what a dictionary entry points at: usually a `Sequence`
/// (`uniform 0`, `Gauss linear`, `wall`), occasionally a `List`, a
/// `DimensionSet` or a nested `Dictionary`. `FoamValue` adds the conversions the
/// import projection needs (`asReal`, `asVector`, `asDimensions`, ...) without
/// touching the tree, so a bad value never forces a re-parse.
class FoamValue
{
public:
    FoamValue() = default;
    explicit FoamValue(const FoamNode& node);
    explicit FoamValue(const FoamNode* node);

    bool isValid() const noexcept;
    const FoamNode* node() const noexcept;
    const FoamNode& operator*() const;

    FoamNodeKind kind() const noexcept;
    bool isDictionary() const noexcept;
    bool isSequence() const noexcept;
    bool isList() const noexcept;
    bool isDimensionSet() const noexcept;

    /// Number of atoms (sequence) or items (list); 0 for scalars and dictionaries.
    int itemCount() const noexcept;
    const FoamNode* item(int index) const noexcept;

    /// Single-line canonical text of the value (strings quoted, verbatim wrapped).
    std::string text() const;

    /// First atom that is not the `uniform` / `nonuniform` / `List<...>` prefix.
    const FoamNode* dataNode() const noexcept;

    bool isUniform() const noexcept;
    bool isNonuniform() const noexcept;

    /// Plain word value (`type` -> `zeroGradient`), unquoted for `String` atoms.
    bool asWord(std::string& out) const;
    bool asBool(bool& out) const;
    bool asInt(long long& out) const;
    bool asReal(double& out) const;
    /// Accepts `(0 0 0)` and `uniform (0 0 0)`.
    bool asVector(std::array<double, 3>& out) const;
    /// Seven dimension exponents from `[0 2 -2 0 0 0 0]`.
    bool asDimensions(std::array<int, 7>& out) const;
    /// Flat numeric list (`nonuniform List<scalar>` payload or a plain list).
    bool asRealList(std::vector<double>& out) const;
    /// All word/string atoms decoded, in order (`Gauss linear` -> {"Gauss","linear"}).
    std::vector<std::string> words() const;

private:
    const FoamNode* node_{nullptr};
};

} // namespace ofgui::io
