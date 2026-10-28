#include "io/FoamValue.h"

#include "io/FoamToken.h"
#include "io/FoamValueText.h"

#include <string>
#include <string_view>
#include <utility>

namespace ofgui::io {

FoamValue::FoamValue(const FoamNode& node)
    : node_(&node)
{
}

FoamValue::FoamValue(const FoamNode* node)
    : node_(node)
{
}

bool FoamValue::isValid() const noexcept
{
    return node_ != nullptr;
}

const FoamNode* FoamValue::node() const noexcept
{
    return node_;
}

const FoamNode& FoamValue::operator*() const
{
    return *node_;
}

FoamNodeKind FoamValue::kind() const noexcept
{
    return node_ ? node_->kind() : FoamNodeKind::Sequence;
}

bool FoamValue::isDictionary() const noexcept
{
    return node_ != nullptr && node_->isDictionary();
}

bool FoamValue::isSequence() const noexcept
{
    return node_ != nullptr && node_->isSequence();
}

bool FoamValue::isList() const noexcept
{
    return node_ != nullptr && node_->isList();
}

bool FoamValue::isDimensionSet() const noexcept
{
    return node_ != nullptr && node_->isDimensionSet();
}

int FoamValue::itemCount() const noexcept
{
    if (!node_)
    {
        return 0;
    }
    if (node_->isSequence() || node_->isList())
    {
        return static_cast<int>(node_->childCount());
    }
    return 0;
}

const FoamNode* FoamValue::item(int index) const noexcept
{
    if (!node_ || index < 0)
    {
        return nullptr;
    }
    return node_->childAt(static_cast<std::size_t>(index));
}

std::string FoamValue::text() const
{
    if (!node_)
    {
        return {};
    }
    return foamValueLeafText(*node_);
}

const FoamNode* FoamValue::dataNode() const noexcept
{
    if (!node_)
    {
        return nullptr;
    }
    if (!node_->isSequence())
    {
        return node_->isDictionary() ? nullptr : node_;
    }
    if (node_->childCount() == 0)
    {
        return nullptr;
    }

    std::size_t index = 0;
    const FoamNode* first = node_->childAt(0);
    if (first->kind() == FoamNodeKind::Word
        && (first->rawText() == "uniform" || first->rawText() == "nonuniform"))
    {
        index = 1;
        while (index < node_->childCount())
        {
            const FoamNode* candidate = node_->childAt(index);
            if (candidate->kind() == FoamNodeKind::Word
                && candidate->rawText().rfind("List<", 0) == 0)
            {
                ++index;
                continue;
            }
            return candidate;
        }
        return nullptr;
    }
    return first;
}

bool FoamValue::isUniform() const noexcept
{
    if (!node_ || !node_->isSequence() || node_->childCount() == 0)
    {
        return false;
    }
    const FoamNode* first = node_->childAt(0);
    return first->kind() == FoamNodeKind::Word && first->rawText() == "uniform";
}

bool FoamValue::isNonuniform() const noexcept
{
    if (!node_ || !node_->isSequence() || node_->childCount() == 0)
    {
        return false;
    }
    const FoamNode* first = node_->childAt(0);
    return first->kind() == FoamNodeKind::Word && first->rawText() == "nonuniform";
}

std::vector<std::string> FoamValue::words() const
{
    std::vector<std::string> out;
    if (!node_)
    {
        return out;
    }
    if (!node_->isSequence())
    {
        if (node_->kind() == FoamNodeKind::Word || node_->kind() == FoamNodeKind::String)
        {
            out.push_back(node_->rawText());
        }
        return out;
    }
    for (std::size_t i = 0; i < node_->childCount(); ++i)
    {
        const FoamNode* child = node_->childAt(i);
        if (child->kind() == FoamNodeKind::Word || child->kind() == FoamNodeKind::String)
        {
            out.push_back(child->rawText());
        }
    }
    return out;
}

} // namespace ofgui::io
