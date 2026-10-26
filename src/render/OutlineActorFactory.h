#pragma once

#include "render/PreviewDomain.h"

#include <vtkSmartPointer.h>

class vtkActor;

namespace ofgui {

class OutlineActorFactory
{
public:
    static vtkSmartPointer<vtkActor> create(const PreviewDomain& domain);
};

} // namespace ofgui