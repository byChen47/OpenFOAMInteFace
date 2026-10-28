#include "io/FoamNode.h"

#include <utility>

namespace ofgui::io {

FoamNode::FoamNode(FoamNodeKind kind, FoamSourceLocation location)
    : kind_(kind)
    , location_(std::move(location))
{
}

FoamNode::~FoamNode() = default;

FoamNodeKind FoamNode::kind() const noexcept
{
    return kind_;
}

bool FoamNode::is(FoamNodeKind kind) const noexcept
{
    return kind_ == kind;
}

const FoamSourceLocation& FoamNode::location() const noexcept
{
    return location_;
}

void FoamNode::setLocation(FoamSourceLocation location)
{
    location_ = std::move(location);
}

const std::string& FoamNode::keyword() const noexcept
{
    return keyword_;
}

void FoamNode::setKeyword(std::string keyword, bool quoted)
{
    keyword_ = std::move(keyword);
    keywordQuoted_ = quoted;
}

bool FoamNode::hasKeyword() const noexcept
{
    return !keyword_.empty();
}

bool FoamNode::keywordIsString() const noexcept
{
    return keywordQuoted_;
}

const std::string& FoamNode::rawText() const noexcept
{
    return rawText_;
}

void FoamNode::setRawText(std::string rawText)
{
    rawText_ = std::move(rawText);
}

const std::vector<std::unique_ptr<FoamNode>>& FoamNode::children() const noexcept
{
    return children_;
}

std::size_t FoamNode::childCount() const noexcept
{
    return children_.size();
}

const FoamNode* FoamNode::childAt(std::size_t index) const noexcept
{
    return index < children_.size() ? children_[index].get() : nullptr;
}

FoamNode* FoamNode::childAt(std::size_t index) noexcept
{
    return index < children_.size() ? children_[index].get() : nullptr;
}

FoamNode& FoamNode::addChild(std::unique_ptr<FoamNode> child)
{
    children_.push_back(std::move(child));
    return *children_.back();
}

std::unique_ptr<FoamNode> FoamNode::takeChild(std::size_t index)
{
    if (index >= children_.size())
    {
        return nullptr;
    }
    std::unique_ptr<FoamNode> child = std::move(children_[index]);
    children_.erase(children_.begin() + static_cast<std::ptrdiff_t>(index));
    return child;
}

const FoamNode* FoamNode::findChild(std::string_view keyword) const noexcept
{
    for (const auto& child : children_)
    {
        if (child->keyword() == keyword)
        {
            return child.get();
        }
    }
    return nullptr;
}

bool FoamNode::isDictionary() const noexcept
{
    return kind_ == FoamNodeKind::Dictionary;
}

bool FoamNode::isSequence() const noexcept
{
    return kind_ == FoamNodeKind::Sequence;
}

bool FoamNode::isList() const noexcept
{
    return kind_ == FoamNodeKind::List;
}

bool FoamNode::isDimensionSet() const noexcept
{
    return kind_ == FoamNodeKind::DimensionSet;
}

bool FoamNode::isLeaf() const noexcept
{
    switch (kind_)
    {
    case FoamNodeKind::Word:
    case FoamNodeKind::String:
    case FoamNodeKind::Number:
    case FoamNodeKind::Variable:
    case FoamNodeKind::Expression:
    case FoamNodeKind::Verbatim:
    case FoamNodeKind::Directive:
        return true;
    default:
        return false;
    }
}

std::string_view FoamNode::kindName() const noexcept
{
    return foamNodeKindName(kind_);
}

std::string_view foamNodeKindName(FoamNodeKind kind)
{
    switch (kind)
    {
    case FoamNodeKind::Dictionary:
        return "dictionary";
    case FoamNodeKind::Sequence:
        return "sequence";
    case FoamNodeKind::List:
        return "list";
    case FoamNodeKind::DimensionSet:
        return "dimensions";
    case FoamNodeKind::Word:
        return "word";
    case FoamNodeKind::String:
        return "string";
    case FoamNodeKind::Number:
        return "number";
    case FoamNodeKind::Variable:
        return "variable";
    case FoamNodeKind::Expression:
        return "expression";
    case FoamNodeKind::Verbatim:
        return "verbatim";
    case FoamNodeKind::Directive:
        return "directive";
    }
    return "unknown";
}

} // namespace ofgui::io
