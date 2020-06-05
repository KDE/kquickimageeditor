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

            property bool resizing: false;
            property string imagePath: "/usr/share/wallpapers/Next/contents/images/5120x2880.jpg";

            function crop() {
                const ratioX = editImage.paintedWidth / editImage.nativeWidth;
                const ratioY = editImage.paintedHeight / editImage.nativeHeight;
                rootEditorView.resizing = false
                imageDoc.crop((resizeRectangle.insideX - rootEditorView.contentItem.width + editImage.paintedWidth) / ratioX, (resizeRectangle.insideY - rootEditorView.contentItem.height + editImage.paintedHeight) / ratioY, resizeRectangle.insideWidth / ratioX, resizeRectangle.insideHeight / ratioY);
            }

            actions {
                right: Kirigami.Action {
                    id: undoAction
                    text: i18nc("@action:button Undo modification", "Undo")
                    iconName: "edit-undo"
                    onTriggered: imageDoc.undo();
                    visible: imageDoc.edited
                }
                contextualActions: [
                    Kirigami.Action {
                        id: saveAction
                        text: i18nc("@action:button Save the image as a new image", "Save As")
                        iconName: "document-save-as"
                        onTriggered: fileDialog.visible = true;
                    },
                    Kirigami.Action {
                        id: saveAsAction
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
                
                OpacityMask {
                    anchors.fill: editImage
                    visible: rootEditorView.resizing
                    maskSource: resizeRectangle
                    source: editImage
                    invert: false
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

                width: editImage.width
                height: editImage.height
                x: 0
                y: 0
                
                insideX: 100
                insideY: 100
                insideWidth: 100
                insideHeight: 100

                onAcceptSize: rootEditorView.crop();
                
                //resizeHandle: KQuickImageEditor.BasicResizeHandle { }

                /*Rectangle {
                    radius: 2
                    width: Kirigami.Units.gridUnit * 8
                    height: Kirigami.Units.gridUnit * 3
                    anchors.centerIn: parent
                    Kirigami.Theme.colorSet: Kirigami.Theme.View
                    color: Kirigami.Theme.backgroundColor
                    QQC2.Label {
                        anchors.centerIn: parent
                        text: "x: " + (resizeRectangle.x - rootEditorView.contentItem.width + editImage.paintedWidth)
                            + " y: " +  (resizeRectangle.y - rootEditorView.contentItem.height + editImage.paintedHeight)
                            + "\nwidth: " + resizeRectangle.width
                            + " height: " + resizeRectangle.height
                    }
                }*/
            }
        }
    }
}
