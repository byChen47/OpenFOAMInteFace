#include "render/VtkViewportWidget.h"

#include "render/DomainSceneBuilder.h"

#include <QShowEvent>

#include <vtkRenderWindowInteractor.h>

namespace ofgui {

VtkViewportWidget::VtkViewportWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
    , m_renderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New())
    , m_renderer(vtkSmartPointer<vtkRenderer>::New())
{
    setObjectName(QStringLiteral("vtkViewport"));
    setMinimumSize(640, 480);
    setFocusPolicy(Qt::StrongFocus);

    setRenderWindow(m_renderWindow);

    m_renderer->SetBackground(0.075, 0.090, 0.115);
    m_renderer->SetBackground2(0.145, 0.170, 0.205);
    m_renderer->GradientBackgroundOn();
    m_renderWindow->AddRenderer(m_renderer);

    buildScene();
    m_orientationMarker.attach(interactor());
}

void VtkViewportWidget::buildScene()
{
    for (const auto& face : m_scene.faces)
    {
        if (face.actor != nullptr)
        {
            m_renderer->RemoveActor(face.actor);
        }
    }

    if (m_scene.outline != nullptr)
    {
        m_renderer->RemoveActor(m_scene.outline);
    }

    m_scene = DomainSceneBuilder{}.build(makeDefaultPreviewDomain());

    for (const auto& face : m_scene.faces)
    {
        m_renderer->AddActor(face.actor);
    }

    m_renderer->AddActor(m_scene.outline);
}

void VtkViewportWidget::fitView()
{
    m_cameraController.fit(m_renderer);
    m_renderWindow->Render();
}

void VtkViewportWidget::resetView()
{
    m_cameraController.reset(m_renderer);
    m_renderWindow->Render();
}

void VtkViewportWidget::showEvent(QShowEvent* event)
{
    QVTKOpenGLNativeWidget::showEvent(event);

    if (!m_initialViewApplied)
    {
        m_initialViewApplied = true;
        resetView();
    }
}

} // namespace ofgui