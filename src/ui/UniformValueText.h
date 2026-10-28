#pragma once

#include "core/Basics.h"
#include "core/Vec3d.h"

#include <QString>
#include <QStringList>

#include <cmath>

namespace ofgui::ui {

/// Formats a real for the GUI. It is only a display form: writers re-format the
/// value, so no precision is lost by round-tripping through the model.
[[nodiscard]] inline QString formatReal(core::Real value)
{
    if (value == 0.0)
    {
        return QStringLiteral("0");
    }
    return QString::number(value, 'g', 12);
}

/// Removes surrounding whitespace and trailing `;` so imported entries can be
/// inspected without their statement terminator.
[[nodiscard]] inline QString stripStatementSemicolon(QString text)
{
    text = text.trimmed();
    while (text.endsWith(QLatin1Char(';')))
    {
        text.chop(1);
        text = text.trimmed();
    }
    return text;
}

/// Parses `uniform <scalar>`; returns false for vectors, `$refs` and lists.
[[nodiscard]] inline bool parseUniformScalar(const QString& text, core::Real* value)
{
    const QString cleaned = stripStatementSemicolon(text);
    if (!cleaned.startsWith(QStringLiteral("uniform")))
    {
        return false;
    }
    const QString number = cleaned.mid(7).trimmed();
    bool ok = false;
    const double parsed = number.toDouble(&ok);
    if (!ok || !std::isfinite(parsed))
    {
        return false;
    }
    *value = parsed;
    return true;
}

/// Parses `uniform (<x> <y> <z>)`, tolerating commas and extra spaces.
[[nodiscard]] inline bool parseUniformVector(const QString& text, core::Vec3d* value)
{
    QString cleaned = stripStatementSemicolon(text);
    if (!cleaned.startsWith(QStringLiteral("uniform")))
    {
        return false;
    }
    cleaned = cleaned.mid(7).trimmed();
    if (!cleaned.startsWith(QLatin1Char('(')) || !cleaned.endsWith(QLatin1Char(')')))
    {
        return false;
    }
    cleaned = cleaned.mid(1, cleaned.size() - 2).replace(QLatin1Char(','), QLatin1Char(' '));
    const QStringList parts = cleaned.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (parts.size() != 3)
    {
        return false;
    }
    core::Real components[3] = {0.0, 0.0, 0.0};
    for (int index = 0; index < 3; ++index)
    {
        bool ok = false;
        components[index] = parts.at(index).toDouble(&ok);
        if (!ok || !std::isfinite(components[index]))
        {
            return false;
        }
    }
    *value = core::Vec3d(components[0], components[1], components[2]);
    return true;
}

} // namespace ofgui::ui
