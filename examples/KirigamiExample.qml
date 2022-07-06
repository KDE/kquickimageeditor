/*
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

import QtQuick 2.10
import QtQuick.Controls 2.1 as QQC2
import QtQuick.Layouts 1.12
import org.kde.kirigami 2.12 as Kirigami
import QtQuick.Dialogs 1.2
import org.kde.kquickimageeditor 1.0 as KQuickImageEditor
import QtGraphicalEffects 1.12

Kirigami.ApplicationWindow {
    id: root
    Component.onCompleted: {
        pageStack.layers.push(editorComponent);
    }
    
    pageStack.initialPage: Kirigami.Page {
        QQC2.Button {
            text: "Open Editor"
            onClicked: root.pageStack.layers.push(editorComponent);
        }
    }
    
    Component { 
        id: editorComponent

        Kirigami.Page {
            id: rootEditorView

            property bool resizing: false;
            property string imagePath: '/usr/share/wallpapers/Next/contents/images/5120x2880.jpg'

            signal imageEdited();

            title: "Edit"
            leftPadding: 0
            rightPadding: 0
            topPadding: 0
            bottomPadding: 0

            function crop() {
                rootEditorView.resizing = false
                imageDoc.crop(selectionTool.selectionX / editImage.ratioX,
                              selectionTool.selectionY / editImage.ratioY,
                              selectionTool.selectionWidth / editImage.ratioX,
                              selectionTool.selectionHeight / editImage.ratioY);
            }

            actions {
                main: Kirigami.Action {
                    id: saveAction
                    visible: imageDoc.edited
                    text: "Save"
                    iconName: "document-save"
                    onTriggered: {
                        if (!imageDoc.save()) {
                            msg.type = Kirigami.MessageType.Error
                            msg.text = "Unable to save file. Check if you have the correct permission to edit this file."
                            msg.visible = true;
                        }
                        rootEditorView.imageEdited();
                        applicationWindow().pageStack.layers.pop();
                    }
                }
                left: Kirigami.Action {
                    id: undoAction
                    text: "Undo"
                    iconName: "edit-undo"
                    onTriggered: {
                        if (imageDoc.edited) {
                            imageDoc.undo();
                        }
                    }
                    visible: imageDoc.edited
                }
                contextualActions: [
                    Kirigami.Action {
                        iconName: rootEditorView.resizing ? "dialog-ok" : "transform-crop"
                        text: rootEditorView.resizing ? "Accept" : "Crop"
                        onTriggered: rootEditorView.resizing = !rootEditorView.resizing;
                    },
                    Kirigami.Action {
                        iconName: "dialog-cancel"
                        visible: rootEditorView.resizing
                        text: "Cancel"
                        onTriggered: rootEditorView.resizing = !rootEditorView.resizing
                    },
                    Kirigami.Action {
                        iconName: "object-rotate-left"
                        text: "Rotate left";
                        onTriggered: imageDoc.rotate(-90);
                        visible: !rootEditorView.resizing
                    },
                    Kirigami.Action {
                        iconName: "object-rotate-right"
                        text: "@action:button Rotate an image to the right", "Rotate right";
                        onTriggered: imageDoc.rotate(90);
                        visible: !rootEditorView.resizing
                    },
                    Kirigami.Action {
                        iconName: "object-flip-vertical"
                        text: "Flip"
                        onTriggered: imageDoc.mirror(false, true);
                        visible: !rootEditorView.resizing
                    },
                    Kirigami.Action {
                        iconName: "object-flip-horizontal"
                        text: "Mirror"
                        onTriggered: imageDoc.mirror(true, false);
                        visible: !rootEditorView.resizing
                    },
                    Kirigami.Action {
                        visible: rootEditorView.resizing
                        displayComponent: QQC2.ToolSeparator {
                            leftPadding: Kirigami.Units.largeSpacing
                            rightPadding: leftPadding
                        }
                    },
                    Kirigami.Action {
                        visible: rootEditorView.resizing
                        displayComponent: QQC2.Label {
                            text: "Size:"
                        }
                    },
                    Kirigami.Action {
                        visible: rootEditorView.resizing
                        displayComponent: EditorSpinBox {
                            minimumContentWidth: widthTextMetrics.width
                            from: 1
                            to: editImage.nativeWidth
                            value: selectionTool.selectionWidth / editImage.ratioX
                            onValueModified: selectionTool.selectionWidth = value * editImage.ratioX
                        }
                    },
                    Kirigami.Action {
                        visible: rootEditorView.resizing
                        displayComponent: EditorSpinBox {
                            minimumContentWidth: heightTextMetrics.width
                            from: 1
                            to: editImage.nativeHeight
                            value: selectionTool.selectionHeight / editImage.ratioY
                            onValueModified: selectionTool.selectionHeight = value * editImage.ratioY
                        }
                    },
                    Kirigami.Action {
                        visible: rootEditorView.resizing
                        displayComponent: Item {
                            implicitWidth: Kirigami.Units.largeSpacing
                        }
                    },
                    Kirigami.Action {
                        visible: rootEditorView.resizing
                        displayComponent: QQC2.Label {
                            text: "Position:"
                        }
                    },
                    Kirigami.Action {
                        visible: rootEditorView.resizing
                        displayComponent: EditorSpinBox {
                            minimumContentWidth: widthTextMetrics.width
                            from: 0
                            to: editImage.nativeWidth - (selectionTool.selectionWidth / editImage.ratioX)
                            value: selectionTool.selectionX / editImage.ratioX
                            onValueModified: selectionTool.selectionX = value * editImage.ratioX
                        }
                    },
                    Kirigami.Action {
                        visible: rootEditorView.resizing
                        displayComponent: EditorSpinBox {
                            minimumContentWidth: heightTextMetrics.width
                            from: 0
                            to: editImage.nativeHeight - (selectionTool.selectionHeight / editImage.ratioY)
                            value: selectionTool.selectionY / editImage.ratioY
                            onValueModified: selectionTool.selectionY = value * editImage.ratioY
                        }
                    }
                ]
            }

            TextMetrics {
                id: widthTextMetrics
                text: editImage.nativeWidth.toLocaleString(rootEditorView.locale, 'f', 0)
            }

            TextMetrics {
                id: heightTextMetrics
                text: editImage.nativeHeight.toLocaleString(rootEditorView.locale, 'f', 0)
            }

            component EditorSpinBox : QQC2.SpinBox {
                id: control
                property real minimumContentWidth: 0
                contentItem: QQC2.TextField {
                    id: textField
                    implicitWidth: control.minimumContentWidth + leftPadding + rightPadding + 2
                    implicitHeight: Math.ceil(contentHeight) + topPadding + bottomPadding
                    palette: control.palette
                    leftPadding: control.spacing
                    rightPadding: control.spacing
                    topPadding: 0
                    bottomPadding: 0
                    text: control.displayText
                    font: control.font
                    color: Kirigami.Theme.textColor
                    selectionColor: Kirigami.Theme.highlightColor
                    selectedTextColor: Kirigami.Theme.highlightedTextColor
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                    readOnly: !control.editable
                    validator: control.validator
                    inputMethodHints: control.inputMethodHints
                    selectByMouse: true
                    background: null
                }
            }

            FileDialog {
                id: fileDialog
                title: "Save As"
                folder: shortcuts.home
                selectMultiple: false
                selectExisting: false
                onAccepted: {
                    if (imageDoc.saveAs(fileDialog.fileUrl)) {;
                        imagePath = fileDialog.fileUrl;
                        msg.type = Kirigami.MessageType.Information
                        msg.text = "You are now editing a new file."
                        msg.visible = true;
                    } else {
                        msg.type = Kirigami.MessageType.Error
                        msg.text = "Unable to save file. Check if you have the correct permission to edit this file."
                        msg.visible = true;
                    }
                    fileDialog.close()
                }
                onRejected: {
                    fileDialog.close()
                }
                Component.onCompleted: visible = false
            }

            KQuickImageEditor.ImageItem {
                id: editImage
                readonly property real ratioX: editImage.paintedWidth / editImage.nativeWidth;
                readonly property real ratioY: editImage.paintedHeight / editImage.nativeHeight;

                // Assigning this to the contentItem and setting the padding causes weird positioning issues
                anchors.fill: parent
                anchors.margins: Kirigami.Units.gridUnit
                fillMode: KQuickImageEditor.ImageItem.PreserveAspectFit
                image: imageDoc.image

                Shortcut {
                    sequence: StandardKey.Undo
                    onActivated: undoAction.trigger();
                }

                Shortcut {
                    sequences: [StandardKey.Save, "Enter"]
                    onActivated: saveAction.trigger();
                }

                Shortcut {
                    sequence: StandardKey.SaveAs
                    onActivated: saveAsAction.trigger();
                }

                KQuickImageEditor.ImageDocument {
                    id: imageDoc
                    path: rootEditorView.imagePath
                }

                KQuickImageEditor.SelectionTool {
                    id: selectionTool
                    visible: rootEditorView.resizing
                    width: editImage.paintedWidth
                    height: editImage.paintedHeight
                    x: editImage.horizontalPadding
                    y: editImage.verticalPadding
                    KQuickImageEditor.CropBackground {
                        anchors.fill: parent
                        z: -1
                        insideX: selectionTool.selectionX
                        insideY: selectionTool.selectionY
                        insideWidth: selectionTool.selectionWidth
                        insideHeight: selectionTool.selectionHeight
                    }
                    Connections {
                        target: selectionTool.selectionArea
                        function onDoubleClicked() {
                            rootEditorView.crop()
                        }
                    }
                }
                onImageChanged: {
                    selectionTool.selectionX = 0
                    selectionTool.selectionY = 0
                    selectionTool.selectionWidth = Qt.binding(() => selectionTool.width)
                    selectionTool.selectionHeight = Qt.binding(() => selectionTool.height)
                }
            }


            footer: Kirigami.InlineMessage {
                id: msg
                type: Kirigami.MessageType.Error
                showCloseButton: true
                visible: false
            }
        }
    }
}
