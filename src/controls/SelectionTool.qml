/* SPDX-FileCopyrightText: 2021 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

import QtQuick 2.15
import QtQml 2.15

Item {
    id: root
    // make this readonly so it can be accessed without risking external modification
    readonly property SelectionHandle pressedHandle: _private.pressedHandle
    readonly property alias selectionArea: selectionArea
    property alias selectionX: selectionArea.x
    property alias selectionY: selectionArea.y
    property alias selectionWidth: selectionArea.width
    property alias selectionHeight: selectionArea.height

    QtObject {
        id: _private
        property SelectionHandle pressedHandle: null
    }

    MouseArea {
        id: selectionArea
        x: 0
        y: 0
        z: 1
        width: parent.width
        height: parent.height
        LayoutMirroring.enabled: false
        anchors.left: if (_private.pressedHandle) {
            if (_private.pressedHandle.backwardDiagonal) {
                handleTopLeft.horizontalCenter
            } else if (_private.pressedHandle.forwardDiagonal) {
                handleBottomLeft.horizontalCenter
            } else if (_private.pressedHandle.horizontalOnly) {
                handleLeft.horizontalCenter
            }
        }
        anchors.right: if (_private.pressedHandle) {
            if (_private.pressedHandle.backwardDiagonal) {
                handleBottomRight.horizontalCenter
            } else if (_private.pressedHandle.forwardDiagonal) {
                handleTopRight.horizontalCenter
            } else if (_private.pressedHandle.horizontalOnly) {
                handleRight.horizontalCenter
            }
        }
        anchors.top: if (_private.pressedHandle) {
            if (_private.pressedHandle.backwardDiagonal) {
                handleTopLeft.verticalCenter
            } else if (_private.pressedHandle.forwardDiagonal) {
                handleTopRight.verticalCenter
            } else if (_private.pressedHandle.verticalOnly) {
                handleTop.verticalCenter
            }
        }
        anchors.bottom: if (_private.pressedHandle) {
            if (_private.pressedHandle.backwardDiagonal) {
                handleBottomRight.verticalCenter
            } else if (_private.pressedHandle.forwardDiagonal) {
                handleBottomLeft.verticalCenter
            } else if (_private.pressedHandle.verticalOnly) {
                handleBottom.verticalCenter
            }
        }
        enabled: drag.target
        cursorShape: if (_private.pressedHandle || (pressed && enabled)) {
            Qt.ClosedHandCursor
        } else if (enabled) {
            Qt.OpenHandCursor
        } else {
            Qt.ArrowCursor
        }
        drag {
            axis: Drag.XAndYAxis
            target: (selectionArea.width === root.width && selectionArea.height === root.height) || _private.pressedHandle ? null : selectionArea
            minimumX: 0
            maximumX: root.width - selectionArea.width
            minimumY: 0
            maximumY: root.height - selectionArea.height
            threshold: 0
        }
    }

    SelectionHandle {
        id: handleTopLeft
        target: selectionArea
        position: SelectionHandle.TopLeft
        lockX: _private.pressedHandle && _private.pressedHandle.backwardDiagonal
        lockY: lockX
        drag.maximumX: handleBottomRight.x - implicitWidth / 2
        drag.maximumY: handleBottomRight.y - implicitHeight / 2
        Binding {
            target: _private; property: "pressedHandle"
            value: handleTopLeft; when: handleTopLeft.pressed
            restoreMode: Binding.RestoreBindingOrValue
        }
    }
    SelectionHandle {
        id: handleTop
        visible: selectionArea.width >= implicitWidth
        target: selectionArea
        position: SelectionHandle.Top
        lockY: _private.pressedHandle && _private.pressedHandle.verticalOnly
        drag.maximumY: handleBottom.y - implicitHeight / 2
        Binding {
            target: _private; property: "pressedHandle"
            value: handleTop; when: handleTop.pressed
            restoreMode: Binding.RestoreBindingOrValue
        }
    }
    SelectionHandle {
        id: handleTopRight
        target: selectionArea
        position: SelectionHandle.TopRight
        lockX: _private.pressedHandle && _private.pressedHandle.forwardDiagonal
        lockY: lockX
        drag.minimumX: handleBottomLeft.x + implicitWidth / 2
        drag.maximumY: handleBottomLeft.y - implicitHeight / 2
        Binding {
            target: _private; property: "pressedHandle"
            value: handleTopRight; when: handleTopRight.pressed
            restoreMode: Binding.RestoreBindingOrValue
        }
    }
    SelectionHandle {
        id: handleLeft
        visible: selectionArea.height >= implicitHeight
        target: selectionArea
        position: SelectionHandle.Left
        lockX: _private.pressedHandle && _private.pressedHandle.horizontalOnly
        drag.maximumX: handleRight.x - implicitWidth / 2
        Binding {
            target: _private; property: "pressedHandle"
            value: handleLeft; when: handleLeft.pressed
            restoreMode: Binding.RestoreBindingOrValue
        }
    }
    SelectionHandle {
        id: handleRight
        visible: selectionArea.height >= implicitHeight
        target: selectionArea
        position: SelectionHandle.Right
        lockX: _private.pressedHandle && _private.pressedHandle.horizontalOnly
        drag.minimumX: handleLeft.x + implicitWidth / 2
        Binding {
            target: _private; property: "pressedHandle"
            value: handleRight; when: handleRight.pressed
            restoreMode: Binding.RestoreBindingOrValue
        }
    }
    SelectionHandle {
        id: handleBottomLeft
        target: selectionArea
        position: SelectionHandle.BottomLeft
        lockX: _private.pressedHandle && _private.pressedHandle.forwardDiagonal
        lockY: lockX
        drag.maximumX: handleTopRight.x - implicitWidth / 2
        drag.minimumY: handleTopRight.y + implicitHeight / 2
        Binding {
            target: _private; property: "pressedHandle"
            value: handleBottomLeft; when: handleBottomLeft.pressed
            restoreMode: Binding.RestoreBindingOrValue
        }
    }
    SelectionHandle {
        id: handleBottom
        visible: selectionArea.width >= implicitWidth
        target: selectionArea
        position: SelectionHandle.Bottom
        lockY: _private.pressedHandle && _private.pressedHandle.verticalOnly
        drag.minimumY: handleTop.y + implicitHeight / 2
        Binding {
            target: _private; property: "pressedHandle"
            value: handleBottom; when: handleBottom.pressed
            restoreMode: Binding.RestoreBindingOrValue
        }
    }
    SelectionHandle {
        id: handleBottomRight
        target: selectionArea
        position: SelectionHandle.BottomRight
        lockX: _private.pressedHandle && _private.pressedHandle.backwardDiagonal
        lockY: lockX
        drag.minimumX: handleTopLeft.x + implicitWidth / 2
        drag.minimumY: handleTopLeft.y + implicitHeight / 2
        Binding {
            target: _private; property: "pressedHandle"
            value: handleBottomRight; when: handleBottomRight.pressed
            restoreMode: Binding.RestoreBindingOrValue
        }
    }
    // TODO: maybe scale proportions instead of just limiting size
    onWidthChanged: if (selectionArea.x + selectionArea.width > root.width) {
        selectionArea.width = Math.max(root.width - selectionArea.x, handleTopLeft.implicitWidth/2)
        if (selectionArea.x > root.width) {
            selectionArea.x = Math.max(root.width - selectionArea.width, 0)
        }
    }
    onHeightChanged: if (selectionArea.y + selectionArea.height > root.height) {
        selectionArea.height = Math.max(root.height - selectionArea.y, handleTopLeft.implicitHeight/2)
        if (selectionArea.y > root.height) {
            selectionArea.y = Math.max(root.height - selectionArea.height, 0)
        }
    }
}
