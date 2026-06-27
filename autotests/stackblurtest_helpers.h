// SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QImage>
#include <random>

// Using 32-bits is supposed to be faster than using fewer bits on modern CPUs
// with the random number generator.
// Also, std::uniform_int_distribution can only use
// short, int, long, long long, and their unsigned counterparts,
// so you can't use char/unsigned char.
template<typename T,
         typename T32 = std::conditional_t<std::is_floating_point_v<T>, float, uint32_t>,
         typename T64 = std::conditional_t<std::is_floating_point_v<T>, double, uint64_t>>
using RNGValueType = std::conditional_t<sizeof(T) <= sizeof(T32), T32, T64>;

inline void randomizeImageData(QImage &image)
{
    if (image.isNull()) {
        return;
    }
    auto prng = std::mt19937(qHash("fixed seed for reproducibility",
                                   // qHash seed defaults to 0. Set 0 just to be sure.
                                   0uz));
    const auto pixelFormat = image.pixelFormat();
    const auto bpp = pixelFormat.bitsPerPixel();
    const auto channels = pixelFormat.channelCount();
    const auto bytesPerChannel = (bpp / channels) >> 3;
    const bool usesFloats = pixelFormat.typeInterpretation() == QPixelFormat::FloatingPoint;
    if (usesFloats) {
        if (bytesPerChannel == sizeof(float)) {
            // Floating-point spaces are traditionally bound from 0 to 1,
            // although there are floating point formats that are supposed to
            // support values greater than 1.
            auto dist = std::uniform_real_distribution<RNGValueType<float>>(0.0f, 1.0f);
            const auto height = image.height();
            for (int y = 0; y < height; ++y) {
                auto *const row = reinterpret_cast<float *>(image.scanLine(y));
                const auto elementsPerLine = (image.bytesPerLine() / sizeof(float));
                for (size_t i = 0; i < elementsPerLine; ++i) {
                    row[i] = dist(prng);
                }
            }
        }
    } else {
        if (bytesPerChannel == sizeof(uint8_t)) {
            auto dist = std::uniform_int_distribution<RNGValueType<uint8_t>>(0, std::numeric_limits<uint8_t>::max());
            const auto height = image.height();
            for (int y = 0; y < height; ++y) {
                auto *const row = reinterpret_cast<uint8_t *>(image.scanLine(y));
                const auto elementsPerLine = (image.bytesPerLine() / sizeof(uint8_t));
                for (size_t i = 0; i < elementsPerLine; ++i) {
                    row[i] = dist(prng);
                }
            }
        } else if (bytesPerChannel == sizeof(uint16_t)) {
            auto dist = std::uniform_int_distribution<RNGValueType<uint16_t>>(0, std::numeric_limits<uint16_t>::max());
            const auto height = image.height();
            for (int y = 0; y < height; ++y) {
                auto *const row = reinterpret_cast<uint16_t *>(image.scanLine(y));
                const auto elementsPerLine = (image.bytesPerLine() / sizeof(uint16_t));
                for (size_t i = 0; i < elementsPerLine; ++i) {
                    row[i] = dist(prng);
                }
            }
        }
    }
}

namespace ImageComparison
{
Q_NAMESPACE
enum Result {
    BothNull,
    OriginalNull,
    ModifiedNull,
    WidthDifferent,
    HeightDifferent,
    PixelDifferent,
    ExactSamePixels,
};
Q_ENUM_NS(Result)

template<typename PixelType>
inline Result compare(const QImage &original, const QImage &modified)
{
    if (original.isNull() && modified.isNull()) {
        return BothNull;
    }
    if (original.isNull()) {
        return OriginalNull;
    }
    if (modified.isNull()) {
        return ModifiedNull;
    }
    if (original.width() != modified.width()) {
        return WidthDifferent;
    }
    if (original.height() != modified.height()) {
        return HeightDifferent;
    }
    // A QImage is a 1D array with 32-bit minimum alignment. If we're comparing
    // pixels in row-major order for two images with the same size, we can treat
    // them the same as two 1D arrays.
    const size_t totalPixels = original.width() * original.height();
    const auto originalData = reinterpret_cast<const PixelType *>(original.constBits());
    const auto modifiedData = reinterpret_cast<const PixelType *>(modified.constBits());
    // brute force pixel comparison
    for (size_t i = 0; i < totalPixels; ++i) {
        if (*(originalData + i) != *(modifiedData + i)) {
            return PixelDifferent;
        }
    }
    return ExactSamePixels;
};
}

template<typename PixelCoordinateType>
struct Radius {
    constexpr Radius(auto r)
        : x(r)
        , y(r)
    {
    }
    constexpr Radius(auto x, auto y)
        : x(x)
        , y(y)
    {
    }
    PixelCoordinateType x;
    PixelCoordinateType y;
};
