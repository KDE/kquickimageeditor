// SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QQmlEngine>
#include <QResource>

#include "kquickimageeditor_plugin.h"

#include "imagedocument.h"
#include "imageitem.h"
#include "resizehandle.h"
#include "resizerectangle.h"

void KQuickImageEditorPlugin::registerTypes(const char *uri)
{
#if defined(Q_OS_ANDROID)
    QResource::registerResource(QStringLiteral("assets:/android_rcc_bundle.rcc"));
#endif

    qmlRegisterType<ResizeHandle>(uri, 1, 0, "ResizeHandle");
    qmlRegisterType<ResizeRectangle>(uri, 1, 0, "ResizeRectangle");
    qmlRegisterType<ImageItem>(uri, 1, 0, "ImageItem");
    qmlRegisterType<ImageDocument>(uri, 1, 0, "ImageDocument");
    qmlRegisterType(resolveFileUrl(QStringLiteral("BasicResizeHandle.qml")), uri, 1, 0, "BasicResizeHandle");
    qmlRegisterType(resolveFileUrl(QStringLiteral("SelectionTool.qml")), uri, 1, 0, "SelectionTool");
    qmlRegisterType(resolveFileUrl(QStringLiteral("SelectionHandle.qml")), uri, 1, 0, "SelectionHandle");
    qmlRegisterType(resolveFileUrl(QStringLiteral("SelectionBackground.qml")), uri, 1, 0, "SelectionBackground");
    qmlRegisterType(resolveFileUrl(QStringLiteral("CropBackground.qml")), uri, 1, 0, "CropBackground");
    qmlRegisterType(resolveFileUrl(QStringLiteral("RectangleCutout.qml")), uri, 1, 0, "RectangleCutout");
}
