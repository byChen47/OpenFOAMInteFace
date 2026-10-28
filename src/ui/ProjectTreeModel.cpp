#include "ui/ProjectTreeModel.h"

#include "ui/CaseDocumentController.h"
#include "ui/ProjectTreeLabels.h"

#include "core/FieldDefinition.h"

#include <QFont>

#include <functional>
#include <utility>

namespace ofgui {
namespace {

bool isSection(NodeKind kind) noexcept
{
    switch (kind)
    {
        case NodeKind::Project:
        case NodeKind::Domain:
        case NodeKind::Mesh:
        case NodeKind::Boundaries:
        case NodeKind::Fields:
        case NodeKind::Solver:
        case NodeKind::ExportPreview:
            return true;
        case NodeKind::BoundaryFace:
        case NodeKind::Field:
        case NodeKind::FieldBoundary:
            return false;
    }
    return false;
}

} // namespace

ProjectTreeModel::ProjectTreeModel(CaseDocumentController* document, QObject* parent)
    : QAbstractItemModel(parent)
    , m_document(document)
{
    m_root = Node{};

    if (m_document != nullptr)
    {
        connect(m_document, &CaseDocumentController::projectChanged,
                this, &ProjectTreeModel::rebuild);
        connect(m_document, &CaseDocumentController::documentStateChanged,
                this, &ProjectTreeModel::rebuild);
        connect(m_document, &CaseDocumentController::importReportChanged,
                this, &ProjectTreeModel::rebuild);
    }
    rebuild();
}

ProjectTreeModel::~ProjectTreeModel() = default;

ProjectTreeModel::Node ProjectTreeModel::section(NodeKind kind)
{
    Node node;
    node.ref = ProjectNodeRef::section(kind);
    return node;
}

void ProjectTreeModel::rebuild()
{
    const core::CaseProject& project = m_document->project();

    beginResetModel();

    m_root = Node{};
    Node projectNode = section(NodeKind::Project);
    projectNode.children.push_back(section(NodeKind::Domain));
    projectNode.children.push_back(section(NodeKind::Mesh));

    Node boundaries = section(NodeKind::Boundaries);
    for (const core::FaceId face : core::kAllFaceIds)
    {
        Node node;
        node.ref = ProjectNodeRef::forFace(face);
        boundaries.children.push_back(std::move(node));
    }
    projectNode.children.push_back(std::move(boundaries));

    Node fields = section(NodeKind::Fields);
    for (const core::FieldDefinition& field : project.fields())
    {
        Node fieldNode;
        fieldNode.ref = ProjectNodeRef::forField(field.id());
        for (const core::BoundaryPatch& patch : project.patches())
        {
            Node boundaryNode;
            boundaryNode.ref = ProjectNodeRef::forFieldBoundary(field.id(), patch.id());
            fieldNode.children.push_back(std::move(boundaryNode));
        }
        fields.children.push_back(std::move(fieldNode));
    }
    projectNode.children.push_back(std::move(fields));

    projectNode.children.push_back(section(NodeKind::Solver));
    projectNode.children.push_back(section(NodeKind::ExportPreview));
    m_root.children.push_back(std::move(projectNode));

    endResetModel();
}

QModelIndex ProjectTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (column != 0 || row < 0)
    {
        return {};
    }

    const Node* parentNode = parent.isValid()
        ? static_cast<const Node*>(parent.internalPointer())
        : &m_root;
    if (parentNode == nullptr || static_cast<std::size_t>(row) >= parentNode->children.size())
    {
        return {};
    }

    Node* child = const_cast<Node*>(&parentNode->children[static_cast<std::size_t>(row)]);
    return createIndex(row, column, child);
}

QModelIndex ProjectTreeModel::parent(const QModelIndex& child) const
{
    if (!child.isValid())
    {
        return {};
    }

    const Node* target = static_cast<const Node*>(child.internalPointer());
    if (target == nullptr)
    {
        return {};
    }

    std::function<QModelIndex(const Node&, const QModelIndex&)> findParent =
        [&](const Node& parentNode, const QModelIndex& parentIndex) -> QModelIndex
    {
        for (std::size_t row = 0; row < parentNode.children.size(); ++row)
        {
            const Node& candidate = parentNode.children[row];
            if (&candidate == target)
            {
                return parentIndex;
            }
            const QModelIndex candidateIndex =
                createIndex(static_cast<int>(row), 0, const_cast<Node*>(&candidate));
            const QModelIndex found = findParent(candidate, candidateIndex);
            if (found.isValid())
            {
                return found;
            }
        }
        return {};
    };

    return findParent(m_root, QModelIndex());
}

int ProjectTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
    {
        return 0;
    }
    const Node* node = parent.isValid()
        ? static_cast<const Node*>(parent.internalPointer())
        : &m_root;
    return node == nullptr ? 0 : static_cast<int>(node->children.size());
}

int ProjectTreeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant ProjectTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
    {
        return {};
    }

    const Node* node = static_cast<const Node*>(index.internalPointer());
    if (node == nullptr)
    {
        return {};
    }

    switch (role)
    {
        case Qt::DisplayRole:
            return treeNodeLabel(*m_document, node->ref);
        case Qt::ToolTipRole:
            return treeNodeToolTip(*m_document, node->ref);
        case Qt::FontRole:
        {
            if (!isSection(node->ref.kind))
            {
                return {};
            }
            QFont font;
            font.setBold(true);
            return font;
        }
        case NodeRefRole:
            return QVariant::fromValue(node->ref);
        default:
            return {};
    }
}

Qt::ItemFlags ProjectTreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> ProjectTreeModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles.insert(NodeRefRole, QByteArrayLiteral("nodeRef"));
    return roles;
}

ProjectNodeRef ProjectTreeModel::nodeRef(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return {};
    }
    const Node* node = static_cast<const Node*>(index.internalPointer());
    return node == nullptr ? ProjectNodeRef{} : node->ref;
}

QModelIndex ProjectTreeModel::indexFor(const ProjectNodeRef& ref) const
{
    std::function<QModelIndex(const Node&)> search =
        [&](const Node& parentNode) -> QModelIndex
    {
        for (std::size_t row = 0; row < parentNode.children.size(); ++row)
        {
            const Node& candidate = parentNode.children[row];
            if (candidate.ref == ref)
            {
                return createIndex(static_cast<int>(row), 0, const_cast<Node*>(&candidate));
            }
            const QModelIndex found = search(candidate);
            if (found.isValid())
            {
                return found;
            }
        }
        return {};
    };

    return search(m_root);
}

} // namespace ofgui
