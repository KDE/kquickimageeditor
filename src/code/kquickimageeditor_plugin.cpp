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
    qmlRegisterType<ResizeHandle>(uri, 1, 0, "ResizeHandle");
    qmlRegisterType<ResizeRectangle>(uri, 1, 0, "ResizeRectangle");
    qmlRegisterType<ImageItem>(uri, 1, 0, "ImageItem");
    qmlRegisterType<ImageDocument>(uri, 1, 0, "ImageDocument");
}
