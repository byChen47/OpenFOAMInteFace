#pragma once

class vtkRenderer;

namespace ofgui {

class ViewportCameraController
{
public:
    void reset(vtkRenderer* renderer) const;
    void fit(vtkRenderer* renderer) const;
};

} // namespace ofgui