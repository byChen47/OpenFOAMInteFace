#include "io/FoamValue.h"

#include "io/FoamNode.h"
#include "io/FoamToken.h"
#include "io/FoamValueText.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace ofgui::io {
namespace {

bool numberValue(const FoamNode* node, double& out)
{
    if (!node || node->kind() != FoamNodeKind::Number)
    {
        return false;
    }
    return parseFoamReal(node->rawText(), out);
}

bool intValue(const FoamNode* node, long long& out)
{
    if (!node || node->kind() != FoamNodeKind::Number)
    {
        return false;
    }
    return parseFoamInt(node->rawText(), out);
}

bool vectorFromList(const FoamNode& list, std::array<double, 3>& out)
{
    if (!list.isList() || list.childCount() != 3)
    {
        return false;
    }
    std::array<double, 3> parsed{};
    for (std::size_t i = 0; i < 3; ++i)
    {
        if (!numberValue(list.childAt(i), parsed[i]))
        {
            return false;
        }
    }
    out = parsed;
    return true;
}

const FoamNode* firstChildOfKind(const FoamNode& node, FoamNodeKind kind)
{
    for (std::size_t i = 0; i < node.childCount(); ++i)
    {
        if (node.childAt(i)->is(kind))
        {
            return node.childAt(i);
        }
    }
    return nullptr;
}


} // namespace

bool FoamValue::asWord(std::string& out) const
{
    const FoamNode* data = dataNode();
    if (!data)
    {
        return false;
    }
    switch (data->kind())
    {
    case FoamNodeKind::Word:
    case FoamNodeKind::String:
    case FoamNodeKind::Number:
    case FoamNodeKind::Variable:
    case FoamNodeKind::Expression:
        out = data->rawText();
        return true;
    default:
        return false;
    }
}

bool FoamValue::asBool(bool& out) const
{
    const FoamNode* data = dataNode();
    if (!data)
    {
        return false;
    }
    if (data->kind() == FoamNodeKind::Word || data->kind() == FoamNodeKind::String)
    {
        const std::string& text = data->rawText();
        if (text == "true" || text == "on" || text == "yes" || text == "y")
        {
            out = true;
            return true;
        }
        if (text == "false" || text == "off" || text == "no" || text == "n")
        {
            out = false;
            return true;
        }
        return false;
    }
    if (data->kind() == FoamNodeKind::Number)
    {
        double value = 0.0;
        if (!numberValue(data, value))
        {
            return false;
        }
        out = value != 0.0;
        return true;
    }
    return false;
}

bool FoamValue::asInt(long long& out) const
{
    long long value = 0;
    if (!intValue(dataNode(), value))
    {
        return false;
    }
    out = value;
    return true;
}

bool FoamValue::asReal(double& out) const
{
    double value = 0.0;
    if (!numberValue(dataNode(), value))
    {
        return false;
    }
    out = value;
    return true;
}

bool FoamValue::asVector(std::array<double, 3>& out) const
{
    const FoamNode* data = dataNode();
    if (!data || !data->isList())
    {
        return false;
    }
    return vectorFromList(*data, out);
}

bool FoamValue::asDimensions(std::array<int, 7>& out) const
{
    const FoamNode* set = nullptr;
    if (!node_)
    {
        return false;
    }
    if (node_->isDimensionSet())
    {
        set = node_;
    }
    else if (node_->isSequence())
    {
        set = firstChildOfKind(*node_, FoamNodeKind::DimensionSet);
    }
    if (!set || set->childCount() != 7)
    {
        return false;
    }

    std::array<int, 7> parsed{};
    for (std::size_t i = 0; i < 7; ++i)
    {
        long long exponent = 0;
        if (!intValue(set->childAt(i), exponent))
        {
            return false;
        }
        parsed[i] = static_cast<int>(exponent);
    }
    out = parsed;
    return true;
}

bool FoamValue::asRealList(std::vector<double>& out) const
{
    if (!node_)
    {
        return false;
    }

    const FoamNode* list = nullptr;
    if (node_->isList())
    {
        list = node_;
    }
    else if (node_->isSequence())
    {
        list = firstChildOfKind(*node_, FoamNodeKind::List);
    }
    if (list)
    {
        std::vector<double> parsed;
        parsed.reserve(list->childCount());
        for (std::size_t i = 0; i < list->childCount(); ++i)
        {
            double value = 0.0;
            if (!numberValue(list->childAt(i), value))
            {
                return false;
            }
            parsed.push_back(value);
        }
        out = std::move(parsed);
        return true;
    }

    double single = 0.0;
    if (!numberValue(dataNode(), single))
    {
        return false;
    }
    out.assign(1, single);
    return true;
}


} // namespace ofgui::io
