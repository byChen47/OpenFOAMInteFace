#include "ui/ApplicationStyle.h"

namespace ofgui {

QString applicationStyleSheet()
{
    return QStringLiteral(R"(
        QMainWindow, QWidget {
            background-color: #f3f5f7;
            color: #25313d;
            font-family: "Microsoft YaHei UI", "Segoe UI";
            font-size: 10pt;
        }

        QMenuBar {
            background-color: #ffffff;
            border-bottom: 1px solid #d8dee5;
            padding: 3px 8px;
        }

        QMenuBar::item {
            padding: 6px 10px;
            border-radius: 4px;
        }

        QMenuBar::item:selected {
            background-color: #e8eef5;
        }

        QToolBar {
            background-color: #ffffff;
            border-bottom: 1px solid #d8dee5;
            spacing: 6px;
            padding: 5px 8px;
        }

        QToolButton {
            padding: 6px 12px;
            border: 1px solid transparent;
            border-radius: 5px;
        }

        QToolButton:hover {
            background-color: #e8eef5;
            border-color: #c8d4e1;
        }

        QStatusBar {
            background-color: #ffffff;
            border-top: 1px solid #d8dee5;
            color: #586675;
        }

        QSplitter::handle {
            background-color: #d8dee5;
        }

        QTreeWidget {
            background-color: #ffffff;
            border: 1px solid #d8dee5;
            border-radius: 6px;
            outline: 0;
            padding: 6px;
        }

        QTreeWidget::item {
            min-height: 28px;
            padding: 2px 4px;
            border-radius: 4px;
        }

        QTreeWidget::item:hover {
            background-color: #eef3f8;
        }

        QTreeWidget::item:selected {
            background-color: #dce9f7;
            color: #1c4d78;
        }

        QLabel#panelTitle {
            color: #1f3448;
            font-size: 12pt;
            font-weight: 600;
        }

        QLabel#inspectorTitle {
            color: #1f3448;
            font-size: 12pt;
            font-weight: 600;
            padding-top: 2px;
        }

        QLabel#inspectorDescription {
            color: #586675;
            line-height: 1.5;
        }

        QLabel#inspectorHint {
            background-color: #eef3f8;
            border: 1px solid #d7e2ec;
            border-radius: 5px;
            color: #667789;
            padding: 9px;
        }
    )");
}

} // namespace ofgui