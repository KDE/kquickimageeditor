/*
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "mirrorcommand.h"

MirrorCommand::MirrorCommand(bool horizonal, bool vertical)
    : m_horizontal(horizonal)
    , m_vertical(vertical)
{
}

QImage MirrorCommand::undo(QImage image)
{
    return image.mirrored(m_horizontal, m_vertical);
}

QImage MirrorCommand::redo(QImage image)
{
    return image.mirrored(m_horizontal, m_vertical);
}
