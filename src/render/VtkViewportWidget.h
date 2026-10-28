#pragma once

#include "core/DomainBox.h"
#include "render/DomainSceneActors.h"
#include "render/OrientationMarkerController.h"
#include "render/PreviewDomain.h"
#include "render/ViewportCameraController.h"

#include <QVTKOpenGLNativeWidget.h>

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include <array>

class QShowEvent;

namespace ofgui {

/// VTK viewport of the main window.
///
/// The widget owns no business state: `setDocumentDomain()` converts the
/// document `DomainBox` into a render-side snapshot and rebuilds the six face
/// actors only when the bounds actually changed, so editing the cell counts
/// never touches the geometry (docs/ARCHITECTURE.md §5.4).
class VtkViewportWidget final : public QVTKOpenGLNativeWidget
{
    Q_OBJECT

public:
    explicit VtkViewportWidget(QWidget* parent = nullptr);
    ~VtkViewportWidget() override = default;

    /// Applies `domain`; invalid boxes reset the viewport to the default
    /// preview instead of drawing a degenerate cube.
    void setDocumentDomain(const core::DomainBox& domain);

    /// Drops the document domain and shows the default preview domain.
    void resetDocumentDomain();

    /// Six bounds of the currently displayed/desired domain, in the
    /// `PreviewDomain::bounds()` order (xmin, xmax, ymin, ymax, zmin, zmax).
    [[nodiscard]] std::array<double, 6> sceneBounds() const;

    /// Incremented every time the face actors are rebuilt; unchanged when only
    /// cell counts (or other non-geometric settings) changed.
    [[nodiscard]] int sceneRevision() const noexcept;

    /// True once a valid document domain has been applied.
    [[nodiscard]] bool hasDocumentDomain() const noexcept;

public slots:
    void fitView();
    void resetView();

protected:
    void showEvent(QShowEvent* event) override;

private:
    void rebuildScene(const PreviewDomain& domain);
    void renderIfVisible();

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    DomainSceneActors m_scene;
    PreviewDomain m_domain = makeDefaultPreviewDomain();
    ViewportCameraController m_cameraController;
    OrientationMarkerController m_orientationMarker;
    int m_sceneRevision = 0;
    bool m_hasDocumentDomain = false;
    bool m_initialViewApplied = false;
};

} // namespace ofgui
