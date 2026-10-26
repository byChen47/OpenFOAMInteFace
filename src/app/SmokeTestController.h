#pragma once

#include <QStringList>

class QApplication;
class QWidget;

namespace ofgui {

void scheduleSmokeTest(QApplication& application, QWidget& window, const QStringList& arguments);

} // namespace ofgui