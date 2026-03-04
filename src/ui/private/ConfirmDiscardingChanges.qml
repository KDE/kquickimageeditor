// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

pragma ComponentBehavior: Bound

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.ki18n

Kirigami.PromptDialog {
    id: root

    required property string imageFileName
    required property KI18nContext libI18n

    signal saveChanges()
    signal discardChanges()

    title: libI18n.i18nc("@title", "Unsaved Changes")
    subtitle: libI18n.xi18nc("@label", "The image <filename>%1</filename> has been modified. Do you want to save your changes or discard them?", imageFileName)
    dialogType: Kirigami.PromptDialog.Warning

    standardButtons: Kirigami.Dialog.Save | Kirigami.Dialog.Discard | Kirigami.Dialog.Cancel

    onAccepted: {
        saveChanges();
        close();
    }

    onDiscarded: {
        discardChanges();
        close();
    }

    onRejected: close()
}
