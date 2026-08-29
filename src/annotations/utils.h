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

/*!
 * \inqmlmodule org.kde.kquickimageeditor
 * \qmltype Utils
 */
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

    /*!
     * \qmlmethod real Utils::dprRound(real value, real dpr)
     */
    Q_INVOKABLE constexpr static inline qreal dprRound(qreal value, qreal dpr) noexcept
    {
        // Using qRound because std::round isn't constexpr until C++23
        return dprRound<qreal>(value, dpr);
    }

    /*!
     * \qmlmethod point Utils::dprRound(point value, real dpr)
     */
    Q_INVOKABLE constexpr static inline QPointF dprRound(const QPointF &value, qreal dpr) noexcept
    {
        return {dprRound(value.x(), dpr), dprRound(value.y(), dpr)};
    }

    /*!
     * \qmlmethod vector2d Utils::dprRound(vector2d value, real dpr)
     */
    Q_INVOKABLE constexpr static inline QVector2D dprRound(const QVector2D &value, qreal dpr) noexcept
    {
        return {dprRound(value.x(), dpr), dprRound(value.y(), dpr)};
    }

    /*!
     * \qmlmethod rect Utils::rectScaled(rect rect, real factor)
     */
    Q_INVOKABLE constexpr static inline QRectF rectScaled(const QRectF &rect, qreal factor) noexcept
    {
        return {rect.topLeft() * factor, rect.size() * factor};
    }

    Q_INVOKABLE static inline QRectF rectNormalized(const QRectF &rect) noexcept
    {
        return rect.normalized();
    }

    Q_INVOKABLE static inline QRectF rectClipped(const QRectF &rect, const QRectF &clipRect) noexcept
    {
        if (rect == clipRect) {
            return rect;
        }
        auto newRect = rect;
        const auto &nClipRect = clipRect.normalized(); // normalize to make math easier
        if (rect.width() >= 0) {
            newRect.setLeft(std::max(rect.x(), nClipRect.x()));
            newRect.setRight(std::min(rect.right(), nClipRect.right()));
        } else {
            newRect.setLeft(std::min(rect.x(), nClipRect.right()));
            newRect.setRight(std::max(rect.right(), nClipRect.x()));
        }
        if (rect.height() >= 0) {
            newRect.setTop(std::max(rect.y(), nClipRect.y()));
            newRect.setBottom(std::min(rect.bottom(), nClipRect.bottom()));
        } else {
            newRect.setTop(std::min(rect.y(), nClipRect.bottom()));
            newRect.setBottom(std::max(rect.bottom(), nClipRect.y()));
        }
        return newRect;
    }

    Q_INVOKABLE static inline QRectF rectAspectRatioedForHandle(const QRectF &rect, qreal ratio, int edges) noexcept
    {
        if (ratio <= 0) {
            return rect;
        }
        const bool square = ratio == 1;
        const bool landscape = ratio > 1;
        const bool portrait = ratio < 1;
        // wants to adjust top/bottom only
        const bool vEdge = edges == Qt::TopEdge || edges == Qt::BottomEdge;
        // wants to adjust left/right only
        const bool hEdge = edges == Qt::LeftEdge || edges == Qt::RightEdge;
        auto pos = rect.topLeft();
        auto size = rect.size();
        if ((landscape && !vEdge) || (hEdge)) {
            size = {size.width(), std::abs(size.width()) / ratio * std::copysign(1.0, size.height())};
        } else if ((portrait && !hEdge) || (vEdge)) {
            size = {std::abs(size.height()) * ratio * std::copysign(1.0, size.width()), size.height()};
        } else if (square) {
            auto w = std::abs(std::sqrt(rect.width() * rect.height() / ratio));
            auto h = std::abs(std::sqrt(rect.width() * rect.height() * ratio));
            size = {w * std::copysign(1.0, size.width()), h * std::copysign(1.0, size.height())};
        }
        auto wdiff = size.width() - rect.width();
        auto hdiff = size.height() - rect.height();
        if (edges & Qt::LeftEdge) {
            pos.rx() -= wdiff;
        }
        if (edges & Qt::TopEdge) {
            pos.ry() -= hdiff;
        }
        if (vEdge) {
            pos.rx() -= wdiff / 2;
        } else if (hEdge) {
            pos.ry() -= hdiff / 2;
        }
        return {pos, size};
    }

    Q_INVOKABLE static inline QRectF rectAspectRatioed(const QRectF &rect, qreal ratio) noexcept
    {
        if (ratio <= 0) {
            return rect;
        }
        auto w = std::sqrt(rect.width() * rect.height() * ratio);
        auto h = std::sqrt(rect.width() * rect.height() / ratio);
        return {rect.x(), rect.y(), w, h};
    }

    /*!
     * \qmlmethod real Utils::clamp(real value, real min = infinity, real max = infinity)
     * Behaves like qBound, which behaves differently from std::clamp,
     * but uses the same argument order as std::clamp.
     */
    Q_INVOKABLE constexpr static inline qreal
    clamp(qreal value, qreal min = -std::numeric_limits<qreal>::infinity(), qreal max = std::numeric_limits<qreal>::infinity()) noexcept
    {
        // We don't use qBound or std::clamp because we don't want asserts.
        return std::max(min, std::min(value, max));
    }

    /*!
     * \qmlmethod real Utils::combinedScale(matrix4x4 matrix)
     */
    Q_INVOKABLE static inline qreal combinedScale(const QMatrix4x4 &matrix) noexcept
    {
        // Not constexpr until C++26
        return std::sqrt(std::pow(matrix(0, 0), 2) + std::pow(matrix(1, 0), 2) + std::pow(matrix(2, 0), 2) + //
                         std::pow(matrix(0, 1), 2) + std::pow(matrix(1, 1), 2) + std::pow(matrix(2, 1), 2));
    }

    static inline QImage shapeShadow(const Traits::OptTuple &traits, qreal devicePixelRatio = 1)
    {
        auto &shadowTrait = std::get<Traits::Shadow::Opt>(traits);
        if (!shadowTrait || !Traits::isVisible(traits)) {
            return QImage();
        }

        auto &geometryTrait = std::get<Traits::Geometry::Opt>(traits);
        auto &visualTrait = std::get<Traits::Visual::Opt>(traits);
        QImage shadow(std::ceil(visualTrait->rect.width() * devicePixelRatio), //
                      std::ceil(visualTrait->rect.height() * devicePixelRatio),
                      QImage::Format_ARGB32_Premultiplied);
        shadow.setDevicePixelRatio(devicePixelRatio); // also scales QPainter
        shadow.fill(Qt::transparent);
        QPainter p(&shadow);
        p.setRenderHint(QPainter::Antialiasing);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::NoBrush);
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
        const int radius = qRound(Traits::Shadow::radius * devicePixelRatio);
        // We only want black shadows with opacity, so we only need black and 8 bits of alpha.
        // If we don't do this, color emojis won't have black semi-transparent shadows.
        shadow.convertTo(QImage::Format_Alpha8);
        // Blur after converting to save CPU cycles.
        StackBlur::blur(shadow, radius, radius);
        return shadow;
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
        const auto radianZRotation = std::atan2(documentTransform(1,0), documentTransform(0,0));
        scaleTransform.rotateRadians(radianZRotation);
        const auto rotatedScale = scaleTransform.map(QPointF(xScale, yScale));
        matrix.scale(rotatedScale.x(), rotatedScale.y());
        return {{u"edges"_s, edges}, {u"matrix"_s, matrix}};
    }

    static inline QImage::Format formatForQPainter(QImage::Format format)
    {
        const auto pf = QImage::toPixelFormat(format);
        if (pf.colorModel() == QPixelFormat::Indexed) {
            return QImage::Format_RGB32;
        }
        const auto maxChannelSize = std::max({pf.redSize(), pf.greenSize(), pf.blueSize(), pf.blackSize(), pf.alphaSize()});
        const auto usesAlpha = pf.alphaUsage() == QPixelFormat::UsesAlpha;
        if (maxChannelSize > 8) {
            return usesAlpha ? QImage::Format_RGBA64_Premultiplied : QImage::Format_RGBX64;
        }
        const auto alphaAtEnd = pf.alphaPosition() == QPixelFormat::AtEnd;
        if (alphaAtEnd) {
            return usesAlpha ? QImage::Format_RGBA8888_Premultiplied : QImage::Format_RGBX8888;
        }
        return usesAlpha ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32;
    }

    // The value threshold used by qFuzzyIsNull and qFuzzyCompare
    template<std::floating_point T>
    static constexpr T fuzzyEpsilon()
    {
        if constexpr (std::same_as<T, float>) {
            // A negative power of 10 two orders of magnitude more than float epsilon.
            return 1e-5;
        } else if constexpr (std::same_as<T, double>) {
            // A negative power of 10 four orders of magnitude more than double epsilon.
            return 1e-12;
        }
    }

    Q_INVOKABLE static constexpr qreal fuzzyEpsilonF32()
    {
        return fuzzyEpsilon<float>();
    }

    // The smallest value increment for a 32-bit float.
    Q_INVOKABLE static constexpr qreal epsilonF32()
    {
        return std::numeric_limits<float>::epsilon();
    }

    Q_INVOKABLE static constexpr bool fuzzyIsNullF32(qreal v)
    {
        return v <= fuzzyEpsilon<float>() && v >= -fuzzyEpsilon<float>();
    }

    Q_INVOKABLE static constexpr bool fuzzyCompareF32(qreal v1, qreal v2)
    {
        return Utils::fuzzyIsNullF32(std::max(v1, v2) - std::min(v1, v2));
    }

    // QQuickMatrix4x4 does not have isIdentity
    Q_INVOKABLE static inline bool isIdentityMatrix4x4(const QMatrix4x4 &matrix)
    {
        return matrix.isIdentity();
    }

    /*!
    \qmlmethod matrix4x4 Utils::brightnessMatrix(real brightness)

    \brief Get a matrix4x4 with a brightness transformation.

    If you have an existing matrix you want to modify, multiply it with this
    matrix. The order in which you multiply matters.

    In QML, \c{brightnessMatrix.times(otherMatrix)} will apply an absolute
    brightness change to the other matrix, except it will still be multiplied
    by the other matrix's global divisor (QML: \c {matrix.m44}, C++: \c{matrix(3,3)}).
    \c{otherMatrix.times(brightnessMatrix)} will apply a brightness change that
    is affected by the other matrix's existing scale, rotation and perspective
    operations, but not the other matrix's global divisor. Read the C++ code
    of \c{QMatrix4x4 operator*(const QMatrix4x4& m1, const QMatrix4x4& m2)}
    to understand all the details. In QML, \c{m1.times(m2)} uses matrices in
    the same order as \c{QMatrix4x4 operator*(m1, m2)}.

    \a brightness An absolute brightness offset. You typically wouldn't use
    values outside of [-1,1] so that you don't stray too far outside the range
    of valid color values. However, there is nothing stopping you from doing so.
    Brightness is not clamped because the brightness change may be combined with
    scale, rotation or perspective operations. Either way, the effect applying
    the brightness change will need to clamp color channel values unless you are
    using a floating point image format where values outside of [0,1] are valid.
    Values within [-1e-5, 1e-5] and non-finite values will do nothing.
    */
    Q_INVOKABLE static inline QMatrix4x4 brightnessMatrix(qreal brightness)
    {
        if (!std::isfinite(brightness) || Utils::fuzzyIsNullF32(brightness)) {
            return {};
        }
        QMatrix4x4 m;
        // Will be affected by existing scale/rotation/perspective operations
        m.translate(brightness, brightness, brightness);
        return m;
    }

    /*!
    \qmlmethod matrix4x4 Utils::contrastMatrix(real contrast)

    \brief Get a matrix4x4 with a contrast transformation.

    If you have an existing matrix you want to modify, multiply it with this
    matrix. The order in which you multiply matters in much the same way as
    \l{Utils::brightnessMatrix}{brightnessMatrix} for translations
    caused by the contrast change. Either way, the scales of this matrix and
    another matrix will be multiplied with each other. If there are rotation
    and perspective operations in the other matrix, those will affect the scales
    from this matrix. Read the C++ code of \c{QMatrix4x4 operator*(m1, m2)}
    to understand all the details.

    \a{contrast} A multiplier and offset that changes contrast.
    A typical value range might be (0,2] or (0,4], but there is nothing stopping
    you from using values outside that range. Values greater than 1 increase
    contrast. Values less than 1 decrease contrast. Values less than 0 will give
    the same result as 0. Values within 1±1e-5 and non-finite values will do
    nothing. Values greater than 1 may technically be able to change contrast up
    to the maximum value of a 32-bit float, but you probably won't notice any
    difference above 55.5.
    */
    Q_INVOKABLE static inline QMatrix4x4 contrastMatrix(qreal contrast)
    {
        if (!std::isfinite(contrast) || Utils::fuzzyCompareF32(contrast, 1.0)) {
            return {};
        }
        // Prevent actually going to 0 so that the matrix can be inverted.
        contrast = std::copysign(std::max(std::abs(contrast), Utils::fuzzyEpsilonF32()), contrast);
        float offset = (1.0f - contrast) * 0.5f;
        // Will be affected by existing scale/rotation/perspective operations,
        // so translate before scaling.
        QMatrix4x4 m;
        m.translate(offset, offset, offset);
        m.scale(contrast);
        return m;
    }

    /*!
    \qmlmethod matrix4x4 Utils::saturationMatrix(real saturation)

    \brief Get a matrix4x4 with a saturation transformation.

    If you have an existing matrix you want to modify, multiply it with this
    matrix. The order in which you multiply matters in much the same way as
    \l{Utils::contrastMatrix}{contrastMatrix}. Read the C++ code
    of \c{QMatrix4x4 operator*(m1, m2)} to understand all the details.

    \a{saturation} A multiplier that changes saturation.
    A typical value range might be (0,2] or (0,4], but there is nothing stopping
    you from using values outside that range. Values greater than 1 increase
    saturation. Values less than 1 decrease saturation. Values less than 0 will
    invert the colors and apply saturation to the inverted colors. Values within
    1±1e-5 and non-finite values will do nothing. Values greater than 1 may
    technically be able to change saturation up to the maximum value of a 32-bit
    float, but you probably won't notice any difference above 50.
    */
    Q_INVOKABLE static inline QMatrix4x4 saturationMatrix(qreal saturation)
    {
        if (!std::isfinite(saturation) || Utils::fuzzyCompareF32(saturation, 1.0)) {
            return {};
        }
        // Prevent actually going to 0 so that the matrix can be inverted.
        saturation = std::copysign(std::max(std::abs(saturation), Utils::fuzzyEpsilonF32()), saturation);
        QVector3D eye(0, 0, 0);
        QVector3D center(1, 1, 1);
        // At least one of `up` needs to be 1.
        // At least one other of `up` needs to be 0.
        QVector3D up(0, 0, 1);
        QMatrix4x4 lookAtMat;
        // Look along the grayscale axis.
        lookAtMat.lookAt(eye, center, up);
        // If you don't transpose or invert, the colors will be wrong.
        QMatrix4x4 m = lookAtMat.transposed();
        // Scale grayness.
        m.scale(saturation, saturation, 1);
        // Go back to RGB.
        return m * lookAtMat;
    }

    /*!
    \qmlmethod matrix4x4 Utils::relativeLuminanceMatrix(real rY, real gY, real bY)

    \brief Get a matrix4x4 with relative luminance weights.
    This may be necessary for some effects like saturation.

    If you have an existing matrix you want to modify, add it to this matrix.
    The order in which you add does not matter.
    */
    Q_INVOKABLE static inline QMatrix4x4 relativeLuminanceMatrix(qreal rY, qreal gY, qreal bY)
    {
        // clang-format off
        return QMatrix4x4(rY, gY, bY, 0,
                          rY, gY, bY, 0,
                          rY, gY, bY, 0,
                           0,  0,  0, 1);
        // clang-format on
    }
};
