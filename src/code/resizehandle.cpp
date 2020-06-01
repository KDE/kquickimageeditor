/*
 * SPDX-FileCopyrightText: (C) 2019 Marco Martin <mart@kde.org>
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "resizehandle.h"

#include <QCursor>
#include <cmath>

ResizeHandle::ResizeHandle(QQuickItem *parent)
    : QQuickItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    
    auto syncCursor = [this] () {
        switch (m_resizeCorner) {
        case Left:
        case Right:
            setCursor(QCursor(Qt::SizeHorCursor));
            break;
        case Top:
        case Bottom:
            setCursor(QCursor(Qt::SizeVerCursor));
            break;
        case TopLeft:
        case BottomRight:
            setCursor(QCursor(Qt::SizeFDiagCursor));
            break;
        case TopRight:
        case BottomLeft:
        default:
            setCursor(Qt::SizeBDiagCursor);
        }
    };
    
    syncCursor();
    connect(this, &ResizeHandle::resizeCornerChanged, this, syncCursor);
    
}

QQuickItem *ResizeHandle::rectangle() const
{
    return m_rectangle;
}

void ResizeHandle::setRectangle(QQuickItem *rectangle)
{
    if (m_rectangle == rectangle) {
        return;
    }
    m_rectangle = rectangle;
    Q_EMIT rectangleChanged();

}

bool ResizeHandle::resizeBlocked() const
{
    return false; //m_resizeWidthBlocked || m_resizeHeightBlocked;
}

bool ResizeHandle::resizeLeft() const
{
    return m_resizeCorner == Left || m_resizeCorner == TopLeft || m_resizeCorner == BottomLeft;
}

bool ResizeHandle::resizeTop() const
{
    return m_resizeCorner == Top || m_resizeCorner == TopLeft || m_resizeCorner == TopRight;
}

bool ResizeHandle::resizeRight() const
{
    return m_resizeCorner == Right || m_resizeCorner == TopRight || m_resizeCorner == BottomRight;
}

bool ResizeHandle::resizeBottom() const
{
    return m_resizeCorner == Bottom || m_resizeCorner == BottomLeft || m_resizeCorner == BottomRight;
}

void ResizeHandle::setResizeBlocked(bool width, bool height)
{
    if (m_resizeWidthBlocked == width && m_resizeHeightBlocked == height) {
        return;
    }

    m_resizeWidthBlocked = width;
    m_resizeHeightBlocked = height;
    
    Q_EMIT resizeBlockedChanged();
}


void ResizeHandle::mousePressEvent(QMouseEvent *event)
{
    m_mouseDownPosition = event->windowPos();
    m_mouseDownGeometry = QRectF(m_rectangle->x(), m_rectangle->y(), m_rectangle->width(), m_rectangle->height());
    setResizeBlocked(false, false);
    event->accept();
}

void ResizeHandle::mouseMoveEvent(QMouseEvent *event)
{

    const QPointF difference = m_mouseDownPosition - event->windowPos();
    
    const QSizeF minimumSize = QSize(20, 20);
    
    // Horizontal resize
    if (resizeLeft()) {
        const qreal width = qMax(minimumSize.width(), m_mouseDownGeometry.width() + difference.x());
        const qreal x = m_mouseDownGeometry.x() + (m_mouseDownGeometry.width() - width);

        m_rectangle->setX(x);
        m_rectangle->setWidth(width);
        setResizeBlocked(m_mouseDownGeometry.width() + difference.x() < minimumSize.width(), m_resizeHeightBlocked);
    } else if (resizeRight()) {
        const qreal width = qMax(minimumSize.width(), m_mouseDownGeometry.width() - difference.x());

        m_rectangle->setWidth(width);
        setResizeBlocked(m_mouseDownGeometry.width() - difference.x() < minimumSize.width(), m_resizeHeightBlocked);
    }

    // Vertical Resize
    if (resizeTop()) {
        const qreal height = qMax(minimumSize.height(), m_mouseDownGeometry.height() + difference.y());
        const qreal y = m_mouseDownGeometry.y() + (m_mouseDownGeometry.height() - height);

        m_rectangle->setY(y);
        m_rectangle->setHeight(height);
        setResizeBlocked(m_resizeWidthBlocked,
                            m_mouseDownGeometry.height() + difference.y() < minimumSize.height());
    } else if (resizeBottom()) {
        const qreal height = qMax(minimumSize.height(), m_mouseDownGeometry.height() - difference.y());

        m_rectangle->setHeight(qMax(height, minimumSize.height()));
        setResizeBlocked(m_resizeWidthBlocked,
                            m_mouseDownGeometry.height() - difference.y() < minimumSize.height());
    }

    event->accept();
}

void ResizeHandle::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();

    setResizeBlocked(false, false);
    Q_EMIT resizeBlockedChanged();
}

#include "moc_resizehandle.cpp"
