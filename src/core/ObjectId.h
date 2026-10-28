#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace ofgui::core {

/// Stable identifier for every addressable model object (patch, field, ...).
///
/// The id is independent from the display name: renaming a patch keeps its id,
/// so tree nodes, VTK actors and persisted JSON stay attached to the same
/// object. A default constructed `ObjectId` is invalid (`value() == 0`).
class ObjectId
{
public:
    ObjectId() noexcept = default;

    explicit constexpr ObjectId(std::uint64_t value) noexcept
        : value_(value)
    {
    }

    /// Process unique id. Never returns an invalid id.
    static ObjectId create();

    /// Parses the canonical text form produced by `toString()` (lower case hex,
    /// optional `0x`/`0X` prefix, at most 16 digits). Returns `std::nullopt`
    /// for malformed text and for the invalid id `0`.
    static std::optional<ObjectId> fromString(std::string_view text);

    [[nodiscard]] bool isValid() const noexcept
    {
        return value_ != 0;
    }

    [[nodiscard]] constexpr std::uint64_t value() const noexcept
    {
        return value_;
    }

    /// Canonical text form: exactly 16 lower case hex digits, zero padded.
    [[nodiscard]] std::string toString() const;

    friend constexpr bool operator==(ObjectId lhs, ObjectId rhs) noexcept
    {
        return lhs.value_ == rhs.value_;
    }

    friend constexpr auto operator<=>(ObjectId lhs, ObjectId rhs) noexcept
    {
        return lhs.value_ <=> rhs.value_;
    }

private:
    std::uint64_t value_ = 0;
};

} // namespace ofgui::core
