#include "render/OutlineActorFactory.h"

#include <vtkActor.h>
#include <vtkOutlineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

namespace ofgui {

vtkSmartPointer<vtkActor> OutlineActorFactory::create(const PreviewDomain& domain)
{
    const auto bounds = domain.bounds();

    auto outline = vtkSmartPointer<vtkOutlineSource>::New();
    outline->SetBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
    outline->Update();

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(outline->GetOutputPort());

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(0.92, 0.94, 0.97);
    actor->GetProperty()->SetLineWidth(1.5);

    return actor;
}

} // namespace ofgui