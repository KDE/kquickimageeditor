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

/*!
 * \inqmlmodule org.kde.kquickimageeditor
 * \qmltype AnnotationViewport
 * \brief A QML item which paints its correspondent AnnotationDocument or a sub-part of it,
 * depending from viewportRect and zoom.
 *
 * This also manages all the input for adding the annotations.
 */
class KQUICKIMAGEEDITOR_EXPORT AnnotationViewport : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    /*!
     * \qmlproperty rect AnnotationViewport::viewportRect
     */
    Q_PROPERTY(QRectF viewportRect READ viewportRect WRITE setViewportRect NOTIFY viewportRectChanged)
    /*!
     * \qmlproperty AnnotationDocument AnnotationViewport::document
     */
    Q_PROPERTY(AnnotationDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    /*!
     * \qmlproperty point AnnotationViewport::hoverPosition
     */
    Q_PROPERTY(QPointF hoverPosition READ hoverPosition NOTIFY hoverPositionChanged)
    /*!
     * \qmlproperty bool AnnotationViewport::hovered
     */
    Q_PROPERTY(bool hovered READ isHovered NOTIFY hoveredChanged)
    /*!
     * \qmlproperty point AnnotationViewport::pressPosition
     */
    Q_PROPERTY(QPointF pressPosition READ pressPosition NOTIFY pressPositionChanged)
    /*!
     * \qmlproperty bool AnnotationViewport::pressed
     */
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged)
    /*!
     * \qmlproperty bool AnnotationViewport::anyPressed
     */
    Q_PROPERTY(bool anyPressed READ isAnyPressed NOTIFY anyPressedChanged)
    /*!
     * \qmlproperty QPainterPath AnnotationViewport::hoveredMousePath
     */
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
