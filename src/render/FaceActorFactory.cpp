#include "render/FaceActorFactory.h"

#include "render/FaceAppearance.h"
#include "render/FacePlaneFactory.h"

#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

namespace ofgui {

vtkSmartPointer<vtkActor> FaceActorFactory::create(FaceId faceId, const PreviewDomain& domain)
{
    auto plane = FacePlaneFactory::create(faceId, domain);
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(plane->GetOutputPort());

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    const auto appearance = faceAppearance(faceId);
    auto* property = actor->GetProperty();
    property->SetColor(appearance.color[0], appearance.color[1], appearance.color[2]);
    property->SetAmbient(0.28);
    property->SetDiffuse(0.72);
    property->SetSpecular(0.10);
    property->SetSpecularPower(18.0);
    property->SetOpacity(0.92);

    return actor;
}

} // namespace ofgui