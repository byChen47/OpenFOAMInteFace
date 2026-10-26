#pragma once

#include "render/FaceId.h"
#include "render/PreviewDomain.h"

#include <vtkSmartPointer.h>

class vtkActor;

namespace ofgui {

class FaceActorFactory
{
public:
    static vtkSmartPointer<vtkActor> create(FaceId faceId, const PreviewDomain& domain);
};

} // namespace ofgui