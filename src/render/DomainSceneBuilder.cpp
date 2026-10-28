#include "render/DomainSceneBuilder.h"

#include "render/DomainBoundsAdapter.h"
#include "render/FaceActorFactory.h"
#include "render/OutlineActorFactory.h"

namespace ofgui {

DomainSceneActors DomainSceneBuilder::build(const PreviewDomain& domain) const
{
    DomainSceneActors scene;
    scene.faces.reserve(kAllFaceIds.size());

    for (const FaceId faceId : kAllFaceIds)
    {
        scene.faces.push_back({faceId, FaceActorFactory::create(faceId, domain)});
    }

    scene.outline = OutlineActorFactory::create(domain);
    return scene;
}

DomainSceneActors DomainSceneBuilder::build(const core::DomainBox& box) const
{
    const std::optional<PreviewDomain> domain = DomainBoundsAdapter::previewDomain(box);
    if (!domain.has_value())
    {
        return {};
    }
    return build(*domain);
}

} // namespace ofgui
