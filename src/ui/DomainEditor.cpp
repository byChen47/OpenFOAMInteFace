#include "ui/DomainEditor.h"

#include "ui/CaseDocumentController.h"

#include "core/CaseProject.h"
#include "core/Vec3d.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QStringList>
#include <QVBoxLayout>

#include <cstddef>

namespace ofgui {
namespace {

QStringList axisLabels()
{
    return {QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("z")};
}

QString formatReal(double value)
{
    return QString::number(value, 'g', 10);
}

QDoubleSpinBox* makeCoordinateSpin(QWidget* parent, const QString& objectName)
{
    auto* spin = new QDoubleSpinBox(parent);
    spin->setObjectName(objectName);
    spin->setDecimals(6);
    spin->setRange(-1.0e9, 1.0e9);
    spin->setSingleStep(0.01);
    spin->setMinimumWidth(96);
    return spin;
}

QSpinBox* makeCellsSpin(QWidget* parent, const QString& objectName)
{
    auto* spin = new QSpinBox(parent);
    spin->setObjectName(objectName);
    spin->setRange(0, 1000000);
    spin->setMinimumWidth(96);
    return spin;
}

} // namespace

DomainEditor::DomainEditor(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("domainEditor"));

    m_mode = new QComboBox(this);
    m_mode->setObjectName(QStringLiteral("domainModeCombo"));
    m_mode->addItem(tr("三维 (3D)"), static_cast<int>(core::DomainMode::ThreeD));
    m_mode->addItem(tr("二维 (2D)"), static_cast<int>(core::DomainMode::TwoD));

    auto* modeRow = new QHBoxLayout;
    modeRow->setSpacing(8);
    modeRow->addWidget(new QLabel(tr("模式"), this));
    modeRow->addWidget(m_mode, 1);

    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(6);

    const QStringList axes = axisLabels();
    for (int axis = 0; axis < static_cast<int>(axes.size()); ++axis)
    {
        auto* header = new QLabel(axes.at(axis), this);
        header->setAlignment(Qt::AlignHCenter);
        grid->addWidget(header, 0, axis + 1);
    }

    const QStringList suffixes = {QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z")};
    grid->addWidget(new QLabel(tr("最小"), this), 1, 0);
    grid->addWidget(new QLabel(tr("最大"), this), 2, 0);
    grid->addWidget(new QLabel(tr("网格数"), this), 3, 0);

    for (int axis = 0; axis < static_cast<int>(axes.size()); ++axis)
    {
        const QString suffix = suffixes.at(axis);
        m_min[axis] = makeCoordinateSpin(this, QStringLiteral("domainMin") + suffix);
        m_max[axis] = makeCoordinateSpin(this, QStringLiteral("domainMax") + suffix);
        m_cells[axis] = makeCellsSpin(this, QStringLiteral("domainCells") + suffix);

        grid->addWidget(m_min[axis], 1, axis + 1);
        grid->addWidget(m_max[axis], 2, axis + 1);
        grid->addWidget(m_cells[axis], 3, axis + 1);

        connect(m_min[axis], &QDoubleSpinBox::valueChanged, this, &DomainEditor::onValueChanged);
        connect(m_max[axis], &QDoubleSpinBox::valueChanged, this, &DomainEditor::onValueChanged);
        connect(m_cells[axis], &QSpinBox::valueChanged, this, &DomainEditor::onValueChanged);
    }
    connect(m_mode, &QComboBox::currentIndexChanged, this, &DomainEditor::onModeChanged);

    m_sizeLabel = new QLabel(this);
    m_sizeLabel->setObjectName(QStringLiteral("domainSizeLabel"));
    m_sizeLabel->setWordWrap(true);

    m_volumeLabel = new QLabel(this);
    m_volumeLabel->setObjectName(QStringLiteral("domainVolumeLabel"));
    m_volumeLabel->setWordWrap(true);

    m_cellsLabel = new QLabel(this);
    m_cellsLabel->setObjectName(QStringLiteral("domainCellsTotalLabel"));
    m_cellsLabel->setWordWrap(true);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("domainErrorLabel"));
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setStyleSheet(QStringLiteral("color: #c0392b;"));

    m_hintLabel = new QLabel(this);
    m_hintLabel->setObjectName(QStringLiteral("domainHintLabel"));
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setStyleSheet(QStringLiteral("color: #7f8c8d;"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addLayout(modeRow);
    layout->addLayout(grid);
    layout->addWidget(m_sizeLabel);
    layout->addWidget(m_volumeLabel);
    layout->addWidget(m_cellsLabel);
    layout->addWidget(m_errorLabel);
    layout->addWidget(m_hintLabel);
    layout->addStretch(1);

    showMessages({}, {});
}

void DomainEditor::setDocument(CaseDocumentController* document)
{
    if (m_document != nullptr)
    {
        disconnect(m_document, nullptr, this, nullptr);
    }
    m_document = document;

    if (m_document != nullptr)
    {
        connect(m_document, &CaseDocumentController::projectChanged,
                this, &DomainEditor::refresh);
        connect(m_document, &CaseDocumentController::validationChanged,
                this, &DomainEditor::refresh);
        connect(m_document, &CaseDocumentController::documentStateChanged,
                this, &DomainEditor::refresh);
    }

    refresh();
}

QString DomainEditor::errorText() const
{
    return m_errorLabel->text();
}

QString DomainEditor::hintText() const
{
    return m_hintLabel->text();
}

void DomainEditor::refresh()
{
    if (m_document == nullptr)
    {
        return;
    }

    const core::CaseProject& project = m_document->project();
    const core::DomainBox& domain = project.domain();
    const core::MeshSettings& mesh = project.mesh();

    m_updating = true;
    const int modeIndex = m_mode->findData(static_cast<int>(domain.mode()));
    if (modeIndex >= 0)
    {
        m_mode->setCurrentIndex(modeIndex);
    }
    for (std::size_t axis = 0; axis < m_min.size(); ++axis)
    {
        m_min[axis]->setValue(domain.min().component(axis));
        m_max[axis]->setValue(domain.max().component(axis));
        m_cells[axis]->setValue(mesh.cellCount(axis));
    }
    m_cells[2]->setEnabled(domain.mode() == core::DomainMode::ThreeD);
    m_updating = false;

    const Candidate candidate = readCandidate();
    updateDerived(candidate.domain, candidate.mesh);
    showMessages(candidate.errors, candidate.hints);
}

void DomainEditor::onValueChanged()
{
    if (m_updating)
    {
        return;
    }
    applyCandidate();
}

void DomainEditor::onModeChanged()
{
    if (m_updating)
    {
        return;
    }

    const bool threeDimensional =
        m_mode->currentData().toInt() == static_cast<int>(core::DomainMode::ThreeD);
    m_cells[2]->setEnabled(threeDimensional);

    // 2D means exactly one cell layer in the thickness direction; force it
    // before validating so switching the mode never creates a blocking state.
    if (!threeDimensional && m_cells[2]->value() != 1)
    {
        m_updating = true;
        m_cells[2]->setValue(1);
        m_updating = false;
    }

    applyCandidate();
}

DomainEditor::Candidate DomainEditor::readCandidate() const
{
    Candidate candidate;

    const core::Vec3d minimum{m_min[0]->value(), m_min[1]->value(), m_min[2]->value()};
    const core::Vec3d maximum{m_max[0]->value(), m_max[1]->value(), m_max[2]->value()};
    candidate.domain.setMin(minimum);
    candidate.domain.setMax(maximum);
    candidate.domain.setMode(
        m_mode->currentData().toInt() == static_cast<int>(core::DomainMode::TwoD)
            ? core::DomainMode::TwoD
            : core::DomainMode::ThreeD);

    core::MeshSettings::CellCounts cells{};
    for (std::size_t axis = 0; axis < cells.size(); ++axis)
    {
        cells[axis] = m_cells[axis]->value();
    }
    if (m_document != nullptr)
    {
        candidate.mesh.setGrading(m_document->project().mesh().grading());
        candidate.mesh.setScale(m_document->project().mesh().scale());
    }
    candidate.mesh.setCells(cells);

    const QStringList axes = axisLabels();
    for (int axis = 0; axis < static_cast<int>(axes.size()); ++axis)
    {
        const double minimumValue = minimum.component(axis);
        const double maximumValue = maximum.component(axis);
        if (!(maximumValue > minimumValue))
        {
            candidate.errors.append(
                tr("%1 轴：最大值必须大于最小值（当前 min = %2，max = %3）。")
                    .arg(axes.at(axis), formatReal(minimumValue),
                         formatReal(maximumValue)));
        }
    }

    for (int axis = 0; axis < static_cast<int>(axes.size()); ++axis)
    {
        if (candidate.mesh.cellCount(axis) <= 0)
        {
            candidate.errors.append(
                tr("%1 方向网格数必须大于 0（当前为 %2）。")
                    .arg(axes.at(axis))
                    .arg(candidate.mesh.cellCount(axis)));
        }
    }

    if (candidate.domain.mode() == core::DomainMode::TwoD
        && candidate.mesh.cellCount(static_cast<std::size_t>(core::kTwoDThicknessAxis)) != 1)
    {
        candidate.errors.append(tr("二维模式下 z 方向网格数必须为 1。"));
    }

    if (candidate.domain.mode() == core::DomainMode::TwoD)
    {
        candidate.hints.append(
            tr("二维模式：z 方向固定为 1 层网格，zMin/zMax 所在 patch 的类型必须为 empty。"));
    }
    else if (candidate.mesh.cellCount(static_cast<std::size_t>(core::kTwoDThicknessAxis)) == 1)
    {
        candidate.hints.append(
            tr("三维模式下 z 方向只有 1 层网格；如果算例实为二维，请切换为二维模式。"));
    }

    if (candidate.mesh.exceedsCellBudget())
    {
        candidate.hints.append(tr("单元总数 %1 超过建议上限 %2，生成网格和求解会较慢。")
                                   .arg(static_cast<qlonglong>(candidate.mesh.totalCells()))
                                   .arg(static_cast<qlonglong>(core::MeshSettings::kCellBudgetWarning)));
    }

    return candidate;
}

void DomainEditor::applyCandidate()
{
    if (m_updating || m_document == nullptr)
    {
        return;
    }

    const Candidate candidate = readCandidate();
    updateDerived(candidate.domain, candidate.mesh);
    showMessages(candidate.errors, candidate.hints);

    if (!candidate.errors.isEmpty())
    {
        // Invalid input never reaches the model, so the tree, the validator and
        // the 3D scene keep their last valid state.
        return;
    }

    const core::CaseProject& current = m_document->project();
    const bool unchanged =
        current.domain().min().isApprox(candidate.domain.min(), 0.0)
        && current.domain().max().isApprox(candidate.domain.max(), 0.0)
        && current.domain().mode() == candidate.domain.mode()
        && current.mesh().cells() == candidate.mesh.cells();
    if (unchanged)
    {
        return;
    }

    m_document->project().setDomain(candidate.domain);
    m_document->project().setMesh(candidate.mesh);
    m_document->notifyProjectEdited();
}

void DomainEditor::updateDerived(const core::DomainBox& domain,
                                 const core::MeshSettings& mesh) const
{
    const core::Vec3d size = domain.size();
    m_sizeLabel->setText(tr("尺寸：%1 × %2 × %3 %4")
                             .arg(formatReal(size.x), formatReal(size.y), formatReal(size.z),
                                  QString::fromStdString(domain.unit())));
    m_volumeLabel->setText(tr("体积：%1 %2³")
                               .arg(formatReal(domain.volume()),
                                    QString::fromStdString(domain.unit())));
    m_cellsLabel->setText(
        tr("单元总数：%1").arg(static_cast<qlonglong>(mesh.totalCells())));
}

void DomainEditor::showMessages(const QStringList& errors, const QStringList& hints)
{
    m_errorLabel->setText(errors.join(QStringLiteral("；")));
    m_errorLabel->setVisible(!errors.isEmpty());
    m_hintLabel->setText(hints.join(QStringLiteral("；")));
    m_hintLabel->setVisible(!hints.isEmpty());
}

} // namespace ofgui
