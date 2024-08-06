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
 * @brief MirrorCommand that mirror an image horizontally or vertically.
 */
class MirrorCommand : public UndoCommand
{
public:
    MirrorCommand(bool horizontal, bool vertical);
    ~MirrorCommand() override = default;

    QImage redo(QImage image) override;

    QImage undo(QImage image) override;

private:
    bool m_horizontal;
    bool m_vertical;
};
