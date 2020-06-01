/*
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

import QtQuick 2.7
import QtQuick.Controls 2.1 as QQC2
import QtQuick.Layouts 1.12
import org.kde.kirigami 2.12 as Kirigami
import QtQuick.Dialogs 1.2
import org.kde.kquickimageeditor 1.0 as KQuickImageEditor

Kirigami.ApplicationWindow {
    id: root
    Component.onCompleted: pageStack.layers.push(editorComponent);
    
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
            title: i18n("Edit")
            leftPadding: 0
            rightPadding: 0

            property bool resizing: false;
            property string imagePath: "/usr/share/wallpapers/Next/contents/images/5120x2880.jpg";

            function crop() {
                const ratioX = editImage.paintedWidth / editImage.nativeWidth;
                const ratioY = editImage.paintedHeight / editImage.nativeHeight;
                rootEditorView.resizing = false
                imageDoc.crop((resizeRectangle.x - rootEditorView.contentItem.width + editImage.paintedWidth) / ratioX, (resizeRectangle.y - rootEditorView.contentItem.height + editImage.paintedHeight) / ratioY, resizeRectangle.width / ratioX, resizeRectangle.height / ratioY);
            }

            actions {
                right: Kirigami.Action {
                    text: i18nc("@action:button Undo modification", "Undo")
                    iconName: "edit-undo"
                    onTriggered: imageDoc.undo();
                    visible: imageDoc.edited
                }
                contextualActions: [
                    Kirigami.Action {
                        text: i18nc("@action:button Save the image as a new image", "Save As")
                        iconName: "document-save-as"
                        onTriggered: fileDialog.visible = true;
                    },
                    Kirigami.Action {
                        text: i18nc("@action:button Save the image", "Save")
                        iconName: "document-save"
                        onTriggered: {
                            if (!imageDoc.save()) {
                                msg.type = Kirigami.MessageType.Error
                                msg.text = i18n("Unable to save file. Check if you have the correct permission to edit this file.")
                                msg.visible = true;
                            }
                        }
                        visible: imageDoc.edited
                    }
                ]
            }


            FileDialog {
                id: fileDialog
                title: i18n("Save As")
                folder: shortcuts.home
                selectMultiple: false
                selectExisting: false
                onAccepted: {
                    if (imageDoc.saveAs(fileDialog.fileUrl)) {;
                        imagePath = fileDialog.fileUrl;
                        msg.type = Kirigami.MessageType.Information
                        msg.text = i18n("You are now editing a new file.")
                        msg.visible = true;
                    } else {
                        msg.type = Kirigami.MessageType.Error
                        msg.text = i18n("Unable to save file. Check if you have the correct permission to edit this file.")
                        msg.visible = true;
                    }
                    fileDialog.close()
                }
                onRejected: {
                    fileDialog.close()
                }
                Component.onCompleted: visible = false
            }
            
            KQuickImageEditor.ImageDocument {
                id: imageDoc
                path: rootEditorView.imagePath
            }

            contentItem: Item {
                KQuickImageEditor.ImageItem {
                    id: editImage
                    fillMode: KQuickImageEditor.ImageItem.PreserveAspectFit
                    image: imageDoc.image
                    anchors.fill: parent
                }
            }

            header: QQC2.ToolBar {
                contentItem: Kirigami.ActionToolBar {
                    id: actionToolBar
                    display: QQC2.Button.TextBesideIcon
                    actions: [
                        Kirigami.Action {
                            iconName: rootEditorView.resizing ? "dialog-cancel" : "transform-crop"
                            text: rootEditorView.resizing ? i18n("Cancel") : i18nc("@action:button Crop an image", "Crop");
                            onTriggered: rootEditorView.resizing = !rootEditorView.resizing;
                        },
                        Kirigami.Action {
                            iconName: "dialog-ok"
                            visible: rootEditorView.resizing
                            text: i18nc("@action:button Rotate an image to the right", "Crop");
                            onTriggered: rootEditorView.crop();
                        },
                        Kirigami.Action {
                            iconName: "object-rotate-left"
                            text: i18nc("@action:button Rotate an image to the left", "Rotate left");
                            onTriggered: imageDoc.rotate(-90);
                            visible: !rootEditorView.resizing
                        },
                        Kirigami.Action {
                            iconName: "object-rotate-right"
                            text: i18nc("@action:button Rotate an image to the right", "Rotate right");
                            onTriggered: imageDoc.rotate(90);
                            visible: !rootEditorView.resizing
                        },
                        Kirigami.Action {
                            iconName: "object-flip-vertical"
                            text: i18nc("@action:button Mirror an image vertically", "Flip");
                            onTriggered: imageDoc.mirror(false, true);
                            visible: !rootEditorView.resizing
                        },
                        Kirigami.Action {
                            iconName: "object-flip-horizontal"
                            text: i18nc("@action:button Mirror an image horizontally", "Mirror");
                            onTriggered: imageDoc.mirror(true, false);
                            visible: !rootEditorView.resizing
                        }
                    ]
                }
            }
            
            footer: Kirigami.InlineMessage {
                id: msg
                type: Kirigami.MessageType.Error
                showCloseButton: true
                visible: false
            }

            KQuickImageEditor.ResizeRectangle {
                id: resizeRectangle

                visible: rootEditorView.resizing

                width: 300
                height: 300
                x: 200
                y: 200

                onAcceptSize: crop();

                Rectangle {
                    color: "#3daee9"
                    opacity: 0.6
                    anchors.fill: parent
                }

                KQuickImageEditor.BasicResizeHandle {
                    rectangle: resizeRectangle
                    resizeCorner: KQuickImageEditor.ResizeHandle.TopLeft
                    anchors {
                        horizontalCenter: parent.left
                        verticalCenter: parent.top
                    }
                }
                KQuickImageEditor.BasicResizeHandle {
                    rectangle: resizeRectangle
                    resizeCorner: KQuickImageEditor.ResizeHandle.BottomLeft
                    anchors {
                        horizontalCenter: parent.left
                        verticalCenter: parent.bottom
                    }
                }
                KQuickImageEditor.BasicResizeHandle {
                    rectangle: resizeRectangle
                    resizeCorner: KQuickImageEditor.ResizeHandle.BottomRight
                    anchors {
                        horizontalCenter: parent.right
                        verticalCenter: parent.bottom
                    }
                }
                KQuickImageEditor.BasicResizeHandle {
                    rectangle: resizeRectangle
                    resizeCorner: KQuickImageEditor.ResizeHandle.TopRight
                    anchors {
                        horizontalCenter: parent.right
                        verticalCenter: parent.top
                    }
                }
            }
        }
    }
}
