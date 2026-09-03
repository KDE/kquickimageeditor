/* SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "utils.h"

#include "annotationdocument.h"
#include "annotationviewport.h"

#include <QCoreApplication>
#include <QQmlEngine>
#include <QQuickWindow>
#include <qqmlregistration.h>

using namespace Qt::StringLiterals;

class QmlUtils : public Utils
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(Utils)

public:
    QmlUtils(QObject *parent = nullptr);

    static QmlUtils *create(QQmlEngine *engine, QJSEngine *)
    {
        static const auto inst = new QmlUtils(QCoreApplication::instance());
        Q_ASSERT(inst);
        Q_ASSERT(inst->thread() == engine->thread());
        QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
        return inst;
    }

    /*!
     * \qmlmethod point Utils::sceneToDocumentPoint(point point, AnnotationViewport viewport)
     */
    Q_INVOKABLE static inline QPointF sceneToDocumentPoint(QPointF point, AnnotationViewport *viewport)
    {
        auto p = dprRound(point, viewport->window()->devicePixelRatio());
        p = viewport->mapFromItem(nullptr, p);
        p = viewport->document()->inputTransform().map(p);
        return p + viewport->viewportRect().topLeft();
    }

    /*!
     * \qmlmethod Object Utils::handleResizeProperties(real dx, real dy, int edges, AnnotationDocument document)
     * Get a QVariantMap of properties for resizing an item in response to the
     * movement of handles.
     *
     * The map contains the effective handle edges so movement
     * can be tracked properly and the QMatrix4x4 to be used
     * that are positioned along the edges of the item's bounding box.
     *
     * The \a dx should be the X axis difference between 2 points in document coordinates.
     *
     * The \a dy should be the Y axis difference between 2 points in document coordinates.
     *
     * The \a edges should be the bounding box edges a handle touches.
     *
     * The \a document should be the AnnotationDocument with the item being transformed.
     */
    Q_INVOKABLE static inline QVariantMap handleResizeProperties(qreal dx, qreal dy, int edges, AnnotationDocument *document)
    {
        Q_ASSERT(document != nullptr);
        // The document can be rotated
        const auto &documentTransform = document->transform();
        auto delta = documentTransform.map(QPointF{dx, dy});
        if ((!std::isfinite(delta.x()) || delta.x() == 0) && (!std::isfinite(delta.y()) || delta.y() == 0)) {
            return {};
        }

        const auto pathSize = [&]() -> QSizeF {
            const auto rect = document->selectedItemWrapper()->geometryPath().boundingRect();
            const auto size = documentTransform.map(QPointF{rect.width(), rect.height()});
            return {std::abs(size.x()), std::abs(size.y())};
        }();
        const bool leftEdge = (edges & Qt::LeftEdge) != 0;
        const bool rightEdge = (edges & Qt::RightEdge) != 0;
        const bool topEdge = (edges & Qt::TopEdge) != 0;
        const bool bottomEdge = (edges & Qt::BottomEdge) != 0;

        // Assume that the scale transformation is centered on the path bounds.
        qreal xScale = 1;
        qreal yScale = 1;
        if (leftEdge && !rightEdge) { // move left edge
            xScale = (pathSize.width() - delta.x()) / std::max(0.001, pathSize.width());
            if (xScale < 0) {
                // This happens when the user tries to resize to a width < 0.
                // From now on the handle will behave like the opposite one.
                edges = (edges & ~Qt::LeftEdge) | Qt::RightEdge;
            }
            // Recalculate based from the size change so when size goes to zero going further down won't move the shape
            delta.rx() = (pathSize.width() - pathSize.width() * xScale) / 2;
        } else if (rightEdge && !leftEdge) { // move right edge
            xScale = (pathSize.width() + delta.x()) / std::max(0.001, pathSize.width());
            if (xScale < 0) {
                edges = (edges & ~Qt::RightEdge) | Qt::LeftEdge;
            }
            delta.rx() = -(pathSize.width() - pathSize.width() * xScale) / 2;
        } else {
            xScale = 1;
            delta.rx() = 0;
        }
        if (!std::isfinite(xScale) || xScale == 0) {
            xScale = 1;
        }
        if (topEdge && !bottomEdge) { // move top edge
            yScale = (pathSize.height() - delta.y()) / std::max(0.001, pathSize.height());
            if (yScale < 0) {
                edges = (edges & ~Qt::TopEdge) | Qt::BottomEdge;
            }
            delta.ry() = (pathSize.height() - pathSize.height() * yScale) / 2;
        } else if (bottomEdge && !topEdge) { // move bottom edge
            yScale = (pathSize.height() + delta.y()) / std::max(0.001, pathSize.height());
            if (yScale < 0) {
                edges = (edges & ~Qt::BottomEdge) | Qt::TopEdge;
            }
            delta.ry() = -(pathSize.height() - pathSize.height() * yScale) / 2;
        } else {
            yScale = 1;
            delta.ry() = 0;
        }
        if (!std::isfinite(yScale) || yScale == 0) {
            yScale = 1;
        }
        // The matrix to be sent as an argument.
        QMatrix4x4 matrix;
        // Put the translation first to avoid scaling it
        delta = documentTransform.inverted().map(delta);
        matrix.translate(delta.x(), delta.y());
        QTransform scaleTransform;
        const auto radianZRotation = std::atan2(documentTransform(1, 0), documentTransform(0, 0));
        scaleTransform.rotateRadians(radianZRotation);
        const auto rotatedScale = scaleTransform.map(QPointF(xScale, yScale));
        matrix.scale(rotatedScale.x(), rotatedScale.y());
        return {{u"edges"_s, edges}, {u"matrix"_s, matrix}};
    }
};
