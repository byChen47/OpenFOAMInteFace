#pragma once

#include "render/FaceId.h"
#include "render/PreviewDomain.h"

#include <vtkSmartPointer.h>
#include <vtkPlaneSource.h>

namespace ofgui {

class FacePlaneFactory
{
public:
    static vtkSmartPointer<vtkPlaneSource> create(FaceId faceId, const PreviewDomain& domain);
};

} // namespace ofgui