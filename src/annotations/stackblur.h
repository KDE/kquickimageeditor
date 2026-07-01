// SPDX-FileCopyrightText: 2010 Mario Klingemann <mario@quasimondo.com>
// SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

/*
The stack blur algorithm was invented by Mario Klingemann.
Original explanation:
https://quasimondo.com/2004/02/25/stackblur-2004
Online demo (the original page is gone):
https://web.archive.org/web/20201012234050/http://www.quasimondo.com/StackBlurForCanvas/StackBlurDemo.html

Detailed explanation:
https://melatonin.dev/blog/implementing-marios-stack-blur-15-times-in-cpp
*/

#pragma once

#include <QImage>

namespace StackBlur
{
/**
 * Maximum blur radius in pixels.
 * This value is kind of arbitrary since we don't use a multiplier lookup table.
 * It's internally helpful to limit the upper bound of what the blur might do.
 */
[[nodiscard]] constexpr int maxRadius()
{
    // Same as the max from Mario Klingemann's original implementation.
    return 254;
}

/**
 * Minimum usable blur radius in pixels.
 * You can't really go lower than this or else you won't blur anything.
 */
[[nodiscard]] constexpr int minRadius()
{
    return 1;
}

/**
 * Get the kernel size for a given blur radius.
 */
template<typename T>
[[nodiscard]] constexpr int kernelSizeFromRadius(T radius)
{
    return radius * 2 + 1;
}

/**
 * Maximum blur kernel size in pixels.
 */
[[nodiscard]] constexpr int maxKernel()
{
    return kernelSizeFromRadius(maxRadius());
}

/**
 * Minimum usable blur kernel size in pixels.
 */
[[nodiscard]] constexpr int minKernel()
{
    return kernelSizeFromRadius(minRadius());
}

/**
 * Get a span of known supported QImage formats.
 * Returns a span so that the return type doesn't have to change every time we
 * update the list (vs directly returning std::array) and so that it can be
 * cheaply copied by value.
 */
[[nodiscard]] constexpr std::span<const QImage::Format> supportedImageFormats()
{
    static constexpr std::array list{
        QImage::Format_Alpha8,
        QImage::Format_Grayscale8,
        QImage::Format_Grayscale16,
        QImage::Format_RGB32,
        QImage::Format_ARGB32_Premultiplied,
        QImage::Format_RGBX8888,
        QImage::Format_RGBA8888_Premultiplied,
        QImage::Format_RGBX64,
        QImage::Format_RGBA64_Premultiplied,
    };
    return list;
}

/**
 * Apply a stack blur to an entire image.
 * Will do nothing if the image format is not supported.
 */
void blur(QImage &image, int radiusX, int radiusY);
}
