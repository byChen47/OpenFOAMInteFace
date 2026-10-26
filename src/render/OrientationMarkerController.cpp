#include "render/OrientationMarkerController.h"

#include <vtkRenderWindowInteractor.h>

namespace ofgui {

void OrientationMarkerController::attach(vtkRenderWindowInteractor* interactor)
{
    if (interactor == nullptr)
    {
        return;
    }

    m_axes->SetTotalLength(1.0, 1.0, 1.0);
    m_axes->SetNormalizedShaftLength(0.75, 0.75, 0.75);
    m_axes->SetNormalizedTipLength(0.25, 0.25, 0.25);

    m_widget->SetOrientationMarker(m_axes);
    m_widget->SetInteractor(interactor);
    m_widget->SetViewport(0.0, 0.0, 0.16, 0.16);
    m_widget->SetEnabled(1);
    m_widget->InteractiveOff();
}

} // namespace ofgui