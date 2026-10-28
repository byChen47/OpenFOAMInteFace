#pragma once

#include "core/BoundaryPatch.h"
#include "core/Basics.h"
#include "core/FieldDefinition.h"
#include "core/FieldValue.h"
#include "core/ValidationResult.h"
#include "core/Vec3d.h"

#include "io/ImportedSetting.h"

#include <QString>

#include <string>
#include <vector>

namespace ofgui::detail {

/// HTML/text helpers shared by the node detail sections.
///
/// All of them live in one file because they only format values; they own no
/// state and are used by every `NodeDetailFormatter` section.

[[nodiscard]] QString esc(const QString& text);
[[nodiscard]] QString fromStd(const std::string& text);
[[nodiscard]] QString row(const QString& label, const QString& value);
[[nodiscard]] QString table(const QString& rows);
[[nodiscard]] QString section(const QString& title);
[[nodiscard]] QString paragraph(const QString& text);
[[nodiscard]] QString number(core::Real value);
[[nodiscard]] QString vecText(const core::Vec3d& value);
[[nodiscard]] QString dimensionsText(const core::FieldDefinition::Dimensions& dimensions);
[[nodiscard]] QString fieldValueText(const core::FieldValue& value);
[[nodiscard]] QString patchTypeText(const core::BoundaryPatch& patch);
[[nodiscard]] QString patchSummary(const core::BoundaryPatch& patch);
[[nodiscard]] QString severityText(core::Severity severity);

[[nodiscard]] QString issuesHtml(const core::ValidationResult& validation,
                                 const std::vector<core::ObjectId>& objectIds,
                                 const QString& targetFieldPrefix);

[[nodiscard]] QString readOnlySettingsHtml(const std::vector<io::ImportedSetting>& settings,
                                           const QString& prefix,
                                           int limit);

} // namespace ofgui::detail
