// SPDX-FileCopyrightText: 2025 Carl Schwan <carl@carlschwan.eu>
// SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "../src/annotations/stackblur.h"
#include "stackblurtest_helpers.h"

#include <QImage>
#include <QObject>
#include <QTest>

using namespace Qt::StringLiterals;

class StackBlurTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testFormats();
    void testRadius();
};

void StackBlurTest::testFormats()
{
    using enum ImageComparison::Result;
    std::string debugString;
    // Verify known supported formats.
    static constexpr auto supportedFormats = StackBlur::supportedImageFormats();
    static_assert(std::find(supportedFormats.begin(), supportedFormats.end(), QImage::Format_Invalid) == supportedFormats.end());
    for (auto format : supportedFormats) {
        QImage image1(64, 64, format);
        randomizeImageData(image1);
        QImage image2 = image1;
        StackBlur::blur(image2, 8, 8);
        const auto result = ImageComparison::compare<QRgb>(image1, image2);
        debugString = u"Got %1 for %2"_s.arg(QDebug::toString(result), QDebug::toString(format)).toStdString();
        QVERIFY2(result == PixelDifferent, debugString.c_str());
    }
}

void StackBlurTest::testRadius()
{
    using enum ImageComparison::Result;
    using Px = decltype(StackBlur::minRadius());
    // Make sure our basic assumptions are correct.
    static_assert(std::is_same_v<decltype(StackBlur::maxRadius()), Px>);
    static_assert(std::is_same_v<decltype(StackBlur::minRadius()), Px>);
    static_assert(std::is_same_v<decltype(StackBlur::maxKernel()), Px>);
    static_assert(std::is_same_v<decltype(StackBlur::minKernel()), Px>);
    static_assert(StackBlur::maxRadius() > 0);
    static_assert(StackBlur::minRadius() > 0);
    static_assert(StackBlur::minRadius() < StackBlur::maxRadius());
    static_assert(StackBlur::maxKernel() > 0);
    static_assert(StackBlur::minKernel() > 0);
    static_assert(StackBlur::minKernel() < StackBlur::maxKernel());
    static_assert(StackBlur::maxKernel() / 2 == StackBlur::maxRadius());
    static_assert(StackBlur::minKernel() / 2 == StackBlur::minRadius());
    static_assert((StackBlur::minKernel() & 1) != 0); // is odd
    static_assert((StackBlur::maxKernel() & 1) != 0);
    static constexpr auto IMAGE_FORMAT = QImage::Format_ARGB32_Premultiplied;
    static constexpr auto supportedImageFormats = StackBlur::supportedImageFormats();
    static_assert(std::find(supportedImageFormats.begin(), supportedImageFormats.end(), IMAGE_FORMAT) != supportedImageFormats.end());

    QImage image;

    auto setSize = [&](int width, int heigth = -1) {
        image = QImage{width, heigth >= 0 ? heigth : width, IMAGE_FORMAT};
        randomizeImageData(image);
    };
    // We don't just put QCOMPARE or QVERIFY in here because then the program
    // will tell you something failed in the lambda when a test fails.
    auto testRadius = [&](Radius<Px> radius) {
        QImage testImage = image;
        auto [radiusX, radiusY] = radius;
        StackBlur::blur(testImage, radiusX, radiusY);
        return ImageComparison::compare<QRgb>(image, testImage);
    };

    // No change to verify since the QImages are null,
    // but make sure this doesn't crash
    setSize(0);
    QCOMPARE(testRadius(1), BothNull);

    // Test too small to blur
    setSize(1);
    QCOMPARE(testRadius(1), ExactSamePixels);

    // Test minimum
    setSize(StackBlur::minKernel()); // minimum blurrable size
    QCOMPARE(testRadius(StackBlur::minRadius()), PixelDifferent);
    // Test slightly out in case of off by 1 error
    QCOMPARE(testRadius(StackBlur::minRadius() + 1), PixelDifferent);
    // Test 0 radius
    QCOMPARE(testRadius(0), ExactSamePixels);
    // Test negative
    QCOMPARE(testRadius(-1), ExactSamePixels);
    QCOMPARE(testRadius(-100), ExactSamePixels);
    QCOMPARE(testRadius(std::numeric_limits<int>::lowest()), ExactSamePixels);
    // Test max constrained by image size.
    QCOMPARE(testRadius(StackBlur::maxRadius()), PixelDifferent);

    // Add extra size so the radius definitely isn't limited by image size
    setSize(StackBlur::maxKernel() + 2);
    // Test max radius unconstrained by size.
    QCOMPARE(testRadius(StackBlur::maxRadius()), PixelDifferent);
    // Test out of max radius by 1
    QCOMPARE(testRadius(StackBlur::maxRadius() + 1), PixelDifferent);
    // Test max int
    QCOMPARE(testRadius(std::numeric_limits<int>::max()), PixelDifferent);

    // Test X/Width-only
    setSize(StackBlur::maxKernel() + 2, 1);
    QCOMPARE(testRadius({8, 0}), PixelDifferent);
    QCOMPARE(testRadius({0, 8}), ExactSamePixels);

    // Test Y/Height-only
    setSize(1, StackBlur::maxKernel() + 2);
    QCOMPARE(testRadius({0, 8}), PixelDifferent);
    QCOMPARE(testRadius({8, 0}), ExactSamePixels);
}

QTEST_GUILESS_MAIN(StackBlurTest)

#include "stackblurtest.moc"
