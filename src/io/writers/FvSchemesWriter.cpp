#include "io/writers/FvSchemesWriter.h"

#include "io/writers/FoamDictionaryEmitter.h"
#include "io/writers/FoamLayout.h"

#include <cstddef>
#include <string>

namespace ofgui::io {

std::string FvSchemesWriter::write(const FoamDictionary& source)
{
    const FoamNode& root = source.root();

    std::string body;
    bool first = true;
    for (std::size_t index = 0; index < root.childCount(); ++index)
    {
        const FoamNode* entry = root.childAt(index);
        if (entry == nullptr || entry->keyword() == "FoamFile")
        {
            continue;
        }
        if (!first)
        {
            body += '\n';
        }
        body += emitFoamEntry(*entry, 0);
        first = false;
    }

    std::string text = foamFileHeader("dictionary", "fvSchemes");
    text += body;
    text += "\n\n";
    text += foamTrailer();
    return text;
}

} // namespace ofgui::io
