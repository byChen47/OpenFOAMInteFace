#pragma once

#include <QTreeWidget>

namespace ofgui {

class ProjectTreeWidget final : public QTreeWidget
{
    Q_OBJECT

public:
    explicit ProjectTreeWidget(QWidget* parent = nullptr);

    void selectFirstNode();

signals:
    void nodeSelected(const QString& title, const QString& description);

private:
    void populateTree();
};

} // namespace ofgui