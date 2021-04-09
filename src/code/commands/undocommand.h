/*
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

class QImage;

/**
 * A class implementing the command pattern. This is used to implemented various filters.
 */
class UndoCommand
{
public:
    virtual ~UndoCommand() = 0;

    /**
     * Applies the change to the document.
     */
    virtual QImage redo(QImage image) = 0;

    /**
     * Revert a change to the document.
     */
    virtual QImage undo(QImage image) = 0;
};
