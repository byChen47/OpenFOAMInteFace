#include "render/ViewportCameraController.h"

#include <vtkCamera.h>
#include <vtkRenderer.h>

namespace ofgui {

void ViewportCameraController::reset(vtkRenderer* renderer) const
{
    if (renderer == nullptr)
    {
        return;
    }

    fit(renderer);

    auto* camera = renderer->GetActiveCamera();
    if (camera != nullptr)
    {
        camera->Azimuth(35.0);
        camera->Elevation(22.0);
        camera->OrthogonalizeViewUp();
    }

    renderer->ResetCamera();
    renderer->ResetCameraClippingRange();
}

void ViewportCameraController::fit(vtkRenderer* renderer) const
{
    if (renderer == nullptr)
    {
        return;
    }

    renderer->ResetCamera();
    renderer->ResetCameraClippingRange();
}

} // namespace ofgui