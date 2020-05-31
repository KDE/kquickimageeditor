#include <QQmlEngine>

#include "kquickimageeditor_plugin.h"

#define PUBLIC_URI "org.kde.kquickimageeditor"
#define PRIVATE_URI "org.kde.kquickimageeditor.private"

#define URI(x) fromBase(QStringLiteral(x))

void KQuickImageEditorPlugin::registerTypes(const char *uri)
{
    qmlRegisterType(URI("controls/Rectangle.qml"), PUBLIC_URI, 1, 0, "Rektangle");
}