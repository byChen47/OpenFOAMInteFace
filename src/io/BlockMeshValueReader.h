#pragma once

#include <vector>

namespace ofgui::io {

class FoamNode;

/// Typed readers for the atoms of `system/blockMeshDict`. They keep the
/// projector code free of token level details and stay deliberately small: only
/// the value shapes `blockMeshDict` uses (numbers, integer lists, point lists).
bool blockMeshToReal(const FoamNode* node, double& value);

bool blockMeshToInt(const FoamNode* node, int& value);

/// Reads `(a b c ...)` with numeric atoms.
bool blockMeshRealList(const FoamNode* node, std::vector<double>& values);

/// Reads `(a b c ...)` with integer atoms.
bool blockMeshIntList(const FoamNode* node, std::vector<int>& values);

} // namespace ofgui::io
