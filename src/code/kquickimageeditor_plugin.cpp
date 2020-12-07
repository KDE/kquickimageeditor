// SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QQmlEngine>
#include <QResource>

#include "kquickimageeditor_plugin.h"

#include <resizehandle.h>
#include <resizerectangle.h>
#include <imageitem.h>
#include <imagedocument.h>

void KQuickImageEditorPlugin::registerTypes(const char *uri)
{
#if defined(Q_OS_ANDROID) && QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QResource::registerResource(QStringLiteral("assets:/android_rcc_bundle.rcc"));
#endif

    qmlRegisterType<ResizeHandle>(uri, 1, 0, "ResizeHandle");
    qmlRegisterType<ResizeRectangle>(uri, 1, 0, "ResizeRectangle");
    qmlRegisterType<ImageItem>(uri, 1, 0, "ImageItem");
    qmlRegisterType<ImageDocument>(uri, 1, 0, "ImageDocument");
    qmlRegisterType(resolveFileUrl(QStringLiteral("BasicResizeHandle.qml")), uri, 1, 0, "BasicResizeHandle");
}
