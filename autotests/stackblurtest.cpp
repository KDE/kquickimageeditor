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

QTEST_GUILESS_MAIN(StackBlurTest)

#include "stackblurtest.moc"
