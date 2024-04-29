/*
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "cropcommand.h"

CropCommand::CropCommand(const QRect &cropRect)
    : m_cropRect(cropRect)
{
}

QImage CropCommand::undo(QImage image)
{
    Q_UNUSED(image)
    return m_image;
}

QImage CropCommand::redo(QImage image)
{
    m_image = image;
    if (m_cropRect.x() < 0) {
        m_cropRect.setWidth(m_cropRect.width() + m_cropRect.x());
        m_cropRect.setX(0);
    }
    if (m_cropRect.y() < 0) {
        m_cropRect.setHeight(m_cropRect.height() + m_cropRect.y());
        m_cropRect.setY(0);
    }
    if (m_image.width() < m_cropRect.width() + m_cropRect.x()) {
        m_cropRect.setWidth(m_image.width() - m_cropRect.x());
    }
    if (m_image.height() < m_cropRect.height() + m_cropRect.y()) {
        m_cropRect.setHeight(m_image.height() - m_cropRect.y());
    }
    return m_image.copy(m_cropRect);
}
