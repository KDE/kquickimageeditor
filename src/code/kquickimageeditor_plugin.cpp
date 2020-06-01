#include <QQmlEngine>

#include "kquickimageeditor_plugin.h"

#include <resizehandle.h>
#include <resizerectangle.h>
#include <imageitem.h>
#include <imagedocument.h>

void KQuickImageEditorPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<ResizeHandle>(uri, 1, 0, "ResizeHandle");
    qmlRegisterType<ResizeRectangle>(uri, 1, 0, "ResizeRectangle");
    qmlRegisterType<ImageItem>(uri, 1, 0, "ImageItem");
    qmlRegisterType<ImageDocument>(uri, 1, 0, "ImageDocument");
    qmlRegisterType(fromBase(QStringLiteral("controls/BasicResizeHandle.qml")), uri, 1, 0, "BasicResizeHandle");
}
