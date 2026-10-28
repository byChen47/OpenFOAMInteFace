#pragma once

#include "core/FieldDefinition.h"
#include "core/Vec3d.h"

#include <QWidget>

#include <optional>

class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QStackedWidget;

namespace ofgui {

/// Edits one `uniform` value: a scalar for `volScalarField`, a vector for
/// `volVectorField`.
///
/// The editor never invents a representation: text it cannot parse (for example
/// `$internalField` or a `nonuniform` list) is shown read only so a later save
/// cannot silently rewrite the value. It is used for `internalField` and for
/// the `value` parameter of `fixedValue`-style boundary conditions.
class UniformValueEditor final : public QWidget
{
    Q_OBJECT

public:
    explicit UniformValueEditor(QWidget* parent = nullptr);

    void setFieldClass(core::FieldClass fieldClass);
    [[nodiscard]] core::FieldClass fieldClass() const noexcept;

    /// Accepts `uniform <scalar>`, `uniform (<x> <y> <z>)` (with optional
    /// trailing `;`); anything else becomes read-only raw text.
    void setRawText(const QString& text);

    /// Canonical text: `uniform 0`, `uniform (0 0 0)`, or the untouched raw text.
    [[nodiscard]] QString rawText() const;

    /// False when the text could not be parsed and is shown read only.
    [[nodiscard]] bool isEditable() const noexcept;

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] QString errorText() const;
    void clearError();

    [[nodiscard]] std::optional<core::Real> scalarValue() const;
    [[nodiscard]] std::optional<core::Vec3d> vectorValue() const;

    void setEditable(bool editable);

signals:
    /// Emitted when the user finished editing a spin box (focus lost or Enter).
    void editingFinished();

private:
    enum class Mode
    {
        Scalar,
        Vector,
        Raw
    };

    void applyParsed(Mode mode, double scalar, const core::Vec3d& vector);
    void showRaw(const QString& text);
    void notifyEdited();

    core::FieldClass m_fieldClass = core::FieldClass::VolScalarField;
    QStackedWidget* m_stack = nullptr;
    QDoubleSpinBox* m_scalar = nullptr;
    QDoubleSpinBox* m_vector[3] = {nullptr, nullptr, nullptr};
    QLineEdit* m_raw = nullptr;
    QLabel* m_errorLabel = nullptr;
    Mode m_mode = Mode::Scalar;
    bool m_editable = true;
    bool m_updating = false;
    QString m_error;
};

} // namespace ofgui
