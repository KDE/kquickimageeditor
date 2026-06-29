// SPDX-FileCopyrightText: 2025 Carl Schwan <carl@carlschwan.eu>
// SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "../src/annotations/stackblur.h"
#include "stackblurtest_helpers.h"

#include <QImage>
#include <QObject>
#include <QTest>
#include <array>

using namespace Qt::StringLiterals;

struct Preset {
    QLatin1String name;
    QSize size;
};

static constexpr std::array resolutionPresets{
    Preset{"default"_L1, QSize{3840, 2160}},
    Preset{"min"_L1, QSize{StackBlur::minKernel(), StackBlur::minKernel()}},
    Preset{"icon"_L1, QSize{64, 64}},
    Preset{"vm"_L1, QSize{800, 600}}, // virtual machine
    Preset{"fhd"_L1, QSize{1920, 1080}},
    Preset{"4k"_L1, QSize{3840, 2160}},
    Preset{"8k"_L1, QSize{7680, 4320}},
    // 4k monitors, 3, landscape, horizontal
    Preset{"4k*3lh"_L1, QSize{3840 * 3, 2160}},
    // 4k monitors, 3, portrait, vertical
    Preset{"4k*3pv"_L1, QSize{2160, 3840 * 3}},
};

static constexpr std::array radiusPresets{
    Preset{"default"_L1, QSize{31, 31}},
    Preset{"min"_L1, QSize{StackBlur::maxRadius(), StackBlur::maxRadius()}},
    Preset{"max"_L1, QSize{StackBlur::maxRadius(), StackBlur::maxRadius()}},
};

constexpr bool allLowercasePresets(std::span<const Preset> presets)
{
    return std::ranges::all_of(presets, [](const Preset &preset) {
        return std::ranges::all_of(preset.name, [](char ch) {
            // Can't use islower/isLower because they're not constexpr.
            return ch < 'A' || ch > 'Z';
        });
    });
}

static_assert(allLowercasePresets(resolutionPresets), "preset names should be lowercase");
static_assert(allLowercasePresets(radiusPresets), "preset names should be lowercase");

constexpr QSize preset(QStringView string, std::span<const Preset> presets)
{
    if (string.empty() || presets.empty()) {
        return presets.empty() ? QSize{0, 0} : presets[0].size;
    }
    for (auto it = presets.begin(); it != presets.end(); ++it) {
        if (it->name == string) {
            return it->size;
        }
    }
    return QSize{0, 0};
}

// Get width and height like "100x100", "100,100" or "100" (sets both).
// Accepts decimal or hexadecimal integers. If the input isn't valid,
// you get a null image. In that case, QVERIFY(!image.isNull()) will fail.
inline QSize qSizeFromString(QStringView string)
{
    static const QRegularExpression regex(uR"(^ *([^x,\s]+)(?: *[x,] *([^x,\s]+))? *$)"_s);
    auto match = regex.matchView(string);
    int width = match.capturedView(1).toInt(nullptr, 0); // autodetect base
    // If height is 0, reuse the width value
    if (int height = match.capturedView(2).toInt(nullptr, 0)) {
        return {width, height};
    }
    return {width, width};
};

static const QSize resolution = [] {
    auto string = qEnvironmentVariable("STACKBLURBENCHMARK_RESOLUTION").trimmed().toLower();
    QSize size = preset(string, resolutionPresets);
    if (!size.isEmpty()) {
        return size;
    }
    return qSizeFromString(string);
}();

static const QSize radius = [] {
    auto string = qEnvironmentVariable("STACKBLURBENCHMARK_RADIUS").trimmed().toLower();
    QSize size = preset(string, radiusPresets);
    if (!size.isEmpty()) {
        return size;
    }
    return qSizeFromString(string);
}();

// Not actually used in here, except for printing benchmark parameters.
// This is used in the stack blur's implementation code.
static const int tileSize = [] -> int {
    auto tileSize = qEnvironmentVariableIntValue("STACKBLURBENCHMARK_TILESIZE");
    // Round to the next power of 2 down.
    return std::bit_floor(static_cast<size_t>(std::clamp(tileSize, 0, 4096)));
}();

class StackBlurBenchmark : public QObject
{
    Q_OBJECT
    void benchmarkStackBlur(QImage &image);
private Q_SLOTS:
    void benchmarkFormat_Alpha8();
    void benchmarkFormat_ARGB32_Premultiplied();
    void benchmarkFormat_RGBA64_Premultiplied();
};

void StackBlurBenchmark::benchmarkStackBlur(QImage &image)
{
    QVERIFY(!image.isNull());
    randomizeImageData(image);
    const auto [radiusX, radiusY] = radius;
    auto debugString = u"\n    Resolution: %1, Radius: [%2 %3]"_s.arg(QDebug::toString(resolution), QDebug::toString(radiusX), QDebug::toString(radiusY));
    if (tileSize) {
        debugString += u", Tile Size: %1"_s.arg(QDebug::toString(tileSize));
    }
    qDebug().noquote() << debugString;
    QBENCHMARK {
        StackBlur::blur(image, radiusX, radiusY);
        QVERIFY(!image.isNull());
    }
}

void StackBlurBenchmark::benchmarkFormat_Alpha8()
{
    QImage image(resolution, QImage::Format_Alpha8);
    benchmarkStackBlur(image);
}

void StackBlurBenchmark::benchmarkFormat_ARGB32_Premultiplied()
{
    QImage image(resolution, QImage::Format_ARGB32_Premultiplied);
    benchmarkStackBlur(image);
}

void StackBlurBenchmark::benchmarkFormat_RGBA64_Premultiplied()
{
    QImage image(resolution, QImage::Format_RGBA64_Premultiplied);
    benchmarkStackBlur(image);
}

QTEST_GUILESS_MAIN(StackBlurBenchmark)

#include "stackblurbenchmark.moc"
