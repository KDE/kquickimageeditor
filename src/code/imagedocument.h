/*
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QObject>
#include <QImage>
#include <QStack>

#include "commands/undocommand.h"

/**
 * @brief An ImageDocument is the base class of the ImageEditor.
 * 
 * This class handles various image manipulation and contains an undo stack to allow
 * reverting the last actions. This class does not display the image, use @c ImageItem
 * for this task.
 * 
 * @code{.qml}
 * KQuickImageEditor.ImageDocument {
 *    id: imageDocument
 *    path: myModel.image
 * }
 * 
 * Kirigami.Actions {
 *    iconName: "object-rotate-left"
 *    onTriggered: imageDocument.rotate(-90);
 * }
 * 
 * KQuickImageEditor.ImageItem {
 *     image: imageDocument.image
 * }
 * @endcode
 */
class ImageDocument : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QImage image READ image NOTIFY imageChanged)
    Q_PROPERTY(bool edited READ edited WRITE setEdited NOTIFY editedChanged)
    
public:
    ImageDocument(QObject *parent = nullptr);
    ~ImageDocument() override = default;
    
    /**
     * The image was is displayed. This propriety is updated when the path change
     * or commands are applied.
     * 
     * @see imageChanged
     */
    QImage image() const;
    
    /**
     * This propriety store if the document was changed or not.
     * 
     * @see setEdited
     * @see editedChanged
     */
    bool edited() const;
    
    /**
     * Change the edited value.
     * @param value The new value.
     */
    void setEdited(bool value);
    
    QString path() const;
    void setPath(const QString &path);

    /**
     * Rotate the image.
     * @param angle The angle of the rotation in degree.
     */
    Q_INVOKABLE void rotate(int angle);

    /**
     * Mirrror the image.
     * @param horizonal Mirror the image horizontally.
     * @param vertical Mirror the image vertically.
     */
    Q_INVOKABLE void mirror(bool horizontal, bool vertical);
    
    /**
     * Crop the image.
     * @param x The x coordinate of the new image in the old image.
     * @param y The y coordinate of the new image in the old image.
     * @param width The width of the new image.
     * @param height The height of the new image.
     */
    Q_INVOKABLE void crop(int x, int y, int width, int height);

    /**
     * Undo the last edit on the images.
     */
    Q_INVOKABLE void undo();

    /**
     * Cancel all the edit.
     */
    Q_INVOKABLE void cancel();

    /**
     * Save current edited image in place. This is a destructive operation and can't be reverted.
     * @return true iff the file saving operattion was successful.
     */
    Q_INVOKABLE bool save();

    /**
     * Save current edited image as a new image.
     * @param location The location where to save the new image.
     * @return true iff the file saving operattion was successful.
     */
    Q_INVOKABLE bool saveAs(const QUrl &location);

signals:
    void pathChanged(const QString &url);
    void imageChanged();
    void editedChanged();
    
private:
    QString m_path;
    QStack<UndoCommand *> m_undos;
    QImage m_image;
    bool m_edited;
};
