/*
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "imagedocument.h"

#include "commands/cropcommand.h"
#include "commands/mirrorcommand.h"
#include "commands/resizecommand.h"
#include "commands/rotatecommand.h"
#include "commands/undocommand.h"

#include <QImage>
#include <QUrl>

ImageDocument::ImageDocument(QObject *parent)
    : QObject(parent)
{
    connect(this, &ImageDocument::pathChanged, this, [this](const QUrl &url) {
        m_image = QImage(url.isLocalFile() ? url.toLocalFile() : url.toString());
        m_edited = false;
        Q_EMIT editedChanged();
        Q_EMIT imageChanged();
    });
}

void ImageDocument::cancel()
{
    while (!m_undos.empty()) {
        const auto command = m_undos.pop();
        m_image = command->undo(m_image);
        delete command;
    }
    setEdited(false);
    Q_EMIT imageChanged();
}

QImage ImageDocument::image() const
{
    return m_image;
}

bool ImageDocument::edited() const
{
    return m_edited;
}

void ImageDocument::undo()
{
    Q_ASSERT(!m_undos.empty());
    const auto command = m_undos.pop();
    m_image = command->undo(m_image);
    delete command;
    Q_EMIT imageChanged();
    if (m_undos.empty()) {
        setEdited(false);
    }
}

void ImageDocument::crop(int x, int y, int width, int height)
{
    const auto command = new CropCommand(QRect(x, y, width, height));
    m_image = command->redo(m_image);
    m_undos.append(command);
    setEdited(true);
    Q_EMIT imageChanged();
}

void ImageDocument::resize(int width, int height)
{
    const auto command = new ResizeCommand(QSize(width, height));
    m_image = command->redo(m_image);
    m_undos.append(command);
    setEdited(true);
    Q_EMIT imageChanged();
}

void ImageDocument::mirror(bool horizontal, bool vertical)
{
    const auto command = new MirrorCommand(horizontal, vertical);
    m_image = command->redo(m_image);
    m_undos.append(command);
    setEdited(true);
    Q_EMIT imageChanged();
}

void ImageDocument::rotate(int angle)
{
    QTransform transform;
    transform.rotate(angle);
    const auto command = new RotateCommand(transform);
    m_image = command->redo(m_image);
    m_undos.append(command);
    setEdited(true);
    Q_EMIT imageChanged();
}

void ImageDocument::setEdited(bool value)
{
    if (m_edited == value) {
        return;
    }

    m_edited = value;
    Q_EMIT editedChanged();
}

bool ImageDocument::save()
{
    return m_image.save(m_path.isLocalFile() ? m_path.toLocalFile() : m_path.toString());
}

bool ImageDocument::saveAs(const QUrl &location)
{
    return m_image.save(location.isLocalFile() ? location.toLocalFile() : location.toString());
}

QUrl ImageDocument::path() const
{
    return m_path;
}

void ImageDocument::setPath(const QUrl &path)
{
    m_path = path;
    Q_EMIT pathChanged(path);
}

#include "moc_imagedocument.cpp"
