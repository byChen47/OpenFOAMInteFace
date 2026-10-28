#include "io/BlockMeshValueReader.h"

#include "io/FoamNode.h"
#include "io/FoamToken.h"

namespace ofgui::io {

bool blockMeshToReal(const FoamNode* node, double& value)
{
    return node != nullptr && parseFoamReal(node->rawText(), value);
}

bool blockMeshToInt(const FoamNode* node, int& value)
{
    long long parsed = 0;
    if (node == nullptr || !parseFoamInt(node->rawText(), parsed))
    {
        return false;
    }
    if (parsed < -2147483647LL - 1LL || parsed > 2147483647LL)
    {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

bool blockMeshRealList(const FoamNode* node, std::vector<double>& values)
{
    if (node == nullptr || !node->isList())
    {
        return false;
    }
    values.clear();
    for (const auto& child : node->children())
    {
        double value = 0.0;
        if (child == nullptr || !blockMeshToReal(child.get(), value))
        {
            return false;
        }
        values.push_back(value);
    }
    return true;
}

bool blockMeshIntList(const FoamNode* node, std::vector<int>& values)
{
    if (node == nullptr || !node->isList())
    {
        return false;
    }
    values.clear();
    for (const auto& child : node->children())
    {
        int value = 0;
        if (child == nullptr || !blockMeshToInt(child.get(), value))
        {
            return false;
        }
        values.push_back(value);
    }
    return true;
}

} // namespace ofgui::io
