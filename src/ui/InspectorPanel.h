#pragma once

#include <QWidget>

class QLabel;

namespace ofgui {

class InspectorPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit InspectorPanel(QWidget* parent = nullptr);

public slots:
    void showNode(const QString& title, const QString& description);

private:
    QLabel* m_title = nullptr;
    QLabel* m_description = nullptr;
};

} // namespace ofgui