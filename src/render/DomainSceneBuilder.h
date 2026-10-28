#pragma once

#include "core/DomainBox.h"
#include "render/DomainSceneActors.h"
#include "render/PreviewDomain.h"

namespace ofgui {

class DomainSceneBuilder
{
public:
    DomainSceneActors build(const PreviewDomain& domain) const;

    /// Builds the six face actors from the document domain.
    ///
    /// Returns an empty scene (`faces` empty, `outline` null) when the box is
    /// not drawable; the viewport keeps the previous scene in that case.
    DomainSceneActors build(const core::DomainBox& box) const;
};

} // namespace ofgui
