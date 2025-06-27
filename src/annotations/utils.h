/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "stackblur.h"
#include "traits.h"

#include <QCoreApplication>
#include <QImage>
#include <QPainter>
#include <QQmlEngine>
#include <QtMath>
#include <QVector2D>
#include <qqmlregistration.h>


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
};
