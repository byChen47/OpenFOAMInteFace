#include "io/FoamDictionary.h"

#include "io/FoamNode.h"
#include "io/FoamParseOptions.h"
#include "io/FoamParser.h"
#include "io/FoamSourceLocation.h"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ofgui::io {
namespace {

constexpr std::size_t kIndentWidth = 4;
constexpr std::size_t kMaxInlineListItems = 6;
constexpr std::size_t kMaxInlineListWidth = 78;

std::string indentOf(int level)
{
    return std::string(static_cast<std::size_t>(level) * kIndentWidth, ' ');
}

std::string quotedString(const std::string& content)
{
    std::string out;
    out.reserve(content.size() + 2);
    out += '"';
    for (const char c : content)
    {
        if (c == '"')
        {
            out += '\\';
        }
        out += c;
    }
    out += '"';
    return out;
}

std::string leafText(const FoamNode& node)
{
    switch (node.kind())
    {
    case FoamNodeKind::String:
        return quotedString(node.rawText());
    case FoamNodeKind::Verbatim:
        return "#{" + node.rawText() + "#}";
    default:
        return node.rawText();
    }
}

std::string joinChildren(const FoamNode& node, char open, char close)
{
    std::string out;
    out += open;
    for (std::size_t i = 0; i < node.childCount(); ++i)
    {
        if (i != 0)
        {
            out += ' ';
        }
        out += leafText(*node.childAt(i));
    }
    out += close;
    return out;
}

std::string renderValue(const FoamNode& node, int level);
std::string renderEntry(const FoamNode& node, int level);

std::string renderInline(const FoamNode& node)
{
    switch (node.kind())
    {
    case FoamNodeKind::Sequence:
    {
        std::string out;
        for (std::size_t i = 0; i < node.childCount(); ++i)
        {
            if (i != 0)
            {
                out += ' ';
            }
            out += renderInline(*node.childAt(i));
        }
        return out;
    }
    case FoamNodeKind::List:
        return joinChildren(node, '(', ')');
    case FoamNodeKind::DimensionSet:
        return joinChildren(node, '[', ']');
    case FoamNodeKind::Dictionary:
        return renderValue(node, 0);
    default:
        return leafText(node);
    }
}

std::string renderValue(const FoamNode& node, int level)
{
    if (node.isList())
    {
        bool inlineable = node.childCount() <= kMaxInlineListItems;
        for (std::size_t i = 0; inlineable && i < node.childCount(); ++i)
        {
            inlineable = node.childAt(i)->isLeaf();
        }
        const std::string inlineText = inlineable ? joinChildren(node, '(', ')') : std::string{};
        if (inlineable && inlineText.size() <= kMaxInlineListWidth)
        {
            return inlineText;
        }

        std::string out;
        out += "(\n";
        for (std::size_t i = 0; i < node.childCount(); ++i)
        {
            const FoamNode& child = *node.childAt(i);
            // A list may hold keyworded sub-dictionaries, e.g. blockMeshDict patches.
            if (child.isDictionary() && child.hasKeyword())
            {
                out += renderEntry(child, level + 1);
                continue;
            }
            out += indentOf(level + 1);
            out += renderValue(child, level + 1);
            out += '\n';
        }
        out += indentOf(level);
        out += ')';
        return out;
    }

    if (node.isDimensionSet())
    {
        return joinChildren(node, '[', ']');
    }

    if (node.isDictionary())
    {
        std::string out;
        out += "{\n";
        for (const auto& child : node.children())
        {
            out += renderEntry(*child, level + 1);
        }
        out += indentOf(level);
        out += '}';
        return out;
    }

    return renderInline(node);
}

std::string renderEntry(const FoamNode& node, int level)
{
    std::string out;
    if (node.is(FoamNodeKind::Directive))
    {
        out += indentOf(level);
        out += node.rawText();
        for (std::size_t i = 0; i < node.childCount(); ++i)
        {
            out += ' ';
            out += renderInline(*node.childAt(i));
        }
        out += '\n';
        return out;
    }

    const std::string key = node.keywordIsString() ? quotedString(node.keyword()) : node.keyword();
    if (!node.hasKeyword())
    {
        out += indentOf(level);
        out += renderValue(node, level);
        out += ";\n";
        return out;
    }

    if (node.isDictionary())
    {
        out += indentOf(level);
        out += key;
        out += '\n';
        out += indentOf(level);
        out += renderValue(node, level);
        out += '\n';
        return out;
    }

    out += indentOf(level);
    out += key;
    out += "    ";
    out += renderValue(node, level);
    out += ";\n";
    return out;
}

void visitEntries(
    const FoamNode& dictionary,
    const std::string& prefix,
    const std::function<void(const FoamNode&, std::string_view)>& visitor)
{
    for (const auto& child : dictionary.children())
    {
        const std::string path = prefix.empty() ? child->keyword() : prefix + "." + child->keyword();
        visitor(*child, path);
        if (child->isDictionary())
        {
            visitEntries(*child, path, visitor);
        }
    }
}

/// Resolves a path against a dictionary tree.
///
/// Path segments are separated by '.' or '/'. A quoted keyword may itself contain
/// separators (`".*"`, `"(free|hole)"`), so the longest exact keyword always wins
/// before the path is split, and a split is only taken when it resolves.
/// Finds a keyworded entry in a dictionary or inside a list of dictionaries.
} // namespace

FoamDictionary::FoamDictionary()
    : root_(std::make_unique<FoamNode>(FoamNodeKind::Dictionary, FoamSourceLocation{}))
{
}

FoamDictionary::~FoamDictionary() = default;

FoamDictionary::FoamDictionary(FoamDictionary&&) noexcept = default;

FoamDictionary& FoamDictionary::operator=(FoamDictionary&&) noexcept = default;

FoamDictionary::FoamDictionary(
    std::string sourceText,
    std::string fileLabel,
    std::unique_ptr<FoamNode> root,
    std::vector<FoamDiagnostic> diagnostics)
    : sourceText_(std::move(sourceText))
    , fileLabel_(std::move(fileLabel))
    , root_(std::move(root))
    , diagnostics_(std::move(diagnostics))
{
}

FoamDictionary FoamDictionary::parse(
    std::string_view source, std::string fileLabel, const FoamParseOptions& options)
{
    FoamParser parser{source, std::move(fileLabel), options};
    return parser.parse();
}

bool FoamDictionary::ok() const
{
    return root_ != nullptr && !foamHasError(diagnostics_);
}

bool FoamDictionary::isEmpty() const
{
    return root_ == nullptr || root_->childCount() == 0;
}

const std::string& FoamDictionary::fileLabel() const noexcept
{
    return fileLabel_;
}

const std::string& FoamDictionary::sourceText() const noexcept
{
    return sourceText_;
}

const FoamNode& FoamDictionary::root() const noexcept
{
    return *root_;
}

const FoamNode* FoamDictionary::header() const noexcept
{
    if (root_ == nullptr || root_->childCount() == 0)
    {
        return nullptr;
    }
    const FoamNode* first = root_->childAt(0);
    return (first != nullptr && first->isDictionary() && first->keyword() == "FoamFile") ? first
                                                                                         : nullptr;
}

bool FoamDictionary::hasFoamFileHeader() const noexcept
{
    return header() != nullptr;
}

const std::vector<FoamDiagnostic>& FoamDictionary::diagnostics() const noexcept
{
    return diagnostics_;
}

std::string FoamDictionary::diagnosticsText() const
{
    std::string out;
    for (const FoamDiagnostic& diagnostic : diagnostics_)
    {
        out += diagnostic.toString();
        out += '\n';
    }
    return out;
}

void FoamDictionary::visit(
    const std::function<void(const FoamNode& node, std::string_view path)>& visitor) const
{
    if (root_ == nullptr || !visitor)
    {
        return;
    }
    visitEntries(*root_, std::string{}, visitor);
}

std::string FoamDictionary::toText() const
{
    std::string out;
    if (root_ == nullptr)
    {
        return out;
    }
    for (const auto& child : root_->children())
    {
        out += renderEntry(*child, 0);
    }
    return out;
}

} // namespace ofgui::io
