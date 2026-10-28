#pragma once

#include "core/DomainBox.h"
#include "render/PreviewDomain.h"

#include <optional>

namespace ofgui {

/// Adapts the document `core::DomainBox` to the render-side snapshot.
///
/// The adapter keeps `render` a pure projection of the document (docs/
/// ARCHITECTURE.md §3.4): `PreviewDomain` stays the only thing VTK sees, and an
/// invalid box (non finite or non positive extent) yields no snapshot at all so
/// callers keep the last valid scene instead of building degenerate actors.
class DomainBoundsAdapter
{
public:
    /// @return the six bounds, or `std::nullopt` when the box cannot be drawn.
    [[nodiscard]] static std::optional<PreviewDomain> previewDomain(const core::DomainBox& box);

    /// True when both snapshots address the same six bounds within
    /// `core::kRealTolerance` (relative to the larger magnitude).
    [[nodiscard]] static bool sameBounds(const PreviewDomain& left, const PreviewDomain& right);
};

} // namespace ofgui
