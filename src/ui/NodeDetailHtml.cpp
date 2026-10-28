#include "ui/NodeDetailHtml.h"

#include "core/ValidationIssue.h"

#include <QCoreApplication>
#include <QStringList>

#include <algorithm>
#include <cstddef>

namespace ofgui::detail {
namespace {

QString tr(const char* text)
{
    return QCoreApplication::translate("NodeDetailFormatter", text);
}

} // namespace

QString esc(const QString& text)
{
    return text.toHtmlEscaped();
}

QString fromStd(const std::string& text)
{
    return QString::fromStdString(text);
}

QString row(const QString& label, const QString& value)
{
    return QStringLiteral("<tr><td width=\"36%\" valign=\"top\"><b>%1</b></td>"
                          "<td valign=\"top\">%2</td></tr>")
        .arg(esc(label), value);
}

QString table(const QString& rows)
{
    return QStringLiteral("<table width=\"100%\" cellspacing=\"0\" cellpadding=\"3\">%1</table>")
        .arg(rows);
}

QString section(const QString& title)
{
    return QStringLiteral("<p style=\"margin-top:12px\"><b>%1</b></p>").arg(esc(title));
}

QString paragraph(const QString& text)
{
    return QStringLiteral("<p>%1</p>").arg(text);
}

QString number(core::Real value)
{
    return QString::number(value, 'g', 6);
}

QString vecText(const core::Vec3d& value)
{
    return QStringLiteral("(%1, %2, %3)").arg(number(value.x), number(value.y), number(value.z));
}

QString dimensionsText(const core::FieldDefinition::Dimensions& dimensions)
{
    QStringList parts;
    for (const int exponent : dimensions)
    {
        parts.append(QString::number(exponent));
    }
    return QStringLiteral("[%1]").arg(parts.join(QLatin1Char(' ')));
}

QString fieldValueText(const core::FieldValue& value)
{
    if (!value.isValid())
    {
        return tr("未设置");
    }
    switch (value.kind())
    {
        case core::FieldValueKind::UniformScalar:
            return tr("uniform %1").arg(number(value.scalar()));
        case core::FieldValueKind::UniformVector:
            return tr("uniform %1").arg(vecText(value.vector()));
        case core::FieldValueKind::NonUniformRaw:
        {
            const QString raw = fromStd(value.rawText()).simplified();
            const QString shortened = raw.size() > 120 ? raw.left(117) + QStringLiteral("...") : raw;
            return tr("原文（只读）：%1").arg(esc(shortened));
        }
    }
    return {};
}

QString patchTypeText(const core::BoundaryPatch& patch)
{
    return fromStd(std::string(core::patchTypeName(patch.type())));
}

QString patchSummary(const core::BoundaryPatch& patch)
{
    QStringList faces;
    for (const core::FaceId face : patch.faces())
    {
        faces.append(QString::fromLatin1(core::faceName(face)));
    }
    const QString faceText = faces.isEmpty() ? tr("无面") : faces.join(QStringLiteral(", "));
    return tr("%1 · 面：%2").arg(patchTypeText(patch), faceText);
}

QString severityText(core::Severity severity)
{
    switch (severity)
    {
        case core::Severity::Info:
            return tr("提示");
        case core::Severity::Warning:
            return tr("警告");
        case core::Severity::Error:
            return tr("错误");
        case core::Severity::BlockingError:
            return tr("阻断");
    }
    return {};
}

QString issuesHtml(const core::ValidationResult& validation,
                   const std::vector<core::ObjectId>& objectIds,
                   const QString& targetFieldPrefix)
{
    QStringList items;
    for (const core::ValidationIssue& issue : validation.issues())
    {
        bool matches = false;
        if (issue.targetObjectId().has_value())
        {
            matches = std::find(objectIds.begin(), objectIds.end(), *issue.targetObjectId())
                      != objectIds.end();
        }
        if (!matches && !targetFieldPrefix.isEmpty())
        {
            matches = fromStd(issue.targetField()).startsWith(targetFieldPrefix);
        }
        if (!matches)
        {
            continue;
        }

        QString text = tr("[%1] %2：%3")
                           .arg(severityText(issue.severity()),
                                esc(fromStd(issue.code())),
                                esc(fromStd(issue.message())));
        if (!issue.suggestedFix().empty())
        {
            text += tr("（建议：%1）").arg(esc(fromStd(issue.suggestedFix())));
        }
        items.append(QStringLiteral("<li>%1</li>").arg(text));
    }

    if (items.isEmpty())
    {
        return QString();
    }
    return section(tr("校验问题")) + QStringLiteral("<ul>%1</ul>").arg(items.join(QString()));
}

QString readOnlySettingsHtml(const std::vector<io::ImportedSetting>& settings,
                             const QString& prefix,
                             int limit)
{
    QStringList rows;
    int shown = 0;
    for (const io::ImportedSetting& setting : settings)
    {
        const QString path = fromStd(setting.path);
        if (!path.startsWith(prefix))
        {
            continue;
        }
        if (shown >= limit)
        {
            break;
        }
        rows.append(row(path, QStringLiteral("<code>%1</code>").arg(esc(fromStd(setting.text)))));
        ++shown;
    }

    if (rows.isEmpty())
    {
        return QString();
    }

    QString html = section(tr("只读字典设置")) + table(rows.join(QString()));
    const auto total = std::count_if(
        settings.begin(), settings.end(), [&prefix](const io::ImportedSetting& setting)
        {
            return fromStd(setting.path).startsWith(prefix);
        });
    if (total > limit)
    {
        html += paragraph(tr("仅显示前 %1 项，共 %2 项。")
                              .arg(QString::number(limit), QString::number(total)));
    }
    return html;
}

} // namespace ofgui::detail
