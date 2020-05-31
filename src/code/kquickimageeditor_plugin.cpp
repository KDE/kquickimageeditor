#include <QQmlEngine>

#include "kquickimageeditor_plugin.h"

#include <resizehandle.h>
#include <resizerectangle.h>
#include <imageitem.h>
#include <imagedocument.h>

#define PUBLIC_URI "org.kde.kquickimageeditor"
#define PRIVATE_URI "org.kde.kquickimageeditor.private"

#define URI(x) fromBase(QStringLiteral(x))

void KQuickImageEditorPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<ResizeHandle>(PUBLIC_URI, 1, 0, "ResizeHandle");
    qmlRegisterType<ResizeRectangle>(PUBLIC_URI, 1, 0, "ResizeRectangle");
    qmlRegisterType<ImageItem>(PUBLIC_URI, 1, 0, "ImageItem");
    qmlRegisterType<ImageDocument>(PUBLIC_URI, 1, 0, "ImageDocument");
    qmlRegisterType(URI("controls/BasicResizeHandle.qml"), PUBLIC_URI, 1, 0, "BasicResizeHandle");
}
