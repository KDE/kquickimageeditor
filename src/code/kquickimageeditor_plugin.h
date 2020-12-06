// SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QDir>
#include <QQmlExtensionPlugin>

class KQuickImageEditorPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
    void registerTypes(const char *uri) override;
private:
    QUrl fromBase(QString path) {
#if defined(Q_OS_ANDROID) && QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        return QUrl(QStringLiteral(":/android_rcc_bundle/qml/org/kde/kirigami.2/") + path);
#elif defined(KIRIGAMI_BUILD_TYPE_STATIC)
        return QUrl(QStringLiteral(":/org/kde/kirigami.2/") + path);
#else
        return QUrl(baseUrl().toLocalFile() + QLatin1Char('/') + path);
#endif
    };
};
