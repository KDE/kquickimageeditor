/*
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "undocommand.h"

#include <QImage>
#include <QRect>

/**
 * @brief CropCommand that crop the current image.
 */
class CropCommand : public UndoCommand
{
public:
    /**
     * Contructor
     */
    CropCommand(const QRect &cropRect);
    ~CropCommand() override = default;

    QImage redo(QImage image) override;

    QImage undo(QImage image) override;

private:
    QImage m_image;
    QRect m_cropRect;
};
