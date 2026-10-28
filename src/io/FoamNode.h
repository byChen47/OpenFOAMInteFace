#pragma once

#include "io/FoamSourceLocation.h"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {

/// Kind of an AST node produced by `FoamParser`.
///
/// The tree keeps the file's entry order and the exact source text of every
/// leaf, which is what docs/CASE_IMPORT_SPEC.md §6.1 requires for loss-free
/// read-modify-write export.
enum class FoamNodeKind
{
    Dictionary,   ///< `{ ... }` ordered entries, each child carries a `keyword`
    Sequence,     ///< one entry value = run of atoms (`uniform 0`, `Gauss linear`)
    List,         ///< `( ... )` list or tuple
    DimensionSet, ///< `[0 2 -2 0 0 0 0]`
    Word,
    String,
    Number,
    Variable,   ///< `$internalField`, `${p}`
    Expression, ///< `${{ ... }}`, never evaluated
    Verbatim,   ///< `#{ ... #}` content without the delimiters
    Directive,  ///< `#include` plus its argument children
};

/// Generic, ordered, owning AST node.
///
/// The class is deliberately a single concrete type instead of a class per node
/// kind: dictionary entries are `FoamNode`s whose `keyword` is set, and the
/// value of an entry is another `FoamNode`. That keeps traversals, cloning and
/// the emitter trivial while still preserving order and original text.
class FoamNode
{
public:
    FoamNode(FoamNodeKind kind, FoamSourceLocation location);
    ~FoamNode();

    FoamNode(const FoamNode&) = delete;
    FoamNode& operator=(const FoamNode&) = delete;
    FoamNode(FoamNode&&) noexcept = default;
    FoamNode& operator=(FoamNode&&) noexcept = default;

    FoamNodeKind kind() const noexcept;
    bool is(FoamNodeKind kind) const noexcept;

    const FoamSourceLocation& location() const noexcept;
    void setLocation(FoamSourceLocation location);

    /// Dictionary entry key; empty for value nodes that are not entry values.
    const std::string& keyword() const noexcept;
    /// `quoted` records that the key was written as a quoted string (`".*"`), so
    /// the emitter can restore the quotes while lookups stay unquoted.
    void setKeyword(std::string keyword, bool quoted = false);
    bool hasKeyword() const noexcept;
    bool keywordIsString() const noexcept;

    /// Exact leaf text: source bytes for words/numbers/variables, content without
    /// quotes for strings, content without `#{`/`#}` for verbatim blocks.
    const std::string& rawText() const noexcept;
    void setRawText(std::string rawText);

    const std::vector<std::unique_ptr<FoamNode>>& children() const noexcept;
    std::size_t childCount() const noexcept;
    const FoamNode* childAt(std::size_t index) const noexcept;
    FoamNode* childAt(std::size_t index) noexcept;
    FoamNode& addChild(std::unique_ptr<FoamNode> child);
    /// Detaches one child; returns nullptr when the index is out of range.
    std::unique_ptr<FoamNode> takeChild(std::size_t index);
    /// First direct child whose keyword equals `keyword` (literal compare).
    const FoamNode* findChild(std::string_view keyword) const noexcept;

    bool isDictionary() const noexcept;
    bool isSequence() const noexcept;
    bool isList() const noexcept;
    bool isDimensionSet() const noexcept;
    /// True for atom kinds that carry source text but no children semantics.
    bool isLeaf() const noexcept;

    std::string_view kindName() const noexcept;

private:
    FoamNodeKind kind_;
    FoamSourceLocation location_;
    std::string keyword_;
    bool keywordQuoted_{false};
    std::string rawText_;
    std::vector<std::unique_ptr<FoamNode>> children_;
};

std::string_view foamNodeKindName(FoamNodeKind kind);

} // namespace ofgui::io
