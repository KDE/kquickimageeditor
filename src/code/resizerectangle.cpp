/*
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "resizerectangle.h"

#include <QDebug>
#include <QSGFlatColorMaterial>
#include <QSGGeometry>
#include <QSGGeometryNode>

ResizeRectangle::ResizeRectangle(QQuickItem *parent)
    : QQuickItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setFlag(ItemHasContents);
}

void ResizeRectangle::componentComplete()
{
    QQuickItem::componentComplete();
    QQmlEngine *engine = qmlEngine(this);
    m_handleComponent = new QQmlComponent(engine, QUrl(QStringLiteral("qrc:/BasicResizeHandle.qml")));

    auto handleItem = qobject_cast<QQuickItem *>(m_handleComponent->create());
    qDebug() << handleItem;
    m_handleBottomLeft = qobject_cast<ResizeHandle *>(handleItem);
    m_handleBottomLeft->setParent(this);
    m_handleBottomLeft->setParentItem(this);
    m_handleBottomLeft->setResizeCorner(ResizeHandle::BottomLeft);
    m_handleBottomLeft->setX(m_insideX - 5);
    m_handleBottomLeft->setY(m_insideY + m_insideHeight - 5);
    m_handleBottomLeft->setRectangle(this);

    handleItem = qobject_cast<QQuickItem *>(m_handleComponent->create());
    m_handleBottomRight = qobject_cast<ResizeHandle *>(handleItem);
    m_handleBottomRight->setParent(this);
    m_handleBottomRight->setParentItem(this);
    m_handleBottomRight->setResizeCorner(ResizeHandle::BottomRight);
    m_handleBottomRight->setX(m_insideX + m_insideWidth - 5);
    m_handleBottomRight->setY(m_insideY + m_insideHeight - 5);
    m_handleBottomRight->setRectangle(this);

    handleItem = qobject_cast<QQuickItem *>(m_handleComponent->create());
    m_handleTopLeft = qobject_cast<ResizeHandle *>(handleItem);
    m_handleTopLeft->setParent(this);
    m_handleTopLeft->setParentItem(this);
    m_handleTopLeft->setResizeCorner(ResizeHandle::TopLeft);
    m_handleTopLeft->setX(m_insideX - 5);
    m_handleTopLeft->setY(m_insideY - 5);
    m_handleTopLeft->setRectangle(this);

    handleItem = qobject_cast<QQuickItem *>(m_handleComponent->create());
    m_handleTopRight = qobject_cast<ResizeHandle *>(handleItem);
    m_handleTopRight->setParent(this);
    m_handleTopRight->setParentItem(this);
    m_handleTopRight->setResizeCorner(ResizeHandle::TopRight);
    m_handleTopRight->setX(m_insideX + m_insideWidth - 5);
    m_handleTopRight->setY(m_insideY - 5);
    m_handleTopRight->setRectangle(this);
}

void ResizeRectangle::updateHandles()
{
    if (isComponentComplete()) {
        m_handleTopRight->setX(m_insideX + m_insideWidth - 5);
        m_handleTopRight->setY(m_insideY - 5);
        m_handleTopLeft->setX(m_insideX - 5);
        m_handleTopLeft->setY(m_insideY - 5);
        m_handleBottomRight->setX(m_insideX + m_insideWidth - 5);
        m_handleBottomRight->setY(m_insideY + m_insideHeight - 5);
        m_handleBottomLeft->setX(m_insideX - 5);
        m_handleBottomLeft->setY(m_insideY + m_insideHeight - 5);
    }
}

qreal ResizeRectangle::insideX() const
{
    return m_insideX;
}

void ResizeRectangle::setInsideX(qreal x)
{
    x = qBound(0.0, x, this->width() - m_insideWidth);
    if (m_insideX == x) {
        return;
    }
    m_insideX = x;
    updateHandles();
    Q_EMIT insideXChanged();
    update();
}

qreal ResizeRectangle::insideY() const
{
    return m_insideY;
}

void ResizeRectangle::setInsideY(qreal y)
{
    y = qBound(0.0, y, this->height() - m_insideHeight);
    if (m_insideY == y) {
        return;
    }
    m_insideY = y;
    updateHandles();
    Q_EMIT insideYChanged();
    update();
}

qreal ResizeRectangle::insideWidth() const
{
    return m_insideWidth;
}

void ResizeRectangle::setInsideWidth(qreal width)
{
    width = qMin(width, this->width());
    if (m_insideWidth == width) {
        return;
    }
    m_insideWidth = width;
    updateHandles();
    Q_EMIT insideWidthChanged();
    update();
}

qreal ResizeRectangle::insideHeight() const
{
    return m_insideHeight;
}

void ResizeRectangle::setInsideHeight(qreal height)
{
    height = qMin(height, this->height());
    if (m_insideHeight == height) {
        return;
    }
    m_insideHeight = height;
    updateHandles();
    Q_EMIT insideHeightChanged();
    update();
}

QSGNode *ResizeRectangle::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
    QSGGeometryNode *node = nullptr;
    QSGGeometry *geometry = nullptr;

    const int vertexCount = 12;
    const int indexCount = 8 * 3;
    if (!oldNode) {
        node = new QSGGeometryNode;
        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount, indexCount);
        geometry->setIndexDataPattern(QSGGeometry::StaticPattern);
        geometry->setDrawingMode(GL_TRIANGLES);
        node->setGeometry(geometry);
        node->setFlag(QSGNode::OwnsGeometry);

        QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
        material->setColor(QColor(0, 0, 0, 70));
        node->setMaterial(material);
        node->setFlag(QSGNode::OwnsMaterial);
    } else {
        node = static_cast<QSGGeometryNode *>(oldNode);
        geometry = node->geometry();
        geometry->allocate(vertexCount, indexCount);
    }

    QSGGeometry::Point2D *points = geometry->vertexDataAsPoint2D();
    points[0].set(0, 0);
    points[1].set(0, height());
    points[2].set(m_insideX, 0);
    points[3].set(m_insideX, height());

    points[4].set(m_insideX + m_insideWidth, 0);
    points[5].set(m_insideX + m_insideWidth, height());
    points[6].set(width(), 0);
    points[7].set(width(), height());

    points[8].set(m_insideX, m_insideY);
    points[9].set(m_insideX + m_insideWidth, m_insideY);
    points[10].set(m_insideX + m_insideWidth, m_insideY + m_insideHeight);
    points[11].set(m_insideX, m_insideY + m_insideHeight);

    quint16 *indices = geometry->indexDataAsUShort();
    // left
    indices[0 + 0] = 0;
    indices[0 + 1] = 1;
    indices[0 + 2] = 2;

    indices[3 + 0] = 3;
    indices[3 + 1] = 1;
    indices[3 + 2] = 2;

    // right
    indices[6 + 0] = 4;
    indices[6 + 1] = 5;
    indices[6 + 2] = 6;

    indices[9 + 0] = 7;
    indices[9 + 1] = 5;
    indices[9 + 2] = 6;

    // top
    indices[12 + 0] = 2;
    indices[12 + 1] = 8;
    indices[12 + 2] = 4;

    indices[15 + 0] = 9;
    indices[15 + 1] = 8;
    indices[15 + 2] = 4;

    // bottom
    indices[18 + 0] = 3;
    indices[18 + 1] = 11;
    indices[18 + 2] = 10;

    indices[21 + 0] = 3;
    indices[21 + 1] = 5;
    indices[21 + 2] = 10;

    geometry->markIndexDataDirty();
    geometry->markVertexDataDirty();
    node->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
    return node;
}

void ResizeRectangle::mouseReleaseEvent(QMouseEvent *event)
{
    m_mouseClickedOnRectangle = false;
    event->accept();
}

void ResizeRectangle::mousePressEvent(QMouseEvent *event)
{
    m_mouseDownPosition = event->pos();
    m_mouseDownGeometry = QPointF(m_insideX, m_insideY);
    if (m_mouseDownPosition.x() >= m_insideX && m_mouseDownPosition.x() <= m_insideX + m_insideWidth && m_mouseDownPosition.y() >= m_insideY
        && m_mouseDownPosition.y() <= m_insideY + m_insideHeight) {
        m_mouseClickedOnRectangle = true;
    }
    event->accept();
}

void ResizeRectangle::mouseMoveEvent(QMouseEvent *event)
{
    if (m_mouseClickedOnRectangle) {
        const QPointF difference = m_mouseDownPosition - event->pos();
        const qreal x = m_mouseDownGeometry.x() - difference.x();
        const qreal y = m_mouseDownGeometry.y() - difference.y();
        setInsideX(x);
        setInsideY(y);
    }
}

void ResizeRectangle::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_EMIT acceptSize();
    event->accept();
}

#include "moc_resizerectangle.cpp"
