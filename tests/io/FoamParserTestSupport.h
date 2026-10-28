#pragma once

#include "io/FoamDiagnostic.h"
#include "io/FoamDictionary.h"
#include "io/FoamNode.h"
#include "io/FoamParseOptions.h"
#include "io/FoamValue.h"

#include <QtCore/QString>

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io::foamtest {

inline QString fixturePath(const char* relative)
{
    return QStringLiteral(OFGUI_FOAM_FIXTURE_DIR) + QLatin1Char('/') + QString::fromUtf8(relative);
}

inline FoamDictionary parseFixture(const char* relative, FoamParseOptions options = FoamParseOptions{})
{
    return FoamDictionary::parseFile(
        std::filesystem::path{fixturePath(relative).toStdWString()}, options);
}

inline FoamDictionary parseText(std::string_view source, FoamParseOptions options = FoamParseOptions{})
{
    return FoamDictionary::parse(source, "inline", options);
}

inline const FoamDiagnostic* firstWithCode(
    const std::vector<FoamDiagnostic>& diagnostics, std::string_view code)
{
    for (const FoamDiagnostic& diagnostic : diagnostics)
    {
        if (diagnostic.code == code)
        {
            return &diagnostic;
        }
    }
    return nullptr;
}

inline bool hasCode(const std::vector<FoamDiagnostic>& diagnostics, std::string_view code)
{
    return firstWithCode(diagnostics, code) != nullptr;
}

inline std::string valueText(const FoamDictionary& dictionary, std::string_view path)
{
    const FoamNode* node = dictionary.find(path);
    if (node == nullptr)
    {
        return "<missing>";
    }
    return FoamValue{*node}.text();
}

inline bool structurallyEqual(const FoamNode& lhs, const FoamNode& rhs)
{
    if (lhs.kind() != rhs.kind() || lhs.keyword() != rhs.keyword()
        || lhs.keywordIsString() != rhs.keywordIsString() || lhs.rawText() != rhs.rawText()
        || lhs.childCount() != rhs.childCount())
    {
        return false;
    }
    for (std::size_t i = 0; i < lhs.childCount(); ++i)
    {
        if (!structurallyEqual(*lhs.childAt(i), *rhs.childAt(i)))
        {
            return false;
        }
    }
    return true;
}

inline constexpr const char* kCavityDictionaries[] = {
    "cavity/system/controlDict",
    "cavity/system/fvSchemes",
    "cavity/system/fvSolution",
    "cavity/system/blockMeshDict",
    "cavity/constant/transportProperties",
    "cavity/0/p",
    "cavity/0/U",
};

} // namespace ofgui::io::foamtest
