#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace ofgui::core {

/// Boundary condition of one field on one patch.
///
/// `type` keeps the OpenFOAM word verbatim (`zeroGradient`, `fixedValue`, ...),
/// so imported conditions the GUI does not model yet survive a round trip.
/// `parameters` stores the remaining keyword/value pairs as raw text; typed
/// values (`value uniform (0 0 0)`) are split by the importer and written back
/// by the dictionary writer.
class BoundaryCondition
{
public:
    BoundaryCondition() = default;

    explicit BoundaryCondition(std::string type);

    [[nodiscard]] const std::string& type() const noexcept;
    void setType(std::string type);

    [[nodiscard]] const std::map<std::string, std::string, std::less<>>& parameters() const noexcept;

    void setParameter(std::string name, std::string value);

    /// @return true when the parameter existed and was removed.
    bool removeParameter(std::string_view name);

    [[nodiscard]] bool hasParameter(std::string_view name) const;

    [[nodiscard]] std::optional<std::string> parameter(std::string_view name) const;

    /// True when no `type` is set (an empty BC cannot be exported).
    [[nodiscard]] bool isEmpty() const noexcept;

    [[nodiscard]] bool isKnownType() const noexcept;

    /// Types the GUI can create and edit today. Imported cases may contain any
    /// other word; those stay read only and are reported by the importer.
    [[nodiscard]] static bool isKnownTypeName(std::string_view type) noexcept;

private:
    std::string type_;
    std::map<std::string, std::string, std::less<>> parameters_;
};

} // namespace ofgui::core
