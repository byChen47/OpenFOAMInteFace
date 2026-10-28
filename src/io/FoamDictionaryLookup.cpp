#include "io/FoamDictionary.h"

#include "io/FoamNode.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {

namespace {

const FoamNode* findEntry(const FoamNode& container, std::string_view keyword)
{
    if (const FoamNode* direct = container.findChild(keyword))
    {
        return direct;
    }
    // `boundary ( movingWall { ... } ... )` keeps patch entries inside a list.
    for (const auto& child : container.children())
    {
        if (child->isList())
        {
            if (const FoamNode* nested = child->findChild(keyword))
            {
                return nested;
            }
        }
    }
    return nullptr;
}

const FoamNode* resolvePath(const FoamNode* node, std::string_view path)
{
    if (path.empty())
    {
        return node;
    }
    if (node == nullptr || (!node->isDictionary() && !node->isList()))
    {
        return nullptr;
    }
    // A quoted keyword may contain separators, so an exact match always wins first.
    if (const FoamNode* direct = findEntry(*node, path))
    {
        return direct;
    }
    if (path.front() == '.' || path.front() == '/')
    {
        return resolvePath(node, path.substr(1)); // tolerate empty path segments
    }
    for (std::size_t i = 0; i < path.size(); ++i)
    {
        if (path[i] != '.' && path[i] != '/')
        {
            continue;
        }
        const std::string_view head = path.substr(0, i);
        if (head.empty())
        {
            continue;
        }
        if (const FoamNode* child = findEntry(*node, head))
        {
            if (const FoamNode* resolved = resolvePath(child, path.substr(i + 1)))
            {
                return resolved;
            }
        }
    }
    return nullptr;
}

} // namespace

const FoamNode* FoamDictionary::find(std::string_view path) const
{
    if (root_ == nullptr)
    {
        return nullptr;
    }
    return resolvePath(root_.get(), path);
}

std::vector<const FoamNode*> FoamDictionary::findAll(
    std::string_view dictionaryPath, std::string_view keyword) const
{
    std::vector<const FoamNode*> matches;
    const FoamNode* dictionary = find(dictionaryPath);
    if (dictionary == nullptr || !dictionary->isDictionary())
    {
        return matches;
    }
    for (const auto& child : dictionary->children())
    {
        if (child->keyword() == keyword)
        {
            matches.push_back(child.get());
        }
    }
    return matches;
}


} // namespace ofgui::io
