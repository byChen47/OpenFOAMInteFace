#include "io/writers/ExportManifest.h"

#include "io/writers/ExportManifestSchema.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLatin1String>
#include <QString>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace ofgui::io {
namespace {

QString toQString(std::string_view text)
{
    return QString::fromUtf8(text.data(), static_cast<int>(text.size()));
}

/// Byte count of one exported file: the source size for copies, the encoded
/// size for generated dictionaries.
std::size_t bytesOf(const ExportFile& file) noexcept
{
    return file.byteSize != 0 ? static_cast<std::size_t>(file.byteSize) : file.text.size();
}

QJsonObject countsObject(const CaseExportPlan& plan)
{
    QJsonObject counts;
    counts.insert(QString{"written"}, QJsonValue{static_cast<double>(plan.count(ExportAction::Written))});
    counts.insert(QString{"copied"}, QJsonValue{static_cast<double>(plan.count(ExportAction::Copied))});
    counts.insert(QString{"mirrored"}, QJsonValue{static_cast<double>(plan.count(ExportAction::Mirrored))});
    counts.insert(QString{"skipped"}, QJsonValue{static_cast<double>(plan.skipped.size())});
    return counts;
}

} // namespace

std::string ExportManifest::build(const CaseExportPlan& plan, std::string_view generatedAt)
{
    std::vector<const ExportFile*> ordered;
    ordered.reserve(plan.files.size());
    for (const ExportFile& file : plan.files)
    {
        ordered.push_back(&file);
    }
    std::sort(ordered.begin(), ordered.end(), [](const ExportFile* left, const ExportFile* right)
              { return left->relativePath < right->relativePath; });

    QJsonArray files;
    for (const ExportFile* file : ordered)
    {
        QJsonObject entry;
        entry.insert(QLatin1String{exportmanifest::kRelativePathKey}, toQString(file->relativePath));
        entry.insert(QLatin1String{exportmanifest::kActionKey}, toQString(exportActionName(file->action)));
        entry.insert(QLatin1String{exportmanifest::kBytesKey}, QJsonValue{static_cast<double>(bytesOf(*file))});
        files.append(entry);
    }

    std::vector<const ExportSkippedFile*> skippedEntries;
    skippedEntries.reserve(plan.skipped.size());
    for (const ExportSkippedFile& entry : plan.skipped)
    {
        skippedEntries.push_back(&entry);
    }
    std::sort(skippedEntries.begin(), skippedEntries.end(), [](const ExportSkippedFile* left, const ExportSkippedFile* right)
              { return left->relativePath < right->relativePath; });

    QJsonArray skipped;
    for (const ExportSkippedFile* entry : skippedEntries)
    {
        QJsonObject object;
        object.insert(QLatin1String{exportmanifest::kRelativePathKey}, toQString(entry->relativePath));
        object.insert(QLatin1String{exportmanifest::kReasonKey}, toQString(entry->reason));
        skipped.append(object);
    }

    QJsonObject root;
    root.insert(QLatin1String{exportmanifest::kSchemaVersionKey}, QJsonValue{exportmanifest::kSchemaVersion});
    root.insert(QLatin1String{exportmanifest::kGeneratedAtKey}, toQString(generatedAt));
    root.insert(QLatin1String{exportmanifest::kCountsKey}, countsObject(plan));
    root.insert(QLatin1String{exportmanifest::kFilesKey}, files);
    root.insert(QLatin1String{exportmanifest::kSkippedKey}, skipped);

    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

} // namespace ofgui::io
