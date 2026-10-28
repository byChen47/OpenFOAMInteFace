#include "render/VtkViewportWidget.h"

#include "render/DomainBoundsAdapter.h"
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

    rebuildScene(m_domain);
    m_orientationMarker.attach(interactor());
}

void VtkViewportWidget::rebuildScene(const PreviewDomain& domain)
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

    m_scene = DomainSceneBuilder{}.build(domain);

    for (const auto& face : m_scene.faces)
    {
        m_renderer->AddActor(face.actor);
    }

    m_renderer->AddActor(m_scene.outline);
}

void VtkViewportWidget::setDocumentDomain(const core::DomainBox& domain)
{
    const std::optional<PreviewDomain> preview = DomainBoundsAdapter::previewDomain(domain);
    if (!preview.has_value())
    {
        resetDocumentDomain();
        return;
    }

    if (DomainBoundsAdapter::sameBounds(m_domain, *preview))
    {
        // Geometry did not change (e.g. only the cell counts were edited):
        // keep the actors and the camera exactly as they are.
        m_hasDocumentDomain = true;
        return;
    }

    m_domain = *preview;
    m_hasDocumentDomain = true;
    rebuildScene(m_domain);
    ++m_sceneRevision;
    renderIfVisible();
}

void VtkViewportWidget::resetDocumentDomain()
{
    const PreviewDomain preview = makeDefaultPreviewDomain();
    m_hasDocumentDomain = false;
    if (DomainBoundsAdapter::sameBounds(m_domain, preview))
    {
        return;
    }

    m_domain = preview;
    rebuildScene(m_domain);
    ++m_sceneRevision;
    renderIfVisible();
}

std::array<double, 6> VtkViewportWidget::sceneBounds() const
{
    return m_domain.bounds();
}

int VtkViewportWidget::sceneRevision() const noexcept
{
    return m_sceneRevision;
}

bool VtkViewportWidget::hasDocumentDomain() const noexcept
{
    return m_hasDocumentDomain;
}

void VtkViewportWidget::renderIfVisible()
{
    if (isVisible())
    {
        m_renderWindow->Render();
    }
}

void VtkViewportWidget::fitView()
{
    m_cameraController.fit(m_renderer);
    renderIfVisible();
}

void VtkViewportWidget::resetView()
{
    m_cameraController.reset(m_renderer);
    renderIfVisible();
}

void VtkViewportWidget::showEvent(QShowEvent* event)
{
    QVTKOpenGLNativeWidget::showEvent(event);

    if (!m_initialViewApplied)
    {
        m_initialViewApplied = true;
        if (m_hasDocumentDomain)
        {
            fitView();
        }
        else
        {
            resetView();
        }
    }
}

} // namespace ofgui
