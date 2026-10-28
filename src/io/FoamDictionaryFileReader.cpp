#include "io/FoamDictionary.h"

#include "io/FoamDiagnostic.h"
#include "io/FoamNode.h"
#include "io/FoamParseOptions.h"

#include <cstddef>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace ofgui::io {
namespace {

FoamDiagnostic makeFileDiagnostic(
    FoamSeverity severity, std::string code, std::string message, const std::string& label)
{
    FoamDiagnostic diagnostic;
    diagnostic.severity = severity;
    diagnostic.code = std::move(code);
    diagnostic.message = std::move(message);
    diagnostic.location = FoamSourceLocation{label, 0, 1, 1};
    return diagnostic;
}



} // namespace


FoamDictionary FoamDictionary::parseFile(
    const std::filesystem::path& path, const FoamParseOptions& options)
{
    const std::string label = path.generic_string();

    std::ifstream stream{path, std::ios::binary};
    if (!stream)
    {
        return FoamDictionary{
            std::string{},
            label,
            std::make_unique<FoamNode>(FoamNodeKind::Dictionary, FoamSourceLocation{label, 0, 1, 1}),
            {makeFileDiagnostic(
                FoamSeverity::Error, "IMPORT-IO-001", "cannot open dictionary file for reading", label)}};
    }

    std::string content{
        std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
    if (stream.bad())
    {
        return FoamDictionary{
            std::string{},
            label,
            std::make_unique<FoamNode>(FoamNodeKind::Dictionary, FoamSourceLocation{label, 0, 1, 1}),
            {makeFileDiagnostic(
                FoamSeverity::Error, "IMPORT-IO-001", "failed while reading dictionary file", label)}};
    }

    if (content.size() > options.maxFileBytes)
    {
        return FoamDictionary{
            std::move(content),
            label,
            std::make_unique<FoamNode>(FoamNodeKind::Dictionary, FoamSourceLocation{label, 0, 1, 1}),
            {makeFileDiagnostic(
                FoamSeverity::Error,
                "IMPORT-IO-001",
                "dictionary file exceeds the size limit",
                label)}};
    }

    if (content.empty())
    {
        return FoamDictionary{
            std::string{},
            label,
            std::make_unique<FoamNode>(FoamNodeKind::Dictionary, FoamSourceLocation{label, 0, 1, 1}),
            {makeFileDiagnostic(
                FoamSeverity::Warning, "IMPORT-IO-002", "dictionary file is empty", label)}};
    }

    const auto byteAt = [&content](std::size_t index) {
        return static_cast<unsigned char>(content[index]);
    };

    if (content.size() >= 2
        && ((byteAt(0) == 0xFF && byteAt(1) == 0xFE) || (byteAt(0) == 0xFE && byteAt(1) == 0xFF)))
    {
        return FoamDictionary{
            std::move(content),
            label,
            std::make_unique<FoamNode>(FoamNodeKind::Dictionary, FoamSourceLocation{label, 0, 1, 1}),
            {makeFileDiagnostic(
                FoamSeverity::Error,
                "IMPORT-ENC-001",
                "UTF-16 dictionaries are not supported",
                label)}};
    }

    const std::size_t probe = std::min<std::size_t>(content.size(), 1024);
    for (std::size_t i = 0; i < probe; ++i)
    {
        if (content[i] == '\0')
        {
            return FoamDictionary{
                std::move(content),
                label,
                std::make_unique<FoamNode>(
                    FoamNodeKind::Dictionary, FoamSourceLocation{label, 0, 1, 1}),
                {makeFileDiagnostic(
                    FoamSeverity::Error,
                    "IMPORT-ENC-001",
                    "binary content (NUL byte) in the first 1024 bytes",
                    label)}};
        }
    }

    std::vector<FoamDiagnostic> prefix;
    if (content.size() >= 3 && byteAt(0) == 0xEF && byteAt(1) == 0xBB && byteAt(2) == 0xBF)
    {
        content.erase(0, 3);
        prefix.push_back(makeFileDiagnostic(
            FoamSeverity::Info, "IMPORT-ENC-003", "UTF-8 byte order mark removed", label));
    }

    FoamDictionary document = FoamDictionary::parse(content, label, options);
    if (!prefix.empty())
    {
        prefix.insert(
            prefix.end(), document.diagnostics_.begin(), document.diagnostics_.end());
        document.diagnostics_ = std::move(prefix);
    }
    return document;
}


} // namespace ofgui::io
