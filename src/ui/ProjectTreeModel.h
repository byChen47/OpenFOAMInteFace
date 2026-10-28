#pragma once

#include "ui/ProjectNodeRef.h"

#include <QAbstractItemModel>
#include <QHash>

#include <vector>

namespace ofgui {

class CaseDocumentController;

/// Tree projection of one `core::CaseProject`.
///
/// Structure (docs/ARCHITECTURE.md §6.2):
/// @verbatim
/// 工程
/// ├── 计算域
/// ├── 网格
/// ├── 边界（六面）
/// │   ├── xMin → inlet (patch)
/// │   └── ...
/// ├── 场（0/）
/// │   ├── p [volScalarField]
/// │   │   ├── inlet — fixedValue
/// │   │   └── ...
/// │   └── U [volVectorField]
/// ├── 求解器设置
/// └── 导出预览
/// @endverbatim
///
/// A node stores `ProjectNodeRef` (kind + stable ids) and nothing else: labels
/// are resolved from the model on every `data()` call, so renaming a patch or a
/// field never changes the identity used by selection, the inspector or VTK.
class ProjectTreeModel final : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Role
    {
        NodeRefRole = Qt::UserRole + 1
    };

    explicit ProjectTreeModel(CaseDocumentController* document, QObject* parent = nullptr);
    ~ProjectTreeModel() override;

    [[nodiscard]] QModelIndex index(
        int row,
        int column,
        const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex& child) const override;
    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    /// Stable reference of `index`; invalid reference for a stale index.
    [[nodiscard]] ProjectNodeRef nodeRef(const QModelIndex& index) const;

    /// Index of `ref`, invalid when the node is not part of the current tree.
    [[nodiscard]] QModelIndex indexFor(const ProjectNodeRef& ref) const;

public slots:
    /// Rebuilds the whole tree from the document (structure changed).
    void rebuild();

private:
    struct Node
    {
        ProjectNodeRef ref;
        std::vector<Node> children;
    };

    [[nodiscard]] static Node section(NodeKind kind);

    CaseDocumentController* m_document = nullptr;
    Node m_root;
};

} // namespace ofgui
