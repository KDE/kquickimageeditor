// SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QImage>
#include <QMatrix4x4>
#include <QMetaEnum>
#include <QtCompilerDetection>

namespace ColorAdjustment
{
constexpr bool isValidGammaAdjustment(float gamma)
{
    // TODO: figure out why including "utils.h" for fuzzyEpsilon causes compilation to fail
    return gamma > 0.00001f && gamma <= std::numeric_limits<float>::max()
        && (gamma >= 1.0f ? gamma - 1.0f : 1.0f - gamma) > 0.00001f;
}
/**
Apply a normalized RGB 4x4 matrix and a gamma to an image in a linear colorspace.

The matrix row order is red, green, blue, projection/attenuation.
We will refer to these rows as `r4[0…3]`, `g4[0…3]`, `b4[0…3]` and `w4[0…3]`.
`w4` is not for white, 'w' is just a commonly used variable for projections.
`r4`, `g4` and `b4` can scale, translate, rotate and shear.
`w4[0…2]` can attenuate red, green and blue. `w4[3]` is a global divisor.
With a floating point grayscale image, `w4` would work like this:
```c++
// With only 1 channel, we use red for transformations.
// This constructor is column-major, but operations are row-major.
//     QMatrix4x4{  r4,   g4,   b4,   w4}: "Rows", but laid out like columns.
QMatrix4x4 matrix{0.5f, 0.0f, 0.0f,-0.5f, // red scale, _, _, red attenuation
                  0.0f, 1.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 1.0f, 0.0f,
                  0.5f, 0.0f, 0,0f, 1.5f}; // red translation, _, _, divisor
QVector4D r4 = matrix.row(0); // {red scale, _, _, red translation}
QVector4D w4 = matrix.row(3); // {red attenuation, _, _, divisor}
// If we were using an integer format, we'd to normalize the channel values.
auto gray = float32Data[i];
gray = std::clamp((gray * r4[0] + r4[3]) / (gray * w4[0] + w4[3]), 0.0f, 1.0f);
// If we were using an integer format, we'd denormalize and round.
float32Data[0] = gray;
```
Floating point grayscale is not a real QImage format, but we used it here
because it's easy to explain with. It's harder to explain how `w4` works with
multiple channels since every channel can affect every other channel. Read the
source code of `QMatrix4x4::map(const QVector3D &point)` to understand how `w4`
works with RGB channels.
*/
void adjust(QImage &image, const QMatrix4x4 &matrix, float gamma = 1.0f);
}
