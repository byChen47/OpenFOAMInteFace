#include "ui/ValidationPanel.h"

#include "ui/CaseDocumentController.h"
#include "ui/ValidationNavigator.h"

#include "core/ValidationResult.h"

#include <QBrush>
#include <QColor>
#include <QCoreApplication>
#include <QHeaderView>
#include <QLabel>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <cstddef>
#include <string>

namespace ofgui {
namespace {

QString severityLabel(core::Severity severity)
{
    switch (severity)
    {
    case core::Severity::Info:
        return QCoreApplication::translate("ValidationPanel", "提示");
    case core::Severity::Warning:
        return QCoreApplication::translate("ValidationPanel", "警告");
    case core::Severity::Error:
        return QCoreApplication::translate("ValidationPanel", "错误");
    case core::Severity::BlockingError:
        return QCoreApplication::translate("ValidationPanel", "阻断");
    }
    return QCoreApplication::translate("ValidationPanel", "提示");
}

QColor severityColor(core::Severity severity)
{
    switch (severity)
    {
    case core::Severity::Info:
        return QColor(0x5b, 0x6b, 0x7a);
    case core::Severity::Warning:
        return QColor(0xa0, 0x6a, 0x00);
    case core::Severity::Error:
        return QColor(0xc0, 0x39, 0x2b);
    case core::Severity::BlockingError:
        return QColor(0x8e, 0x1b, 0x1b);
    }
    return QColor(0x5b, 0x6b, 0x7a);
}

constexpr int kIssueRowRole = Qt::UserRole + 1;

} // namespace

ValidationPanel::ValidationPanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("validationPanel"));

    m_summary = new QLabel(this);
    m_summary->setObjectName(QStringLiteral("validationSummary"));
    m_summary->setWordWrap(true);

    m_tree = new QTreeWidget(this);
    m_tree->setObjectName(QStringLiteral("validationTree"));
    m_tree->setColumnCount(3);
    m_tree->setHeaderLabels(
        {tr("级别"), tr("代码"), tr("说明")});
    m_tree->setRootIsDecorated(false);
    m_tree->setUniformRowHeights(true);
    m_tree->setAllColumnsShowFocus(true);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_tree->setToolTip(tr("双击一行可定位到对应的工程树节点。"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(2);
    layout->addWidget(m_summary);
    layout->addWidget(m_tree, 1);

    connect(m_tree, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem* item, int) { activateItem(item); });

    refresh();
}

void ValidationPanel::setDocument(CaseDocumentController* document)
{
    if (m_document != nullptr)
    {
        disconnect(m_document, nullptr, this, nullptr);
    }
    m_document = document;
    if (m_document != nullptr)
    {
        connect(m_document, &CaseDocumentController::validationChanged,
                this, &ValidationPanel::refresh);
        connect(m_document, &CaseDocumentController::projectChanged,
                this, &ValidationPanel::refresh);
    }
    refresh();
}

void ValidationPanel::refresh()
{
    m_tree->clear();
    if (m_document == nullptr)
    {
        m_summary->setText(tr("校验：尚未打开工程"));
        return;
    }

    const core::ValidationResult& validation = m_document->validation();
    m_summary->setText(tr("校验：%1  ·  双击问题可定位到对应节点")
                           .arg(QString::fromStdString(validation.summary())));

    if (validation.empty())
    {
        auto* item = new QTreeWidgetItem(m_tree);
        item->setText(0, tr("提示"));
        item->setText(2, tr("没有校验问题，可以导出。"));
        item->setDisabled(true);
        return;
    }

    int index = 0;
    for (const core::ValidationIssue& issue : validation.issues())
    {
        auto* item = new QTreeWidgetItem(m_tree);
        item->setText(0, severityLabel(issue.severity()));
        item->setText(1, QString::fromStdString(issue.code()));
        item->setText(2, QString::fromStdString(issue.message()));
        item->setForeground(0, QBrush(severityColor(issue.severity())));
        item->setData(0, kIssueRowRole, index);
        item->setToolTip(2, QString::fromStdString(issue.message()));
        if (!issue.suggestedFix().empty())
        {
            item->setToolTip(2, item->toolTip(2) + QLatin1Char('\n')
                                    + tr("建议：%1").arg(QString::fromStdString(issue.suggestedFix())));
        }
        ++index;
    }
}

int ValidationPanel::issueCount() const
{
    if (m_document == nullptr)
    {
        return 0;
    }
    return static_cast<int>(m_document->validation().issues().size());
}

QTreeWidget* ValidationPanel::tree() const noexcept
{
    return m_tree;
}

ProjectNodeRef ValidationPanel::nodeRefForRow(int row) const
{
    if (m_document == nullptr || row < 0)
    {
        return {};
    }
    QTreeWidgetItem* item = m_tree->topLevelItem(row);
    if (item == nullptr || !item->data(0, kIssueRowRole).isValid())
    {
        return {};
    }
    const int issueIndex = item->data(0, kIssueRowRole).toInt();
    const auto& issues = m_document->validation().issues();
    if (issueIndex < 0 || static_cast<std::size_t>(issueIndex) >= issues.size())
    {
        return {};
    }
    return nodeRefForIssue(m_document->project(), issues[static_cast<std::size_t>(issueIndex)]);
}

bool ValidationPanel::activateRow(int row)
{
    QTreeWidgetItem* item = m_tree->topLevelItem(row);
    if (item == nullptr || !item->data(0, kIssueRowRole).isValid())
    {
        return false;
    }
    activateItem(item);
    return true;
}

void ValidationPanel::activateItem(QTreeWidgetItem* item)
{
    if (item == nullptr || !item->data(0, kIssueRowRole).isValid())
    {
        return;
    }
    const int row = m_tree->indexOfTopLevelItem(item);
    const ProjectNodeRef ref = nodeRefForRow(row);
    if (ref.isValid())
    {
        emit navigateRequested(ref);
    }
}

} // namespace ofgui
