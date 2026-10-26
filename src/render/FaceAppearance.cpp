#include "render/FaceAppearance.h"

namespace ofgui {

FaceAppearance faceAppearance(FaceId faceId)
{
    switch (faceId)
    {
    case FaceId::XMin:
        return {"xMin", "X Min", {0.24, 0.49, 0.76}};
    case FaceId::XMax:
        return {"xMax", "X Max", {0.20, 0.66, 0.60}};
    case FaceId::YMin:
        return {"yMin", "Y Min", {0.91, 0.57, 0.22}};
    case FaceId::YMax:
        return {"yMax", "Y Max", {0.87, 0.74, 0.25}};
    case FaceId::ZMin:
        return {"zMin", "Z Min", {0.66, 0.45, 0.83}};
    case FaceId::ZMax:
        return {"zMax", "Z Max", {0.86, 0.39, 0.47}};
    }

    return {"unknown", "Unknown", {0.7, 0.7, 0.7}};
}

} // namespace ofgui