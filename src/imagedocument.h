/*
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QImage>
#include <QObject>
#include <QStack>
#include <QUrl>
#include <qqmlregistration.h>

#include "commands/undocommand.h"

/*!
 * \inqmlmodule org.kde.kquickimageeditor
 * \qmltype ImageDocument
 * \brief An ImageDocument is the base class of the ImageEditor.
 *
 * This class handles various image manipulation and contains an undo stack to allow
 * reverting the last actions. This class does not display the image, use ImageItem
 * for this task.
 *
 * \qml
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
 * \endqml
 */
class ImageDocument : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    /*!
     * \qmlproperty url ImageDocument::path
     * The path to the image.
     */
    Q_PROPERTY(QUrl path READ path WRITE setPath NOTIFY pathChanged)
    /*!
     * \qmlproperty image ImageDocument::image
     * The image was is displayed.
     *
     * This property is updated when the path changes
     * or commands are applied.
     */
    Q_PROPERTY(QImage image READ image NOTIFY imageChanged)
    /*!
     * \qmlproperty bool ImageDocument::edited
     * Whether the document was changed or not.
     *
     * Allows to change the edited value.
     */
    Q_PROPERTY(bool edited READ edited WRITE setEdited NOTIFY editedChanged)

public:
    ImageDocument(QObject *parent = nullptr);
    ~ImageDocument() override = default;

    QImage image() const;

    bool edited() const;

    void setEdited(bool value);

    QUrl path() const;
    void setPath(const QUrl &path);

    /*!
     * \qmlmethod void ImageDocument::rotate(int angle)
     * Rotate the image with the given \a angle.
     */
    Q_INVOKABLE void rotate(int angle);

    /*!
     * \qmlmethod void ImageDocument::mirror(bool horizontal, bool vertical)
     * Mirror the image \a horizontally and/or \a vertically.
     */
    Q_INVOKABLE void mirror(bool horizontal, bool vertical);

    /*!
     * \qmlmethod void ImageDocument::crop(int x, int y, int width, int height)
     * \brief Crop the image.
     *
     * Requires specifying the initial \a x and \a y coordinates for the crop in the old image
     * as well as the \a width and \a height of the crop selection.
     */
    Q_INVOKABLE void crop(int x, int y, int width, int height);

    /*!
     * \qmlmethod void ImageDocument::resize(int width, int height)
     * Resize the image to the new \a width and \a height.
     */
    Q_INVOKABLE void resize(int width, int height);

    /*!
     * \qmlmethod void ImageDocument::undo()
     * Undo the last edit on the images.
     */
    Q_INVOKABLE void undo();

    /*!
     * \qmlmethod void ImageDocument::cancel()
     * Cancel all edits.
     */
    Q_INVOKABLE void cancel();

    /*!
     * \qmlmethod bool ImageDocument::save()
     * \brief Save current edited image in place.
     *
     * This is a destructive operation and can't be reverted.
     *
     * Returns \c true if the file saving operation was successful.
     */
    Q_INVOKABLE bool save();

    /*!
     * \qmlmethod bool ImageDocument::saveSas(url location)
     * \brief Save current edited image as a new image in \a location.
     *
     * Returns \c true if the file saving operation was successful.
     */
    Q_INVOKABLE bool saveAs(const QUrl &location);

Q_SIGNALS:
    void pathChanged(const QUrl &url);
    void imageChanged();
    void editedChanged();

private:
    QUrl m_path;
    QStack<UndoCommand *> m_undos;
    QImage m_image;
    bool m_edited;
};
