// SPDX-FileCopyrightText: 2006 Zack Rusin <zack@kde.org>
// SPDX-FileCopyrightText: 2006-2007, 2008 Fredrik Höglund <fredrik@kde.org>
//
// The stack blur algorithm was invented by Mario Klingemann <mario@quasimondo.com>
//
// This implementation is based on the version in Anti-Grain Geometry Version 2.4,
// SPDX-FileCopyrightText: 2002-2005 Maxim Shemanarev (http://www.antigrain.com)
//
// SPDX-License-Identifier: BSD-2-Clause

#include "stackblur.h"

#include <QPainter>
#include <QImage>
#include <QColor>

static unsigned short const mulTable[255] = {
    512, 512, 456, 512, 328, 456, 335, 512, 405, 328, 271, 456, 388, 335, 292, 512, 454, 405, 364, 328, 298, 271, 496, 456, 420, 388, 360, 335, 312,
    292, 273, 512, 482, 454, 428, 405, 383, 364, 345, 328, 312, 298, 284, 271, 259, 496, 475, 456, 437, 420, 404, 388, 374, 360, 347, 335, 323, 312,
    302, 292, 282, 273, 265, 512, 497, 482, 468, 454, 441, 428, 417, 405, 394, 383, 373, 364, 354, 345, 337, 328, 320, 312, 305, 298, 291, 284, 278,
    271, 265, 259, 507, 496, 485, 475, 465, 456, 446, 437, 428, 420, 412, 404, 396, 388, 381, 374, 367, 360, 354, 347, 341, 335, 329, 323, 318, 312,
    307, 302, 297, 292, 287, 282, 278, 273, 269, 265, 261, 512, 505, 497, 489, 482, 475, 468, 461, 454, 447, 441, 435, 428, 422, 417, 411, 405, 399,
    394, 389, 383, 378, 373, 368, 364, 359, 354, 350, 345, 341, 337, 332, 328, 324, 320, 316, 312, 309, 305, 301, 298, 294, 291, 287, 284, 281, 278,
    274, 271, 268, 265, 262, 259, 257, 507, 501, 496, 491, 485, 480, 475, 470, 465, 460, 456, 451, 446, 442, 437, 433, 428, 424, 420, 416, 412, 408,
    404, 400, 396, 392, 388, 385, 381, 377, 374, 370, 367, 363, 360, 357, 354, 350, 347, 344, 341, 338, 335, 332, 329, 326, 323, 320, 318, 315, 312,
    310, 307, 304, 302, 299, 297, 294, 292, 289, 287, 285, 282, 280, 278, 275, 273, 271, 269, 267, 265, 263, 261, 259};

static unsigned char const shgTable[255] = {
    9,  11, 12, 13, 13, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24};

void StackBlur::blur(QImage &image, const QSize &kernelSize)
{
    if (kernelSize.width() == 1 && kernelSize.height() == 1) {
        return;
    }

    const int radiusX = kernelSize.width();
    const int radiusY = kernelSize.height();
    const int w = image.width();
    const int h = image.height();

    const int wm = w - 1;
    const int hm = h - 1;

    std::vector<int> r(w * h), g(w * h), b(w * h);
    std::vector<int> vmin(std::max(w, h));

    // Horizontal pass with radiusX
    if (radiusX > 0) {
        int divX = radiusX + radiusX + 1;
        int divsumX = ((divX + 1) >> 1);
        divsumX *= divsumX;

        struct Pixel {
            int r, g, b;
        };
        std::vector<Pixel> stack(divX);

        for (int y = 0; y < h; y++) {
            int rinsum = 0, ginsum = 0, binsum = 0, routsum = 0, goutsum = 0, boutsum = 0;
            int rsum = 0, gsum = 0, bsum = 0;
            int yi = y * w;

            for (int i = -radiusX; i <= radiusX; i++) {
                int x_clamp = std::clamp(i, 0, wm);
                QRgb p = image.pixel(x_clamp, y);
                Pixel &sir = stack[i + radiusX];
                sir.r = qRed(p);
                sir.g = qGreen(p);
                sir.b = qBlue(p);

                int rbs = radiusX + 1 - abs(i);
                rsum += sir.r * rbs;
                gsum += sir.g * rbs;
                bsum += sir.b * rbs;

                if (i > 0) {
                    rinsum += sir.r;
                    ginsum += sir.g;
                    binsum += sir.b;
                } else {
                    routsum += sir.r;
                    goutsum += sir.g;
                    boutsum += sir.b;
                }
            }

            int stackpointer = radiusX;
            for (int x = 0; x < w; x++) {
                r[yi] = (rsum * mulTable[radiusX]) >> shgTable[radiusX];
                g[yi] = (gsum * mulTable[radiusX]) >> shgTable[radiusX];
                b[yi] = (bsum * mulTable[radiusX]) >> shgTable[radiusX];

                rsum -= routsum;
                gsum -= goutsum;
                bsum -= boutsum;

                int stackstart = (stackpointer - radiusX + divX) % divX;
                Pixel &sir = stack[stackstart];

                routsum -= sir.r;
                goutsum -= sir.g;
                boutsum -= sir.b;

                if (y == 0)
                    vmin[x] = std::clamp(x + radiusX + 1, 0, wm);

                QRgb p = image.pixel(vmin[x], y);
                sir.r = qRed(p);
                sir.g = qGreen(p);
                sir.b = qBlue(p);

                rinsum += sir.r;
                ginsum += sir.g;
                binsum += sir.b;
                rsum += rinsum;
                gsum += ginsum;
                bsum += binsum;

                stackpointer = (stackpointer + 1) % divX;

                Pixel &sir2 = stack[stackpointer];
                routsum += sir2.r;
                goutsum += sir2.g;
                boutsum += sir2.b;
                rinsum -= sir2.r;
                ginsum -= sir2.g;
                binsum -= sir2.b;

                yi++;
            }
        }
    } else {
        // If radiusX==0, copy the image to r/g/b
        for (int y = 0; y < h; ++y) {
            int yi = y * w;
            for (int x = 0; x < w; ++x, ++yi) {
                QRgb p = image.pixel(x, y);
                r[yi] = qRed(p);
                g[yi] = qGreen(p);
                b[yi] = qBlue(p);
            }
        }
    }

    // Vertical pass with radiusY
    if (radiusY > 0) {
        int divY = radiusY + radiusY + 1;
        int divsumY = ((divY + 1) >> 1);
        divsumY *= divsumY;

        struct Pixel {
            int r, g, b;
        };
        std::vector<Pixel> stack(divY);

        for (int x = 0; x < w; x++) {
            int rinsum = 0, ginsum = 0, binsum = 0, routsum = 0, goutsum = 0, boutsum = 0;
            int rsum = 0, gsum = 0, bsum = 0;
            int yi = x;

            for (int i = -radiusY; i <= radiusY; i++) {
                int y_clamp = std::clamp(i, 0, hm);
                int p = y_clamp * w + x;
                Pixel &sir = stack[i + radiusY];
                sir.r = r[p];
                sir.g = g[p];
                sir.b = b[p];
                int rbs = radiusY + 1 - abs(i);
                rsum += r[p] * rbs;
                gsum += g[p] * rbs;
                bsum += b[p] * rbs;

                if (i > 0) {
                    rinsum += sir.r;
                    ginsum += sir.g;
                    binsum += sir.b;
                } else {
                    routsum += sir.r;
                    goutsum += sir.g;
                    boutsum += sir.b;
                }
            }

            int stackpointer = radiusY;
            for (int y = 0; y < h; y++) {
                image.setPixel(x,
                               y,
                               qRgb((rsum * mulTable[radiusY]) >> shgTable[radiusY],
                                    (gsum * mulTable[radiusY]) >> shgTable[radiusY],
                                    (bsum * mulTable[radiusY]) >> shgTable[radiusY]));

                rsum -= routsum;
                gsum -= goutsum;
                bsum -= boutsum;

                int stackstart = (stackpointer - radiusY + divY) % divY;
                Pixel &sir = stack[stackstart];
                routsum -= sir.r;
                goutsum -= sir.g;
                boutsum -= sir.b;

                if (x == 0)
                    vmin[y] = std::clamp(y + radiusY + 1, 0, hm) * w;

                int p = x + vmin[y];
                sir.r = r[p];
                sir.g = g[p];
                sir.b = b[p];

                rinsum += sir.r;
                ginsum += sir.g;
                binsum += sir.b;
                rsum += rinsum;
                gsum += ginsum;
                bsum += binsum;

                stackpointer = (stackpointer + 1) % divY;

                Pixel &sir2 = stack[stackpointer];
                routsum += sir2.r;
                goutsum += sir2.g;
                boutsum += sir2.b;
                rinsum -= sir2.r;
                ginsum -= sir2.g;
                binsum -= sir2.b;

                yi += w;
            }
        }
    }
}
