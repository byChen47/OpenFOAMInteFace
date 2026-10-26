#pragma once

#include "render/DomainSceneActors.h"
#include "render/PreviewDomain.h"

namespace ofgui {

class DomainSceneBuilder
{
public:
    DomainSceneActors build(const PreviewDomain& domain) const;
};

} // namespace ofgui