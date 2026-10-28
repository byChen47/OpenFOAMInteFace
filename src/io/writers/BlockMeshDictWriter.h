#pragma once

#include <core/CaseProject.h>

#include <string>

namespace ofgui::io {

/// Writes `system/blockMeshDict` for a single-block hexahedral project.
///
/// The domain is stored in metres while blockMeshDict keeps the model's
/// `scale` factor, so the vertices are emitted as `coordinate / scale`.
class BlockMeshDictWriter
{
public:
    static std::string write(const core::CaseProject& project);
};

} // namespace ofgui::io
