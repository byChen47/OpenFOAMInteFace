#include "io/CaseDirectoryScan.h"

namespace ofgui::io {

std::string_view caseEntryKindName(CaseEntryKind kind) noexcept
{
    switch (kind)
    {
    case CaseEntryKind::Field:
        return "field";
    case CaseEntryKind::Dictionary:
        return "dictionary";
    case CaseEntryKind::MeshDirectory:
        return "mesh-dir";
    case CaseEntryKind::Script:
        return "script";
    case CaseEntryKind::Other:
        return "other";
    }
    return "other";
}

} // namespace ofgui::io
