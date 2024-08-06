/*
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "rotatecommand.h"

RotateCommand::RotateCommand(const QTransform &tranform)
    : m_tranform(tranform)
{
}

QImage RotateCommand::undo(QImage image)
{
    return image.transformed(m_tranform.rotate(180));
}

QImage RotateCommand::redo(QImage image)
{
    return image.transformed(m_tranform);
}
