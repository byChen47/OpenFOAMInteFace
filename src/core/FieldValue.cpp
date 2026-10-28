#include "core/FieldValue.h"

#include <utility>

namespace ofgui::core {

FieldValue FieldValue::uniformScalar(Real value)
{
    FieldValue fieldValue;
    fieldValue.kind_ = FieldValueKind::UniformScalar;
    fieldValue.assigned_ = true;
    fieldValue.scalar_ = value;
    return fieldValue;
}

FieldValue FieldValue::uniformVector(const Vec3d& value)
{
    FieldValue fieldValue;
    fieldValue.kind_ = FieldValueKind::UniformVector;
    fieldValue.assigned_ = true;
    fieldValue.vector_ = value;
    return fieldValue;
}

FieldValue FieldValue::nonUniformRaw(std::string rawText)
{
    FieldValue fieldValue;
    fieldValue.kind_ = FieldValueKind::NonUniformRaw;
    fieldValue.assigned_ = true;
    fieldValue.rawText_ = std::move(rawText);
    return fieldValue;
}

FieldValueKind FieldValue::kind() const noexcept
{
    return kind_;
}

bool FieldValue::isValid() const noexcept
{
    return assigned_;
}

bool FieldValue::isUniformScalar() const noexcept
{
    return assigned_ && kind_ == FieldValueKind::UniformScalar;
}

bool FieldValue::isUniformVector() const noexcept
{
    return assigned_ && kind_ == FieldValueKind::UniformVector;
}

bool FieldValue::isNonUniformRaw() const noexcept
{
    return assigned_ && kind_ == FieldValueKind::NonUniformRaw;
}

Real FieldValue::scalar() const
{
    if (!isUniformScalar())
    {
        throw std::logic_error("FieldValue::scalar() requires a uniform scalar value");
    }

    return scalar_;
}

const Vec3d& FieldValue::vector() const
{
    if (!isUniformVector())
    {
        throw std::logic_error("FieldValue::vector() requires a uniform vector value");
    }

    return vector_;
}

const std::string& FieldValue::rawText() const
{
    if (!isNonUniformRaw())
    {
        throw std::logic_error("FieldValue::rawText() requires a nonuniform raw value");
    }

    return rawText_;
}

} // namespace ofgui::core
