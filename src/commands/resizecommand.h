/* SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 * SPDX-FileCopyrightText: 2021 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "undocommand.h"

#include <QImage>
#include <QRect>

/**
 * @brief ResizeCommand that resizes the current image.
 */
class ResizeCommand : public UndoCommand
{
public:
    /**
     * Contructor
     */
    ResizeCommand(const QSize &resizeSize);
    ~ResizeCommand() override = default;

    QImage redo(QImage image) override;

    QImage undo(QImage image) override;

private:
    QImage m_image;
    QSize m_resizeSize;
};
