/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QColorSpace>
#include <QImage>
#include <QMatrix4x4>
#include <QObject>
#include <QThread>
#include <QtConcurrent/QtConcurrentMap>
#include <QVector2D>
#include <QtMath>

namespace UtilsNS
{
// If at some point in the future we need to do something with relative luminance,
// here are sRGB relative luminance (Y) weights:
// rY = 0.212655f, gY = 0.715158f, bY = 0.072187f
// rgbY_hypotenuse = 0.749615f

template<typename T, typename... Ts>
concept IsAnyOf = std::disjunction_v<std::is_same<T, Ts>...>;

template<typename T>
concept ChannelType = IsAnyOf<T, uint8_t, uint16_t, qfloat16, float>;

template<size_t Channels>
concept ChannelCount = Channels == 1 || Channels == 4;

template<auto V>
concept IsPow2 = (std::is_integral_v<decltype(V)> || std::is_enum_v<decltype(V)>) && (std::has_single_bit(static_cast<size_t>(V)));

// Used to check if lambdas/functions are compatible with other functions.
// Does not do strict type checking.
template<typename Func, typename Ret, typename... Args>
concept CompatibleSignature = std::invocable<Func, Args...> // check args
    && std::convertible_to<std::invoke_result_t<Func, Args...>, Ret>; // check return type

template<size_t Alignment, size_t RequiredAlignment>
concept IsSufficientlyAligned = IsPow2<Alignment>
    && IsPow2<RequiredAlignment>
    // A cheaper modulo that only works when the right side is a power of 2.
    && (Alignment & (RequiredAlignment - 1)) == 0;

// Helps you split an extent into chunks based on the size of the chunks.
template<std::integral T>
constexpr auto extentToChunks(T extent, T chunk)
{
    return (extent + chunk - 1) / chunk;
}

// Allows us to create a list of tasks with the option to have slight variations
// from other lists of tasks using the pushBackFunction argument.
template<std::ranges::contiguous_range Container, std::integral Extent, typename Function>
    requires CompatibleSignature<Function, void, Container &, Extent, Extent>
inline Container makeTasks(Extent extent, Extent chunkSize, Function pushBackFunction)
{
    // Thread count can change over time, so get a new thread count every time.
    const Extent threadCount = QThread::idealThreadCount();
    const Extent extentPerThread = extentToChunks(extent, chunkSize * threadCount);
    Container tasks;
    for (Extent i = 0; i < threadCount; ++i) {
        const Extent start = i * chunkSize * extentPerThread;
        const Extent end = std::min(start + chunkSize * extentPerThread, extent);
        if (start < end) {
            pushBackFunction(tasks, start, end);
        }
    }
    return tasks;
}

// Avoids needing to use blockingMap if we only have 1 task.
inline void maybeBlockingMap(const auto &tasks, auto function)
{
    if (tasks.size() == 1) {
        function(tasks.front());
        return;
    }
    QtConcurrent::blockingMap(tasks, function);
}


// A struct for sharing channel layout info, designed to facilitate compile time
// code branching.
// In QImage source code, some formats in the QPixelFormat table will have
// QPixelFormat::ByteOrder::CurrentSystemEndian, but that's not a problem.
// The byte order will be resolved to big or little when we need it.
template<ChannelType C_t,
         uint8_t ChannelCount,
         // defaults for 1 channel, must set these with 4 channels
         QPixelFormat::ByteOrder ByteOrder = QPixelFormat::CurrentSystemEndian,
         QPixelFormat::AlphaPosition AlphaPosition = QPixelFormat::AtBeginning,
         QPixelFormat::AlphaPremultiplied AlphaPremultiplied = QPixelFormat::NotPremultiplied>
    requires(ChannelCount == 1 || (ChannelCount == 4 && ByteOrder != QPixelFormat::CurrentSystemEndian))
struct ChannelInfo {
    using Channel_t = C_t;
    static constexpr auto channelCount = ChannelCount;
    static constexpr auto byteOrder = ByteOrder;
    static constexpr auto alphaPosition = AlphaPosition;
    static constexpr auto alphaPremultiplied = AlphaPremultiplied;
    using enum QPixelFormat::AlphaPosition;
    using enum QPixelFormat::ByteOrder;
    // little endian ARGB: BGRA
    // big endian ARGB: ARGB
    // little endian RGBA: ABGR
    // big endian RGBA: RGBA

    static constexpr uint8_t redIndex = byteOrder == LittleEndian
        // A0 ? [ B,G(R)A ] : [ A,B,G(R)]
        ? (alphaPosition == AtBeginning ? 2 : 3)
        // A0 ? [ A(R)G,B ] : [(R)G,B,A ]
        : (alphaPosition == AtBeginning ? 1 : 0);
    static constexpr uint8_t greenIndex = byteOrder == LittleEndian
        // A0 ? [ B(G)R,A ] : [ A,B(G)R ]
        ? (alphaPosition == AtBeginning ? 1 : 2)
        // A0 ? [ A,R(G)B ] : [ R(G)B,A ]
        : (alphaPosition == AtBeginning ? 2 : 1);
    static constexpr uint8_t blueIndex = byteOrder == LittleEndian
        // A0 ? [(B)G,R,A ] : [ A(B)G,R ]
        ? (alphaPosition == AtBeginning ? 0 : 1)
        // A0 ? [ A,R,G(B)] : [ R,G(B)A ]
        : (alphaPosition == AtBeginning ? 3 : 2);
    static constexpr uint8_t alphaIndex = byteOrder == LittleEndian
        // A0 ? [ B,G,R(A)] : [(A)B,G,R ]
        ? (alphaPosition == AtBeginning ? 3 : 0)
        // A0 ? [(A)R,G,B ] : [ R,G,B(A)]
        : (alphaPosition == AtBeginning ? 0 : 3);
    // The logic for green and alpha could be simplified, but leave it as-is
    // because it's more readable this way and it's constexpr anyway.

    struct ChannelIndices {
        size_t r = redIndex;
        size_t g = greenIndex;
        size_t b = blueIndex;
        size_t a = alphaIndex;
        constexpr ChannelIndices &operator+=(size_t add)
        {
            r += add;
            g += add;
            b += add;
            a += add;
            return *this;
        }
    };
    static constexpr ChannelIndices channelIndices{};
};
template<typename T>
concept ChannelInfoType = std::same_as<T, ChannelInfo<typename T::Channel_t, T::channelCount, T::byteOrder, T::alphaPosition, T::alphaPremultiplied>>;

static constexpr size_t uint8Bits = sizeof(uint8_t) << 3;
static constexpr size_t uint16Bits = sizeof(uint16_t) << 3;
static constexpr size_t float32Bits = sizeof(float) << 3;

using MatrixFlag_ut = uint32_t;
enum MatrixFlag : MatrixFlag_ut {
    MF_Identity = 0,
    MF_Translation = 1,
    MF_Scale = 1 << 1,
    // No point in distinguishing 2D and 3D rotations with an RGB matrix.
    MF_Rotation = 1 << 2,
    MF_Perspective = 1 << 3,
    MF_General = MF_Translation | MF_Scale | MF_Rotation | MF_Perspective,
};
// QFlags doesn't work as a template arg, so we need to pass the underlying type
// value to make a constexpr QFlags object.
Q_DECLARE_FLAGS(MatrixFlags, MatrixFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(MatrixFlags)

template<MatrixFlag_ut MF>
concept IsIdentity = !MatrixFlags{MF}.testAnyFlags(MF_General);

template<typename T>
struct VecXW {
    T x, w;
};

// A 2x2 matrix with constexpr operation flags and a flat data layout.
// Unlike a normal 2x2 matrix, this uses X & perspective rows & columns.
template<MatrixFlag_ut MF, typename T>
struct Mat2XW {
    // columns:
    // 0,  1; rows:
    T xx, xw; // 0; X|R|Gray
    T wx, ww; // 1; W|A
    static constexpr MatrixFlags flags{MF};
};

// A 4x4 matrix with constexpr operation flags and a flat data layout.
template<MatrixFlag_ut MF, typename T>
struct Mat4XYZW {
    // columns:
    // 0,  1,  2,  3; rows:
    T xx, xy, xz, xw; // 0; X|R|C
    T yx, yy, yz, yw; // 1; Y|G|M
    T zx, zy, zz, zw; // 2; Z|B|Y
    T wx, wy, wz, ww; // 3; W|A|K
    static constexpr MatrixFlags flags{MF};
};

// Uses the X and perspective rows, unlike the standard map for QVector2D.
template<MatrixFlag_ut MF, typename T>
constexpr VecXW<T> mapMatVecXW(const Mat2XW<MF, T> &m, VecXW<T> v)
{
    using Mat2XW = std::decay_t<decltype(m)>;
    if constexpr (IsIdentity<MF> || Mat2XW::flags == MF_Rotation) {
        return v;
    }
    if constexpr (Mat2XW::flags.testFlags(MF_General)) {
        return {v.x * m.xx + v.w * m.xw, //
                v.x * m.wx + v.w * m.ww};
    }
    if constexpr (Mat2XW::flags.testFlags(MF_Translation | MF_Scale)) {
        return {v.x * m.xx + m.xw, v.w};
    }
    if constexpr (Mat2XW::flags.testFlags(MF_Translation)) {
        return {v.x + m.xw, v.w};
    }
    if constexpr (Mat2XW::flags.testFlags(MF_Scale)) {
        return {v.x * m.xx, v.w};
    }
}

// Like the usual map for QVector4D/vec4 (GLSL), but supports QRgbaFloat and has
// constexpr branching.
template<MatrixFlag_ut MF, typename T>
constexpr QRgbaFloat<T> mapMatVecXYZW(const Mat4XYZW<MF, T> &m, QRgbaFloat<T> v)
{
    using Mat4XYZW = std::decay_t<decltype(m)>;
    if constexpr (IsIdentity<MF>) {
        return v;
    }
    if constexpr (Mat4XYZW::flags.testFlags(MF_General)) {
        return {v.r * m.xx + v.g * m.xy + v.b * m.xz + v.a * m.xw, //
                v.r * m.yx + v.g * m.yy + v.b * m.yz + v.a * m.yw,
                v.r * m.zx + v.g * m.zy + v.b * m.zz + v.a * m.zw,
                v.r * m.wx + v.g * m.wy + v.b * m.wz + v.a * m.ww};
    }
    if constexpr (Mat4XYZW::flags.testFlags(MF_Translation | MF_Scale | MF_Rotation)) {
        return {v.r * m.xx + v.g * m.xy + v.b * m.xz + m.xw, //
                v.r * m.yx + v.g * m.yy + v.b * m.yz + m.yw,
                v.r * m.zx + v.g * m.zy + v.b * m.zz + m.zw,
                v.a};
    }
    if constexpr (Mat4XYZW::flags.testFlags(MF_Translation | MF_Scale)) {
        return {v.r * m.xx + m.xw, //
                v.g * m.yy + m.yw,
                v.b * m.zz + m.zw,
                v.a};
    }
    if constexpr (Mat4XYZW::flags.testFlags(MF_Translation | MF_Rotation)) {
        return {v.r /*  */ + v.g * m.xy + v.b * m.xz + m.xw, //
                v.r * m.yx + v.g /*  */ + v.b * m.yz + m.yw,
                v.r * m.zx + v.g * m.zy + v.b /*  */ + m.zw,
                v.a};
    }
    if constexpr (Mat4XYZW::flags.testFlags(MF_Scale | MF_Rotation)) {
        return {v.r * m.xx + v.g * m.xy + v.b * m.xz, //
                v.r * m.yx + v.g * m.yy + v.b * m.yz,
                v.r * m.zx + v.g * m.zy + v.b * m.zz,
                v.a};
    }
    if constexpr (Mat4XYZW::flags.testFlags(MF_Translation)) {
        return {v.r + m.xw, //
                v.g + m.yw,
                v.b + m.zw,
                v.a};
    }
    if constexpr (Mat4XYZW::flags.testFlags(MF_Scale)) {
        return {v.r * m.xx, //
                v.g * m.yy,
                v.b * m.zz,
                v.a};
    }
    if constexpr (Mat4XYZW::flags.testFlags(MF_Rotation)) {
        return {v.r /*  */ + v.g * m.xy + v.b * m.xz, //
                v.r * m.yx + v.g /*  */ + v.b * m.yz,
                v.r * m.zx + v.g * m.zy + v.b /*  */,
                v.a};
    }
}

inline QColorTransform colorSpaceTransform(QColorSpace from, QColorSpace to)
{
    if (from == to) {
        return {};
    }
    // Cache QColorTransforms since they aren't necessarily cheap to generate.
    // They share underlying data, so at least they're cheap to copy.
    // Can't use QMap/QHash/map/unordered_map since there's no QColorSpace hash.
    struct Element {
        QColorSpace from;
        QColorSpace to;
        QColorTransform transform;
    };
    static auto cache = []() -> std::vector<Element> {
        // Initialize with sRGB <-> linear sRGB since sRGB is the most common.
        QColorSpace sRGB = QColorSpace::SRgb;
        QColorSpace sRGBLinear = QColorSpace::SRgbLinear;
        return {{sRGB, sRGBLinear, sRGB.transformationToColorSpace(sRGBLinear)}, //
                {sRGBLinear, sRGB, sRGBLinear.transformationToColorSpace(sRGB)}};
    }();
    auto it = std::ranges::find_if(cache, [&](const Element &e) {
        return e.from == from && e.to == to;
    });
    if (it != cache.end()) {
        return it->transform;
    }
    return cache.emplace_back(from, to, from.transformationToColorSpace(to)).transform;
}

inline QColorSpace imageColorSpace(const QImage &image)
{
    auto cs = image.colorSpace();
    if (cs.isValidTarget()) {
        return cs;
    }
    return QColorSpace::SRgb;
}

inline QImage defaultImage(const QSize &size, qreal dpr)
{
    // RGBA is better for frequent updates to large regions of the scene graph
    // than ARGB since there's no need to rearrange (swizzle) the channels.
    // ARGB is better for frequent updates to large regions with QPainter.
    // Our QPainter logic usually updates small regions frequently and large
    // regions infrequently.
    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    // All of the following QImage methods will no-op if the QImage is null
    // (e.g., invalid size, failed to allocate).
    // ---
    // By default, QImage has an invalid QColorSpace that is assumed sRGB by
    // QPainter and other Qt/KDE APIs.
    // Explicitly use sRGB so that we can track the colorspace.
    image.setColorSpace(QColorSpace::SRgb);
    image.setDevicePixelRatio(dpr);
    image.fill(Qt::transparent);
    return image;
}

inline QRectF deviceIndependentRect(const QImage &image)
{
    return {{0, 0}, image.deviceIndependentSize()};
}

}

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

    template<std::floating_point T, typename V>
    static constexpr bool fuzzyIsNull(V v)
    {
        // This logic works better with the Windows CI. MSVC isn't as generous
        // as GCC with allowing functions to be marked as constexpr.
        return v <= fuzzyEpsilon<T>() && v >= -fuzzyEpsilon<T>();
    }

    template<std::floating_point T, typename V1, typename V2>
    static constexpr bool fuzzyCompare(V1 v1, V2 v2)
    {
        // This logic works better with the Windows CI. MSVC isn't as generous
        // as GCC with allowing functions to be marked as constexpr.
        return Utils::fuzzyIsNull<T>(v1 < v2 ? v2 - v1 : v1 - v2);
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
        return fuzzyIsNull<float>(v);
    }

    Q_INVOKABLE static constexpr bool fuzzyCompareF32(qreal v1, qreal v2)
    {
        return fuzzyCompare<float>(v1, v2);
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
