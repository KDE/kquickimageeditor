// SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "stackblur.h"

#include <QImage>
#include <opencv2/opencv.hpp>

/**
 * Convenience functions for using OpenCV with Qt APIs.
 */
namespace
{
static constexpr int INVALID_MAT_TYPE = -1;
static_assert(CV_8U == 0);
static_assert(std::same_as<decltype(CV_8U), int>);

inline constexpr int matType(QPixelFormat::TypeInterpretation typeInterpretation)
{
    switch (typeInterpretation) {
    case QPixelFormat::UnsignedByte:
        return CV_8U;
    case QPixelFormat::UnsignedShort:
        return CV_16U;
    case QPixelFormat::FloatingPoint:
        return CV_32F;
    default:
        return INVALID_MAT_TYPE;
    }
}

inline constexpr int matType(QPixelFormat pixelFormat)
{
    const auto baseType = matType(pixelFormat.typeInterpretation());
    if (baseType == INVALID_MAT_TYPE) {
        return INVALID_MAT_TYPE;
    }
    return CV_MAKETYPE(baseType, pixelFormat.channelCount());
}

inline cv::Mat qImageToMat(QImage &image)
{
    const auto type = matType(image.pixelFormat());
    if (type == INVALID_MAT_TYPE) {
        return {};
    }
    // Use the constructor with cv::Size as the first arg to avoid type ambiguity in the args.
    return cv::Mat(cv::Size{image.width(), image.height()}, type, image.bits(), image.bytesPerLine());
}
}

void StackBlur::blur(QImage &image, const QSize &kernelSize)
{
    auto mat = qImageToMat(image);
    cv::stackBlur(mat, mat, {kernelSize.width(), kernelSize.height()});
}
