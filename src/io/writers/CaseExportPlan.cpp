#include "io/writers/CaseExportPlan.h"

#include <algorithm>

namespace ofgui::io {

std::size_t CaseExportPlan::totalBytes() const noexcept
{
    std::size_t bytes = 0;
    for (const ExportFile& file : files)
    {
        bytes += file.byteSize != 0 ? static_cast<std::size_t>(file.byteSize) : file.text.size();
    }
    return bytes;
}

std::size_t CaseExportPlan::count(ExportAction action) const noexcept
{
    return static_cast<std::size_t>(std::count_if(
        files.begin(),
        files.end(),
        [action](const ExportFile& file) { return file.action == action; }));
}

const ExportFile* CaseExportPlan::find(std::string_view relativePath) const noexcept
{
    const auto it = std::find_if(files.begin(), files.end(), [relativePath](const ExportFile& file)
                                 { return file.relativePath == relativePath; });
    return it == files.end() ? nullptr : &*it;
}

} // namespace ofgui::io
