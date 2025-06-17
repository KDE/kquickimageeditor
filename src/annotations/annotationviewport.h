/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "annotationdocument.h"
#include <QMatrix4x4>
#include <QPainterPath>
#include <QQuickItem>
#include <qqmlregistration.h>
#include "kquickimageeditor_export.h"

class AnnotationViewportPrivate;
class QPainter;

/**
 * This is a QML item which paints its correspondent AnnotationDocument or a sub-part of it,
 * depending from viewportRect and zoom.
 * This is also managing all the input which will add the annotations.
 */
class KQUICKIMAGEEDITOR_EXPORT AnnotationViewport : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QRectF viewportRect READ viewportRect WRITE setViewportRect NOTIFY viewportRectChanged)
    Q_PROPERTY(AnnotationDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(QPointF hoverPosition READ hoverPosition NOTIFY hoverPositionChanged)
    Q_PROPERTY(bool hovered READ isHovered NOTIFY hoveredChanged)
    Q_PROPERTY(QPointF pressPosition READ pressPosition NOTIFY pressPositionChanged)
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged)
    Q_PROPERTY(bool anyPressed READ isAnyPressed NOTIFY anyPressedChanged)
    Q_PROPERTY(QPainterPath hoveredMousePath READ hoveredMousePath NOTIFY hoveredMousePathChanged)

public:
    explicit AnnotationViewport(QQuickItem *parent = nullptr);
    ~AnnotationViewport() noexcept override;

    QRectF viewportRect() const;
    void setViewportRect(const QRectF &rect);

    AnnotationDocument *document() const;
    void setDocument(AnnotationDocument *doc);

    QPointF hoverPosition() const;

    bool isHovered() const;

    QPointF pressPosition() const;

    bool isPressed() const;

    bool isAnyPressed() const;

    /// Hovered mouse interaction path in non-transformed logical document coordinates
    QPainterPath hoveredMousePath() const;

Q_SIGNALS:
    void viewportRectChanged();
    void documentChanged();
    void hoverPositionChanged();
    void hoveredChanged();
    void pressPositionChanged();
    void pressedChanged();
    void anyPressedChanged();
    void hoveredMousePathChanged();

protected:
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void itemChange(ItemChange, const ItemChangeData &) override;

private:
    std::unique_ptr<AnnotationViewportPrivate> d;
};
