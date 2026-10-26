#pragma once

#include "render/FaceId.h"

#include <vtkActor.h>
#include <vtkSmartPointer.h>

#include <vector>

namespace ofgui {

struct FaceSceneActor
{
    FaceId faceId;
    vtkSmartPointer<vtkActor> actor;
};

struct DomainSceneActors
{
    std::vector<FaceSceneActor> faces;
    vtkSmartPointer<vtkActor> outline;
};

} // namespace ofgui