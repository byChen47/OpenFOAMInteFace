#include "render/FacePlaneFactory.h"

namespace ofgui {

vtkSmartPointer<vtkPlaneSource> FacePlaneFactory::create(FaceId faceId, const PreviewDomain& domain)
{
    auto plane = vtkSmartPointer<vtkPlaneSource>::New();
    plane->SetXResolution(1);
    plane->SetYResolution(1);

    switch (faceId)
    {
    case FaceId::XMin:
        plane->SetOrigin(domain.minX, domain.minY, domain.minZ);
        plane->SetPoint1(domain.minX, domain.minY, domain.maxZ);
        plane->SetPoint2(domain.minX, domain.maxY, domain.minZ);
        break;
    case FaceId::XMax:
        plane->SetOrigin(domain.maxX, domain.minY, domain.minZ);
        plane->SetPoint1(domain.maxX, domain.maxY, domain.minZ);
        plane->SetPoint2(domain.maxX, domain.minY, domain.maxZ);
        break;
    case FaceId::YMin:
        plane->SetOrigin(domain.minX, domain.minY, domain.minZ);
        plane->SetPoint1(domain.maxX, domain.minY, domain.minZ);
        plane->SetPoint2(domain.minX, domain.minY, domain.maxZ);
        break;
    case FaceId::YMax:
        plane->SetOrigin(domain.minX, domain.maxY, domain.minZ);
        plane->SetPoint1(domain.minX, domain.maxY, domain.maxZ);
        plane->SetPoint2(domain.maxX, domain.maxY, domain.minZ);
        break;
    case FaceId::ZMin:
        plane->SetOrigin(domain.minX, domain.minY, domain.minZ);
        plane->SetPoint1(domain.minX, domain.maxY, domain.minZ);
        plane->SetPoint2(domain.maxX, domain.minY, domain.minZ);
        break;
    case FaceId::ZMax:
        plane->SetOrigin(domain.minX, domain.minY, domain.maxZ);
        plane->SetPoint1(domain.maxX, domain.minY, domain.maxZ);
        plane->SetPoint2(domain.minX, domain.maxY, domain.maxZ);
        break;
    }

    plane->Update();
    return plane;
}

} // namespace ofgui