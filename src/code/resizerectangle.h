/*
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "resizehandle.h"

#include <QQuickItem>
#include <QQmlComponent>

class ResizeRectangle : public QQuickItem
{
    Q_OBJECT
    
    Q_PROPERTY(qreal insideX READ insideX WRITE setInsideX NOTIFY insideXChanged)
    Q_PROPERTY(qreal insideY READ insideY WRITE setInsideY NOTIFY insideYChanged)
    Q_PROPERTY(qreal insideWidth READ insideWidth WRITE setInsideWidth NOTIFY insideWidthChanged)
    Q_PROPERTY(qreal insideHeight READ insideHeight WRITE setInsideHeight NOTIFY insideHeightChanged)
    
public:
    ResizeRectangle(QQuickItem *parent = nullptr);
    ~ResizeRectangle() = default;
    
    qreal insideX() const;
    void setInsideX(const qreal x);
    qreal insideY() const;
    void setInsideY(const qreal y);
    qreal insideWidth() const;
    void setInsideWidth(const qreal width);
    qreal insideHeight() const;
    void setInsideHeight(const qreal height);
    
    virtual QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    
protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent * event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    virtual void componentComplete() override;
    
Q_SIGNALS:
    /// Double click event signal
    void acceptSize();
    void insideXChanged();
    void insideYChanged();
    void insideWidthChanged();
    void insideHeightChanged();
    void handleComponentChanged();
    
private:
    void updateHandles();

    qreal m_insideX;
    qreal m_insideY;
    qreal m_insideWidth;
    qreal m_insideHeight;
    QPointF m_mouseDownPosition;
    QPointF m_mouseDownGeometry;
    bool m_mouseClickedOnRectangle = false;
    QQmlComponent *m_handleComponent = nullptr;
    ResizeHandle *m_handleBottomLeft = nullptr;
    ResizeHandle *m_handleBottomRight = nullptr;
    ResizeHandle *m_handleTopLeft = nullptr;
    ResizeHandle *m_handleTopRight = nullptr;
};
