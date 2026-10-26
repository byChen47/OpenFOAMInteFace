#pragma once

#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkSmartPointer.h>

class vtkRenderWindowInteractor;

namespace ofgui {

class OrientationMarkerController
{
public:
    void attach(vtkRenderWindowInteractor* interactor);

private:
    vtkSmartPointer<vtkAxesActor> m_axes = vtkSmartPointer<vtkAxesActor>::New();
    vtkSmartPointer<vtkOrientationMarkerWidget> m_widget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
};

} // namespace ofgui