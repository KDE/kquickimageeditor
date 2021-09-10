/* SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 * SPDX-FileCopyrightText: 2021 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "resizecommand.h"

ResizeCommand::ResizeCommand(const QSize &resizeSize)
    : m_resizeSize(resizeSize)
{
}

QImage ResizeCommand::undo(QImage image)
{
    Q_UNUSED(image)
    return m_image;
}

QImage ResizeCommand::redo(QImage image)
{
    m_image = image;
    return m_image.scaled(m_resizeSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}
