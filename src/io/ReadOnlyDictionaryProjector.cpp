#include "io/ReadOnlyDictionaryProjector.h"

#include "io/FoamDictionary.h"
#include "io/FoamNode.h"
#include "io/FoamValue.h"

#include <string>

namespace ofgui::io {
namespace {

void flattenInto(
    const FoamNode& node,
    const std::string& prefix,
    std::string_view relativePath,
    std::size_t maxEntries,
    std::vector<ImportedSetting>& settings)
{
    if (maxEntries != 0 && settings.size() >= maxEntries)
    {
        return;
    }
    const std::string base = prefix.empty() ? std::string(relativePath) : prefix;

    if (node.isDictionary())
    {
        for (const auto& child : node.children())
        {
            if (child == nullptr)
            {
                continue;
            }
            const std::string path =
                child->keyword().empty() ? base : base + "." + child->keyword();
            flattenInto(*child, path, relativePath, maxEntries, settings);
            if (maxEntries != 0 && settings.size() >= maxEntries)
            {
                return;
            }
        }
        return;
    }

    // The root is a dictionary, so `base` already names this entry.
    settings.push_back(ImportedSetting{base, FoamValue(node).text()});
}

} // namespace

std::vector<ImportedSetting> ReadOnlyDictionaryProjector::flatten(
    const FoamDictionary& dictionary, std::string_view relativePath, std::size_t maxEntries)
{
    std::vector<ImportedSetting> settings;
    if (dictionary.isEmpty())
    {
        return settings;
    }
    for (const auto& child : dictionary.root().children())
    {
        if (child == nullptr || child->keyword() == "FoamFile")
        {
            continue;
        }
        flattenInto(*child, std::string(relativePath) + "." + child->keyword(), relativePath, maxEntries, settings);
        if (maxEntries != 0 && settings.size() >= maxEntries)
        {
            break;
        }
    }
    return settings;
}

} // namespace ofgui::io
