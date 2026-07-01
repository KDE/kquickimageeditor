// SPDX-FileCopyrightText: 2010 Mario Klingemann <mario@quasimondo.com>
// SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "stackblur.h"

#include <QImage>

namespace StackBlur
{

void blur(QImage &image, int radiusX, int radiusY)
{
    static constexpr auto supportedFormats = supportedImageFormats();
    // Null images will have Format_Invalid. Images with (width < 1 || height < 1)
    // will be null, so this also checks if the size is valid.
    if (std::find(supportedFormats.begin(), supportedFormats.end(), image.format()) == supportedFormats.end()) {
        return;
    }
    // Radius is never more than (size-1)/2 because the kernel should always
    // have an odd size and should never be bigger than the image.
    radiusX = std::min({radiusX, (image.width() - 1) / 2, maxRadius()});
    radiusY = std::min({radiusY, (image.height() - 1) / 2, maxRadius()});
    if (radiusX < minRadius() && radiusY < minRadius()) {
        return;
    }
}

} // namespace StackBlur
