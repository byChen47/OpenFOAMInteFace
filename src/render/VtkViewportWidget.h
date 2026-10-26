#pragma once

#include "render/DomainSceneActors.h"
#include "render/OrientationMarkerController.h"
#include "render/PreviewDomain.h"
#include "render/ViewportCameraController.h"

#include <QVTKOpenGLNativeWidget.h>

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

class QShowEvent;

namespace ofgui {

class VtkViewportWidget final : public QVTKOpenGLNativeWidget
{
    Q_OBJECT

public:
    explicit VtkViewportWidget(QWidget* parent = nullptr);
    ~VtkViewportWidget() override = default;

public slots:
    void fitView();
    void resetView();

protected:
    void showEvent(QShowEvent* event) override;

private:
    void buildScene();

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    DomainSceneActors m_scene;
    ViewportCameraController m_cameraController;
    OrientationMarkerController m_orientationMarker;
    bool m_initialViewApplied = false;
};

} // namespace ofgui