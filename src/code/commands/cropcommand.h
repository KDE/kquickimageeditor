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
 * @brief CropCommand that crops the current images
 */
class CropCommand : public UndoCommand
{
public:
    /**
     * Contructor
     */
    CropCommand(const QRect &cropRect);
    ~CropCommand() = default;

    virtual QImage redo(QImage image) override;

    virtual QImage undo(QImage image) override;
    
private:
    QImage m_image;
    QRect m_cropRect;
};
