#include "app/ApplicationBootstrap.h"

#include <QSurfaceFormat>
#include <QVTKOpenGLNativeWidget.h>

namespace ofgui {

void configureOpenGLSurfaceFormat()
{
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
}

} // namespace ofgui