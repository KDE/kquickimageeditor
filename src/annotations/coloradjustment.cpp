// SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "coloradjustment.h"

#include "utils.h"

#include <QColorSpace>
#include <QColorTransform>
#include <QDebug>
#include <QImage>
#include <QRgb>
#include <QRgbaFloat>

using namespace UtilsNS;

namespace ColorAdjustment
{

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
