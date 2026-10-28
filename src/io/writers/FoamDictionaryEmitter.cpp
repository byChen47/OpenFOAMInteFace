#include "io/writers/FoamDictionaryEmitter.h"

#include "io/writers/FoamLayout.h"

#include <cstddef>
#include <set>
#include <string_view>

namespace ofgui::io {
namespace {

std::string keyOf(const FoamNode& node)
{
    const std::string& keyword = node.keyword();
    if (node.keywordIsString())
    {
        return '"' + keyword + '"';
    }
    return keyword;
}

std::string emitEntryImpl(const FoamNode& entry, int level, const FoamEmitOptions& options, const FoamNode* scope);

std::string emitDictImpl(const FoamNode& dictionary, int level, const FoamEmitOptions& options, const FoamNode* scope);

std::string inlineValue(const FoamNode& node, int level, const FoamEmitOptions& options, const FoamNode* scope)
{
    if (node.isSequence())
    {
        std::string text;
        for (std::size_t index = 0; index < node.childCount(); ++index)
        {
            const FoamNode* item = node.childAt(index);
            if (item == nullptr)
            {
                continue;
            }
            if (!text.empty())
            {
                text += ' ';
            }
            text += inlineValue(*item, level, options, scope);
        }
        return text;
    }

    if (node.isList())
    {
        std::string text = "(\n";
        for (std::size_t index = 0; index < node.childCount(); ++index)
        {
            const FoamNode* item = node.childAt(index);
            if (item == nullptr)
            {
                continue;
            }
            if (item->isDictionary() && item->hasKeyword())
            {
                text += foamIndent(level + 1);
                text += keyOf(*item);
                text += '\n';
                text += foamIndent(level + 1);
                text += "{\n";
                text += emitDictImpl(*item, level + 2, options, &node);
                text += foamIndent(level + 1);
                text += "}\n";
                continue;
            }
            text += foamIndent(level + 1);
            text += inlineValue(*item, level + 1, options, scope);
            text += '\n';
        }
        text += foamIndent(level);
        text += ')';
        return text;
    }

    if (node.isDimensionSet())
    {
        std::string text = "[";
        for (std::size_t index = 0; index < node.childCount(); ++index)
        {
            const FoamNode* item = node.childAt(index);
            if (item == nullptr)
            {
                continue;
            }
            if (text.size() > 1)
            {
                text += ' ';
            }
            text += emitFoamLeafText(*item);
        }
        text += ']';
        return text;
    }

    return emitFoamLeafText(node);
}

const FoamNode* findNamedDictionary(const FoamNode* scope, std::string_view name)
{
    if (scope == nullptr)
    {
        return nullptr;
    }
    for (std::size_t index = 0; index < scope->childCount(); ++index)
    {
        const FoamNode* child = scope->childAt(index);
        if (child != nullptr && child->isDictionary() && child->keyword() == name)
        {
            return child;
        }
    }
    return nullptr;
}

std::set<std::string> ownKeywords(const FoamNode& dictionary)
{
    std::set<std::string> keywords;
    for (std::size_t index = 0; index < dictionary.childCount(); ++index)
    {
        const FoamNode* child = dictionary.childAt(index);
        if (child != nullptr && child->hasKeyword())
        {
            keywords.insert(child->keyword());
        }
    }
    return keywords;
}

std::string expandMerge(const FoamNode& dictionary, const FoamNode& variable, int level, const FoamNode* parentScope)
{
    const std::string& text = variable.rawText();
    const FoamNode* source = nullptr;
    if (!text.empty() && text.front() == '$')
    {
        const std::string_view name{text.data() + 1, text.size() - 1};
        source = findNamedDictionary(&dictionary, name);
        if (source == nullptr)
        {
            source = findNamedDictionary(parentScope, name);
        }
    }

    if (source == nullptr)
    {
        return foamIndent(level) + emitFoamLeafText(variable) + ";\n";
    }

    const std::set<std::string> overridden = ownKeywords(dictionary);
    std::string expanded;
    for (std::size_t index = 0; index < source->childCount(); ++index)
    {
        const FoamNode* entry = source->childAt(index);
        if (entry == nullptr || (entry->hasKeyword() && overridden.count(entry->keyword()) != 0))
        {
            continue;
        }
        expanded += emitEntryImpl(*entry, level, FoamEmitOptions{false, false}, parentScope);
    }
    return expanded;
}

std::string emitDictImpl(const FoamNode& dictionary, int level, const FoamEmitOptions& options, const FoamNode* scope)
{
    std::string text;
    bool first = true;
    for (std::size_t index = 0; index < dictionary.childCount(); ++index)
    {
        const FoamNode* entry = dictionary.childAt(index);
        if (entry == nullptr)
        {
            continue;
        }

        std::string rendered;
        if (options.expandVariableMerges && !entry->hasKeyword() && entry->kind() == FoamNodeKind::Variable)
        {
            // A merge replaces the entry in place and keeps the local entries
            // that follow it.
            rendered = expandMerge(dictionary, *entry, level, scope);
            if (rendered.empty())
            {
                continue;
            }
            if (!first && options.blankLineBetweenEntries)
            {
                text += '\n';
            }
            text += rendered;
            first = false;
            continue;
        }

        rendered = emitEntryImpl(*entry, level, options, &dictionary);
        if (rendered.empty())
        {
            continue;
        }

        if (!first && options.blankLineBetweenEntries)
        {
            text += '\n';
        }
        text += rendered;
        first = false;
    }
    return text;
}

std::string emitEntryImpl(const FoamNode& entry, int level, const FoamEmitOptions& options, const FoamNode* scope)
{
    if (entry.isDictionary())
    {
        std::string text;
        if (entry.hasKeyword())
        {
            text += foamIndent(level);
            text += keyOf(entry);
            text += '\n';
        }
        text += foamIndent(level);
        text += "{\n";
        text += emitDictImpl(entry, level + 1, options, scope);
        text += foamIndent(level);
        text += "}\n";
        return text;
    }

    if (!entry.hasKeyword())
    {
        return foamIndent(level) + emitFoamLeafText(entry) + ";\n";
    }

    if (entry.isList())
    {
        return foamIndent(level) + keyOf(entry) + '\n' + inlineValue(entry, level, options, scope) + ";\n";
    }

    return foamEntryLine(level, keyOf(entry), inlineValue(entry, level, options, scope)) + '\n';
}

} // namespace

std::string emitFoamLeafText(const FoamNode& node)
{
    switch (node.kind())
    {
    case FoamNodeKind::String:
        return '"' + node.rawText() + '"';
    case FoamNodeKind::Verbatim:
        return "#{" + node.rawText() + "#}";
    default:
        return node.rawText();
    }
}

std::string emitFoamEntry(const FoamNode& entry, int level, const FoamEmitOptions& options)
{
    return emitEntryImpl(entry, level, options, nullptr);
}

std::string emitFoamDictionary(const FoamNode& dictionary, int level, const FoamEmitOptions& options)
{
    return emitDictImpl(dictionary, level, options, nullptr);
}

} // namespace ofgui::io
