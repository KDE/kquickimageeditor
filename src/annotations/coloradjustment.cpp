// SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "coloradjustment.h"
#include <QColorSpace>
#include <QColorTransform>
#include <QDebug>
#include <QImage>
#include <QRgb>
#include <QRgbaFloat>

namespace ColorAdjustment
{

// If at some point in the future we need to do something with relative luminance,
// here are sRGB relative luminance (Y) weights:
// rY = 0.212655f, gY = 0.715158f, bY = 0.072187f
// rgbY_hypotenuse = 0.749615f

template<typename T, typename... Ts>
concept IsAnyOf = std::disjunction_v<std::is_same<T, Ts>...>;

template<typename T>
concept ChannelType = IsAnyOf<T, uint8_t, uint16_t, qfloat16, float>;

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

QColorTransform colorSpaceTransform(QColorSpace from, QColorSpace to)
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
};

template<MatrixFlag_ut MF, bool AdjustGamma, bool ConvertColorSpace>
void execTransform(QImage &image, const QMatrix4x4 &qMat4, float gamma, QColorTransform toLinear, QColorTransform toOriginal)
{
    QList<QRgb> colorTable = image.colorTable();
    const bool isIndexed = !colorTable.empty();
    // Format_Indexed8 has QPixelFormat::IgnoresAlpha even though
    // QImage::hasAlphaChannel can be true for Format_Indexed8.
    // It depends on whether one of the colors has an alpha other than 255.
    // Format_Indexed8 does not use premultiplied alpha.
    const auto format = isIndexed //
        ? (image.hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32)
        : image.format();
    const auto pixelFormat = QImage::toPixelFormat(format);
    const size_t channelCount = pixelFormat.channelCount();
    const size_t bitsPerChannel = pixelFormat.bitsPerPixel() / channelCount;
    const size_t totalChannels = !colorTable.empty() //
        ? colorTable.size() * channelCount
        : image.width() * image.height() * channelCount;

    auto exec = [&]<ChannelInfoType Info>(Info, typename Info::Channel_t *data) {
        // The code is set up this way to make it easier to set up SIMD later.
        using C_t = typename Info::Channel_t;
        // Math type
        using M_t = float;
        // Math type container for RGBA values
        using RGBA_t = QRgbaFloat<M_t>;
        // meant to be used like literal values
        constexpr M_t _0{0};
        constexpr M_t _1{1};
        constexpr C_t min = std::numeric_limits<C_t>::lowest();
        constexpr C_t max = std::numeric_limits<C_t>::max();
        static_assert(min >= std::numeric_limits<M_t>::lowest());
        static_assert(max <= std::numeric_limits<M_t>::max());
        constexpr M_t invMaxF = _1 / max;
        const M_t gammaPow = _1 / gamma;
        if constexpr (Info::channelCount == 1) {
            const Mat2XW<MF, M_t> matrix{// clang-format off
                qMat4(0,0), qMat4(0,3),
                qMat4(3,0), qMat4(3,3)
            }; // clang-format on
            constexpr M_t _3{3};
            for (size_t i = 0; i < totalChannels; ++i) {
                M_t color = data[i];
                if constexpr (std::integral<C_t>) {
                    color *= invMaxF;
                }
                if constexpr (ConvertColorSpace) {
                    auto rgba = toLinear.map(RGBA_t{color, color, color, _1});
                    color = (rgba.r + rgba.g + rgba.b) / _3;
                }
                if constexpr (!IsIdentity<MF>) {
                    const auto [x, w] = mapMatVecXW(matrix, VecXW<M_t>{color, _1});
                    // clamp before applying gamma to match the preview shader's logic
                    color = std::clamp(x / w, _0, _1);
                }
                if constexpr (AdjustGamma) {
                    color = std::pow(color, gammaPow);
                }
                if constexpr (ConvertColorSpace) {
                    auto rgba = toOriginal.map(RGBA_t{color, color, color, _1});
                    color = (rgba.r + rgba.g + rgba.b) / _3;
                }
                if constexpr (std::integral<C_t>) {
                    data[i] = std::lround(color * max);
                } else {
                    data[i] = color;
                }
            }
        } else if constexpr (Info::channelCount == 4) {
            using Mat4XYZW = Mat4XYZW<MF, M_t>;
            const Mat4XYZW matrix{// clang-format off
                qMat4(0,0), qMat4(0,1), qMat4(0,2), qMat4(0,3),
                qMat4(1,0), qMat4(1,1), qMat4(1,2), qMat4(1,3),
                qMat4(2,0), qMat4(2,1), qMat4(2,2), qMat4(2,3),
                qMat4(3,0), qMat4(3,1), qMat4(3,2), qMat4(3,3)
            }; // clang-format on
            for (auto i = Info::channelIndices; i.a < totalChannels; i += Info::channelCount) {
                if constexpr (Info::alphaPremultiplied == QPixelFormat::Premultiplied) {
                    // No point in doing anything with a color that will become
                    // {0,0,0,0} after re-premultiplying.
                    if (data[i.a] == C_t{0}) {
                        continue;
                    }
                }
                RGBA_t color;
                M_t alpha;
                if constexpr (std::integral<C_t>) {
                    color.r = data[i.r] * invMaxF;
                    color.g = data[i.g] * invMaxF;
                    color.b = data[i.b] * invMaxF;
                } else {
                    color.r = data[i.r];
                    color.g = data[i.g];
                    color.b = data[i.b];
                }
                color.a = 1.0f;
                if constexpr (Info::alphaPremultiplied == QPixelFormat::Premultiplied) {
                    if constexpr (std::integral<C_t>) {
                        alpha = data[i.a] * invMaxF;
                    } else {
                        alpha = data[i.a];
                    }
                    color.r = color.r / alpha;
                    color.g = color.g / alpha;
                    color.b = color.b / alpha;
                }
                // Do the colorspace map after converting the color to floating
                // point values to preserve precision.
                if constexpr (ConvertColorSpace) {
                    color = toLinear.map(color);
                }
                if constexpr (!IsIdentity<MF>) {
                    color = mapMatVecXYZW(matrix, color);
                    if constexpr (Mat4XYZW::flags.testFlag(MF_Perspective)) {
                        color.r = color.r / color.a;
                        color.g = color.g / color.a;
                        color.b = color.b / color.a;
                        color.a = _1;
                    }
                    // clamp before applying gamma to match the preview shader's logic
                    color.r = std::clamp(color.r, _0, _1);
                    color.g = std::clamp(color.g, _0, _1);
                    color.b = std::clamp(color.b, _0, _1);
                }
                if constexpr (AdjustGamma) {
                    color.r = std::pow(color.r, gammaPow);
                    color.g = std::pow(color.g, gammaPow);
                    color.b = std::pow(color.b, gammaPow);
                }
                if constexpr (ConvertColorSpace) {
                    color = toOriginal.map(color);
                }
                if constexpr (Info::alphaPremultiplied == QPixelFormat::Premultiplied) {
                    color.r = color.r * alpha;
                    color.g = color.g * alpha;
                    color.b = color.b * alpha;
                }
                if constexpr (std::integral<C_t>) {
                    data[i.r] = std::lround(color.r * max);
                    data[i.g] = std::lround(color.g * max);
                    data[i.b] = std::lround(color.b * max);
                } else {
                    data[i.r] = color.r;
                    data[i.g] = color.g;
                    data[i.b] = color.b;
                }
            }
        }
    };
    auto execChannelInfo = [&]<ChannelType C_t>(C_t *data) {
        const bool alphaAtBeginning = pixelFormat.alphaPosition() == QPixelFormat::AtBeginning;
        if (channelCount == 1) {
            exec(ChannelInfo<C_t, 1>{}, data);
        } else if (channelCount == 4) {
            auto execPremultiplied = [&]<QPixelFormat::ByteOrder BO, QPixelFormat::AlphaPosition AP> {
                const bool premultiplied = pixelFormat.premultiplied() == QPixelFormat::Premultiplied;
                if (premultiplied) {
                    exec(ChannelInfo<C_t, 4, BO, AP, QPixelFormat::Premultiplied>{}, data);
                } else {
                    exec(ChannelInfo<C_t, 4, BO, AP, QPixelFormat::NotPremultiplied>{}, data);
                }
            };
            auto execAlphaPos = [&]<QPixelFormat::ByteOrder BO> {
                if (alphaAtBeginning) {
                    execPremultiplied.template operator()<BO, QPixelFormat::AtBeginning>();
                } else {
                    execPremultiplied.template operator()<BO, QPixelFormat::AtEnd>();
                }
            };
            if (pixelFormat.byteOrder() == QPixelFormat::LittleEndian) {
                execAlphaPos.template operator()<QPixelFormat::LittleEndian>();
            } else {
                execAlphaPos.template operator()<QPixelFormat::BigEndian>();
            }
        }
    };
    if (!colorTable.empty()) {
        execChannelInfo(reinterpret_cast<uint8_t *>(colorTable.data()));
        image.setColorTable(colorTable);
    } else if (bitsPerChannel == uint8Bits) {
        execChannelInfo(reinterpret_cast<uint8_t *>(image.bits()));
    } else if (bitsPerChannel == uint16Bits) {
        execChannelInfo(reinterpret_cast<uint16_t *>(image.bits()));
    } else if (bitsPerChannel == float32Bits) {
        execChannelInfo(reinterpret_cast<float *>(image.bits()));
    }
};

void adjust(QImage &image, const QMatrix4x4 &matrix, float gamma)
{
    if (image.isNull()) {
        return;
    }
    const bool applyMatrix = !matrix.isIdentity();
    const bool applyGamma = isValidGammaAdjustment(gamma);
    if (!applyMatrix && !applyGamma) {
        return;
    }
    const auto originalColorSpace = [&]() -> QColorSpace {
        auto cs = image.colorSpace();
        if (cs.isValidTarget()) {
            return cs;
        }
        // assume sRGB by default
        return QColorSpace::SRgb;
    }();
    const auto linearColorSpace = originalColorSpace.withTransferFunction(QColorSpace::TransferFunction::Linear);
    const auto toLinear = colorSpaceTransform(originalColorSpace, linearColorSpace);
    const auto toOriginal = colorSpaceTransform(linearColorSpace, originalColorSpace);

    const auto m4x4Flags = matrix.flags();
    // QMatrix4x4 always uses General with Perspective
    const bool generalFlag = m4x4Flags.testFlag(QMatrix4x4::Perspective);
    // Not much of a point in differentiating 2D and 3D rotations for an RGB matrix.
    const bool rotationFlag = generalFlag || m4x4Flags.testAnyFlags(QMatrix4x4::Rotation2D | QMatrix4x4::Rotation);
    const bool scaleFlag = generalFlag || m4x4Flags.testFlag(QMatrix4x4::Scale);
    const bool translationFlag = generalFlag || m4x4Flags.testFlag(QMatrix4x4::Translation);
    auto execMatrixFlags = [&]<bool AdjustGamma, bool ConvertColorSpace> {
        if (!applyMatrix) {
            execTransform<MF_Identity, AdjustGamma, ConvertColorSpace>(image, matrix, gamma, toLinear, toOriginal);
        } else if (generalFlag) {
            execTransform<MF_General, AdjustGamma, ConvertColorSpace>(image, matrix, gamma, toLinear, toOriginal);
        } else if (translationFlag && scaleFlag && rotationFlag) {
            execTransform<MF_Translation | MF_Scale | MF_Rotation, AdjustGamma, ConvertColorSpace>(image, matrix, gamma, toLinear, toOriginal);
        } else if (translationFlag && scaleFlag) {
            execTransform<MF_Translation | MF_Scale, AdjustGamma, ConvertColorSpace>(image, matrix, gamma, toLinear, toOriginal);
        } else if (translationFlag && rotationFlag) {
            execTransform<MF_Translation | MF_Rotation, AdjustGamma, ConvertColorSpace>(image, matrix, gamma, toLinear, toOriginal);
        } else if (scaleFlag && rotationFlag) {
            execTransform<MF_Scale | MF_Rotation, AdjustGamma, ConvertColorSpace>(image, matrix, gamma, toLinear, toOriginal);
        } else if (translationFlag) {
            execTransform<MF_Translation, AdjustGamma, ConvertColorSpace>(image, matrix, gamma, toLinear, toOriginal);
        } else if (scaleFlag) {
            execTransform<MF_Scale, AdjustGamma, ConvertColorSpace>(image, matrix, gamma, toLinear, toOriginal);
        } else if (rotationFlag) {
            execTransform<MF_Rotation, AdjustGamma, ConvertColorSpace>(image, matrix, gamma, toLinear, toOriginal);
        }
    };
    if (applyGamma) {
        if (toLinear.isIdentity()) {
            execMatrixFlags.operator()<true, false>();
        } else {
            execMatrixFlags.operator()<true, true>();
        }
    } else {
        if (toLinear.isIdentity()) {
            execMatrixFlags.operator()<false, false>();
        } else {
            execMatrixFlags.operator()<false, true>();
        }
    }
}

} // Namespace
