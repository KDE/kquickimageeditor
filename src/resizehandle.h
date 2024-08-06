/*
 * SPDX-FileCopyrightText: (C) 2019 Marco Martin <mart@kde.org>
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QQuickItem>

class ResizeRectangle;

class ResizeHandle : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool resizeBlocked READ resizeBlocked NOTIFY resizeBlockedChanged)
    Q_PROPERTY(QQuickItem *rectangle READ rectangle WRITE setRectangle NOTIFY rectangleChanged)

public:
    enum Corner {
        Left = 0,
        TopLeft,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
    };
    Q_ENUM(Corner)

    explicit ResizeHandle(QQuickItem *parent = nullptr);
    ~ResizeHandle() = default;

    QQuickItem *rectangle() const;
    void setRectangle(QQuickItem *rectangle);

    bool resizeBlocked() const;

    void setResizeCorner(Corner corner);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

Q_SIGNALS:
    void resizeCornerChanged();
    void resizeBlockedChanged();
    void rectangleChanged();

private:
    inline bool resizeLeft() const;
    inline bool resizeTop() const;
    inline bool resizeRight() const;
    inline bool resizeBottom() const;
    void setResizeBlocked(bool width, bool height);

    QPointF m_mouseDownPosition;
    QRectF m_mouseDownGeometry;

    Corner m_resizeCorner = Left;
    bool m_resizeWidthBlocked = false;
    bool m_resizeHeightBlocked = false;
    ResizeRectangle *m_rectangle = nullptr;
};
