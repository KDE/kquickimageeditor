/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "annotationdocument.h"
#include "annotationviewport.h"
#include "stackblur.h"
#include "traits.h"

#include <QCoreApplication>
#include <QImage>
#include <QPainter>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QVector2D>
#include <QtMath>
#include <qqmlregistration.h>

using namespace Qt::StringLiterals;

class Utils : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
public:
    Utils(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    static Utils *create(QQmlEngine *engine, QJSEngine *)
    {
        static const auto inst = new Utils(QCoreApplication::instance());
        Q_ASSERT(inst);
        Q_ASSERT(inst->thread() == engine->thread());
        QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
        return inst;
    }

    template<typename T>
    constexpr static inline T dprRound(T value, qreal dpr) noexcept
    {
        // Using qRound because std::round isn't constexpr until C++23
        return qRound(value * dpr) / dpr;
    }

    Q_INVOKABLE constexpr static inline qreal dprRound(qreal value, qreal dpr) noexcept
    {
        // Using qRound because std::round isn't constexpr until C++23
        return dprRound<qreal>(value, dpr);
    }

    Q_INVOKABLE constexpr static inline QPointF dprRound(const QPointF &value, qreal dpr) noexcept
    {
        return {dprRound(value.x(), dpr), dprRound(value.y(), dpr)};
    }

    Q_INVOKABLE constexpr static inline QVector2D dprRound(const QVector2D &value, qreal dpr) noexcept
    {
        return {dprRound(value.x(), dpr), dprRound(value.y(), dpr)};
    }

    Q_INVOKABLE constexpr static inline QRectF rectScaled(const QRectF &rect, qreal factor) noexcept
    {
        return {rect.topLeft() * factor, rect.size() * factor};
    }

    // Behaves like qBound, which behaves differently from std::clamp,
    // but uses the same argument order as std::clamp.
    Q_INVOKABLE constexpr static inline qreal
    clamp(qreal value, qreal min = -std::numeric_limits<qreal>::infinity(), qreal max = std::numeric_limits<qreal>::infinity()) noexcept
    {
        // We don't use qBound or std::clamp because we don't want asserts.
        return std::max(min, std::min(value, max));
    }

    Q_INVOKABLE static inline qreal combinedScale(const QMatrix4x4 &matrix) noexcept
    {
        // Not constexpr until C++26
        return std::sqrt(std::pow(matrix(0, 0), 2) + std::pow(matrix(1, 0), 2) + std::pow(matrix(2, 0), 2) + //
                         std::pow(matrix(0, 1), 2) + std::pow(matrix(1, 1), 2) + std::pow(matrix(2, 1), 2));
    }

    static inline QImage shapeShadow(const Traits::OptTuple &traits, qreal devicePixelRatio = 1)
    {
        auto &shadowTrait = std::get<Traits::Shadow::Opt>(traits);
        if (!Traits::isVisible(traits) || !shadowTrait) {
            return QImage();
        }

        auto &geometryTrait = std::get<Traits::Geometry::Opt>(traits);
        auto &visualTrait = std::get<Traits::Visual::Opt>(traits);
        QImage shadow(visualTrait->rect.size().toSize() * devicePixelRatio, QImage::Format_RGBA8888_Premultiplied);
        shadow.fill(Qt::transparent);
        QPainter p(&shadow);
        p.setRenderHint(QPainter::Antialiasing);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::NoBrush);
        p.scale(devicePixelRatio, devicePixelRatio);
        p.translate(-visualTrait->rect.topLeft() //
                    + QPointF{Traits::Shadow::xOffset, Traits::Shadow::yOffset});

        static constexpr auto alpha = 0.5;
        // Convenience var so we don't keep multiplying alpha by 255.
        static constexpr uint8_t alpha8bit = alpha * 255;

        auto &fillTrait = std::get<Traits::Fill::Opt>(traits);
        auto &strokeTrait = std::get<Traits::Stroke::Opt>(traits);
        auto *fillBrush = fillTrait && fillTrait->isValid() //
            ? std::get_if<Traits::Fill::Brush>(&fillTrait.value())
            : nullptr;
        bool hasStroke = strokeTrait && strokeTrait->isValid();
        // No need to draw fill and stroke separately if they're both opaque
        if (fillBrush && hasStroke && fillBrush->isOpaque() && strokeTrait->pen.brush().isOpaque()) {
            p.setBrush(QColor(0, 0, 0, alpha8bit));
            p.drawPath((strokeTrait->path | geometryTrait->path).simplified());
        } else {
            if (fillBrush) {
                p.setBrush(QColor(0, 0, 0, std::ceil(alpha8bit * fillBrush->color().alphaF())));
                p.drawPath(geometryTrait->path);
            }
            if (strokeTrait) {
                p.setBrush(QColor(0, 0, 0, std::ceil(alpha8bit * strokeTrait->pen.color().alphaF())));
                p.drawPath(strokeTrait->path);
            }
        }

        auto &textTrait = std::get<Traits::Text::Opt>(traits);
        // No need to paint text/number shadow if fill is opaque.
        if ((!fillTrait || (fillBrush && !fillBrush->isOpaque())) && textTrait) {
            p.setFont(textTrait->font);
            p.setBrush(Qt::NoBrush);
            p.setPen(Qt::black);
            // Color emojis don't get semi-transparent shadows with a semi-transparent pen.
            // setOpacity disables sub-pixel text antialiasing, but we don't need sub-pixel AA here.
            p.setOpacity(alpha * textTrait->brush.color().alphaF());
            p.drawText(geometryTrait->path.boundingRect(), textTrait->textFlags(), textTrait->text());
        }
        p.end();
        const qreal sigma = Traits::Shadow::radius * devicePixelRatio * 6;
        const int kernelSize = (int)std::round(sigma + 1) | 1;
        // Do this before converting to Alpha8 because stackBlur gets distorted with Alpha8.
        StackBlur::blur(shadow, {kernelSize, kernelSize});
        // We only want black shadows with opacity, so we only need black and 8 bits of alpha.
        // If we don't do this, color emojis won't have black semi-transparent shadows.
        shadow.convertTo(QImage::Format_Alpha8);
        return shadow;
    }

    Q_INVOKABLE static inline QPointF sceneToDocumentPoint(QPointF point, AnnotationViewport *viewport)
    {
        auto p = dprRound(point, viewport->window()->devicePixelRatio());
        p = viewport->mapFromItem(nullptr, p);
        p = viewport->document()->inputTransform().map(p);
        return p + viewport->viewportRect().topLeft();
    }

    /**
     * Get a QVariantMap of properties for resizing an item in response to the
     * movement of handles. The map contains the effective handle edges so movement
     * can be tracked properly and the QMatrix4x4 to be used.
     * that are positioned along the edges of the item's bounding box.
     * `dx` should be the X axis difference between 2 points in document coordinates.
     * `dy` should be the Y axis difference between 2 points in document coordinates.
     * `edges` should be the bounding box edges a handle touches.
     * `document` should be the AnnotationDocument with the item being transformed.
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
        const auto radianZRotation = std::atan2(documentTransform(1,0), documentTransform(0,0));
        scaleTransform.rotateRadians(radianZRotation);
        const auto rotatedScale = scaleTransform.map(QPointF(xScale, yScale));
        matrix.scale(rotatedScale.x(), rotatedScale.y());
        return {{u"edges"_s, edges}, {u"matrix"_s, matrix}};
    }
};
