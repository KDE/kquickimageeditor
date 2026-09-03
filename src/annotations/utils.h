/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QImage>
#include <QMatrix4x4>
#include <QObject>
#include <QVector2D>
#include <QtMath>

/*!
 * \inqmlmodule org.kde.kquickimageeditor
 * \qmltype Utils
 */
class Utils : public QObject
{
    Q_OBJECT

public:
    Utils(QObject *parent = nullptr);

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

    template<typename OptTuple>
    static QImage shapeShadow(const OptTuple &traits, qreal devicePixelRatio = 1);

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
