// SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "../src/annotations/coloradjustment.h"
#include "stackblurtest_helpers.h"
#include <QColor>
#include <QColorSpace>
#include <QImage>
#include <QObject>
#include <QTest>

using namespace Qt::StringLiterals;

class ColorAdjustmentTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void test();
};

void ColorAdjustmentTest::test()
{
    using enum ImageComparison::Result;
    std::string debugString; // needed for QVERIFY2
    auto setDebugString = [&debugString]<typename Expected = void>(const auto &gotValue, const auto &forOther, const Expected *expected = nullptr) {
        if constexpr (!std::same_as<Expected, void>) {
            debugString =
                u"Got %1 for %2\n    Expected: %3"_s.arg(QDebug::toString(gotValue), QDebug::toString(forOther), QDebug::toString(*expected)).toStdString();
        } else {
            debugString = u"Got %1 for %2"_s.arg(QDebug::toString(gotValue), QDebug::toString(forOther)).toStdString();
        }
    };

    // Set up test matrices
    using Flag = QMatrix4x4::Flag;
    const QMatrix4x4 identityMatrix{};
    const QMap<QMatrix4x4::Flag, QMatrix4x4> matrices{
        {Flag::Translation,
         [] {
             QMatrix4x4 m;
             m.translate(0.125f, 0.25f, 0.375f);
             return m;
         }()},
        {Flag::Scale,
         [] {
             QMatrix4x4 m;
             m.scale(0.875f, 0.75f, 0.625f);
             return m;
         }()},
        {Flag::Rotation2D,
         [] {
             QMatrix4x4 m;
             m.rotate(45.0f, 0.0f, 0.0f, 1.0f);
             return m;
         }()},
        {Flag::Rotation,
         [] {
             QMatrix4x4 m;
             m.rotate(45.0f, 1.0f, 1.0f, 1.0f);
             return m;
         }()},
        {Flag::General,
         [] {
             QMatrix4x4 m;
             m.setRow(3, {0.25f, 0.25f, 0.25f, 1.0f});
             return m;
         }()},
    };
    // Make sure the matrices have been set correctly for the test
    for (auto it = matrices.begin(); it != matrices.end(); ++it) {
        QCOMPARE(it->flags(), it.key());
    }

    const QColorSpace sRGB = QColorSpace::SRgb;
    constexpr auto customGamma = 1.23456f;
    constexpr std::array colors{
        QColor(128, 0, 0, 192), // red 50%, alpha 75%
        QColor(0, 128, 0, 192), // green 50%, alpha 75%
        QColor(0, 0, 128, 192), // blue 50%, alpha 75%
        QColor(128, 128, 128, 192), // gray 50%, alpha 75%
    };
    constexpr auto width = 2;
    constexpr auto height = 2;
    auto getImage = [&](QImage::Format format) {
        QImage image(width, height, format);
        image.setColorSpace(sRGB);
        image.setPixelColor(0, 0, colors[0]);
        image.setPixelColor(1, 0, colors[1]);
        image.setPixelColor(0, 1, colors[2]);
        image.setPixelColor(1, 1, colors[3]);
        return image;
    };
    auto format = QImage::Format_RGBA64_Premultiplied;
    QImage image1 = getImage(format);
    QVERIFY(!image1.isNull());
    setDebugString(image1.colorSpace(), format);
    QVERIFY2(image1.colorSpace() == sRGB, debugString.c_str());
    QImage image2;
    const QMatrix4x4 *lastMatrix = &identityMatrix;
    auto testMatrix = [&](const QMatrix4x4 &matrix) {
        lastMatrix = &matrix;
        image2 = image1;
        ColorAdjustment::adjust(image2, matrix, sRGB.gamma() / sRGB.gamma());
        auto result = ImageComparison::compare<QRgb>(image1, image2);
        setDebugString(result, std::tuple{format, matrix});
        return result;
    };
    auto testGamma = [&]() {
        lastMatrix = &identityMatrix;
        image2 = image1;
        ColorAdjustment::adjust(image2, identityMatrix, customGamma / sRGB.gamma());
        auto result = ImageComparison::compare<QRgb>(image1, image2);
        setDebugString(result, std::tuple{format, sRGB.gamma(), customGamma});
        return result;
    };
    auto testMatrixWithGamma = [&](const QMatrix4x4 &matrix) {
        lastMatrix = &matrix;
        image2 = image1;
        ColorAdjustment::adjust(image2, matrix, customGamma / sRGB.gamma());
        auto result = ImageComparison::compare<QRgb>(image1, image2);
        setDebugString(result, std::tuple{format, matrix, sRGB.gamma(), customGamma});
        return result;
    };

    // with no change
    {
        const auto result = testMatrix(identityMatrix);
        QVERIFY2(result == ExactSamePixels, debugString.c_str());
    }

    // with matrix
    for (auto it = matrices.begin(); it != matrices.end(); ++it) {
        const auto result = testMatrix(*it);
        QVERIFY2(result == PixelDifferent, debugString.c_str());
    }

    // with new gamma
    {
        const auto result = testGamma();
        QVERIFY2(result == PixelDifferent, debugString.c_str());
    }

    // with matrix and new gamma
    for (auto it = matrices.begin(); it != matrices.end(); ++it) {
        const auto result = testMatrixWithGamma(*it);
        QVERIFY2(result == PixelDifferent, debugString.c_str());
    }

    // Make sure format is kept
    setDebugString(image2.format(), format);
    QVERIFY2(image2.format() == format, debugString.c_str());

    QImage img("/home/noah/Pictures/Screenshots/Screenshot_20260714_225601.png"_L1, "PNG");
    img.convertTo(QImage::Format_ARGB32_Premultiplied);
    QMatrix4x4 mat;
    mat.translate(0.1, 0.1, 0.1);
    ColorAdjustment::adjust(img, mat);
    img.save("./test.png"_L1, "PNG");
}

QTEST_GUILESS_MAIN(ColorAdjustmentTest)

#include "coloradjustmenttest.moc"
