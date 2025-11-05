/* SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "annotationtool.h"
#include "annotationconfig.h"

using enum AnnotationTool::Tool;
using enum AnnotationTool::Option;

class AnnotationToolPrivate
{
public:
    static constexpr AnnotationTool::Options optionsForType(AnnotationTool::Tool type);

    static constexpr int defaultStrokeWidthForType(AnnotationTool::Tool type);
    int strokeWidthForType(AnnotationTool::Tool type) const;
    void setStrokeWidthForType(int width, AnnotationTool::Tool type);

    static constexpr QColor defaultStrokeColorForType(AnnotationTool::Tool type);
    QColor strokeColorForType(AnnotationTool::Tool type) const;
    void setStrokeColorForType(const QColor &color, AnnotationTool::Tool type);

    static constexpr QColor defaultFillColorForType(AnnotationTool::Tool type);
    QColor fillColorForType(AnnotationTool::Tool type) const;
    void setFillColorForType(const QColor &color, AnnotationTool::Tool type);

    static constexpr qreal defaultStrengthForType(AnnotationTool::Tool type);
    qreal strengthForType(AnnotationTool::Tool type) const;
    void setStrengthForType(qreal strength, AnnotationTool::Tool type);

    QFont fontForType(AnnotationTool::Tool type) const;
    void setFontForType(const QFont &font, AnnotationTool::Tool type);

    static constexpr QColor defaultFontColorForType(AnnotationTool::Tool type);
    QColor fontColorForType(AnnotationTool::Tool type) const;
    void setFontColorForType(const QColor &color, AnnotationTool::Tool type);

    bool typeHasShadow(AnnotationTool::Tool type) const;
    void setTypeHasShadow(AnnotationTool::Tool type, bool shadow);

    QRectF geometryForType(AnnotationTool::Tool type) const;
    void setGeometryForType(const QRectF &rect, AnnotationTool::Tool type);

    qreal aspectRatioForType(AnnotationTool::Tool type) const;
    void setAspectRatioforType(qreal ratio, AnnotationTool::Tool type);

    AnnotationTool::Tool type = AnnotationTool::NoTool;
    AnnotationTool::Options options = AnnotationTool::Option::NoOptions;
    int number = 1;
    QRectF cropGeometry;
    qreal cropAspectRatio = -1.0;
};

// Default value macros

#define DEFAULT_STROKE_WIDTH(ToolName) case ToolName##Tool: { return AnnotationConfig::default##ToolName##StrokeWidthValue(); }

#define DEFAULT_STROKE_COLOR(ToolName) case ToolName##Tool: { return AnnotationConfig::default##ToolName##StrokeColorValue(); }

#define DEFAULT_FILL_COLOR(ToolName) case ToolName##Tool: { return AnnotationConfig::default##ToolName##FillColorValue(); }

#define DEFAULT_FONT(ToolName) case ToolName##Tool: { return AnnotationConfig::default##ToolName##FontValue(); }

#define DEFAULT_FONT_COLOR(ToolName) case ToolName##Tool: { return AnnotationConfig::default##ToolName##FontColorValue(); }

#define DEFAULT_SHADOW(ToolName) case ToolName##Tool: { return AnnotationConfig::default##ToolName##ShadowValue(); }

// No getter macros because there's no way to lowercase the ToolName arg

// Setter macros

#define SET_STROKE_WIDTH(ToolName) case ToolName##Tool: { AnnotationConfig::set##ToolName##StrokeWidth(width); } break;

#define SET_STROKE_COLOR(ToolName) case ToolName##Tool: { AnnotationConfig::set##ToolName##StrokeColor(color); } break;

#define SET_FILL_COLOR(ToolName) case ToolName##Tool: { AnnotationConfig::set##ToolName##FillColor(color); } break;

#define SET_FONT(ToolName) case ToolName##Tool: { AnnotationConfig::set##ToolName##Font(font); } break;

#define SET_FONT_COLOR(ToolName) case ToolName##Tool: { AnnotationConfig::set##ToolName##FontColor(color); } break;

#define SET_SHADOW(ToolName) case ToolName##Tool: { AnnotationConfig::set##ToolName##Shadow(shadow); } break;

// clang-format on

AnnotationTool::AnnotationTool(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<AnnotationToolPrivate>())
{
}

AnnotationTool::~AnnotationTool()
{
    AnnotationConfig::self()->save();
};

AnnotationTool::Tool AnnotationTool::type() const
{
    return d->type;
}

void AnnotationTool::setType(AnnotationTool::Tool type)
{
    if (d->type == type) {
        return;
    }

    auto oldType = d->type;
    d->type = type;
    AnnotationConfig::setAnnotationToolType(type);
    Q_EMIT typeChanged();

    auto newOptions = AnnotationToolPrivate::optionsForType(type);
    if (d->options != newOptions) {
        d->options = newOptions;
        Q_EMIT optionsChanged();
    }

    const auto &oldStrokeWidth = d->strokeWidthForType(oldType);
    const auto &newStrokeWidth = d->strokeWidthForType(type);
    if (oldStrokeWidth != newStrokeWidth) {
        Q_EMIT strokeWidthChanged(newStrokeWidth);
    }

    const auto &oldStrokeColor = d->strokeColorForType(oldType);
    const auto &newStrokeColor = d->strokeColorForType(type);
    if (oldStrokeColor != newStrokeColor) {
        Q_EMIT strokeColorChanged(newStrokeColor);
    }

    const auto &oldFillColor = d->fillColorForType(oldType);
    const auto &newFillColor = d->fillColorForType(type);
    if (oldFillColor != newFillColor) {
        Q_EMIT fillColorChanged(newFillColor);
    }

    const auto &oldStrength = d->strengthForType(oldType);
    const auto &newStrength = d->strengthForType(type);
    if (oldStrength != newStrength) {
        Q_EMIT strengthChanged(newStrength);
    }

    const auto &oldFont = d->fontForType(oldType);
    const auto &newFont = d->fontForType(type);
    if (oldFont != newFont) {
        Q_EMIT fontChanged(newFont);
    }

    const auto &oldFontColor = d->fontColorForType(oldType);
    const auto &newFontColor = d->fontColorForType(type);
    if (oldFontColor != newFontColor) {
        Q_EMIT fontColorChanged(newFontColor);
    }

    const auto &oldShadow = d->typeHasShadow(oldType);
    const auto &newShadow = d->typeHasShadow(type);
    if (oldShadow != newShadow) {
        Q_EMIT shadowChanged(newShadow);
    }

    const auto &oldGeometry = d->geometryForType(oldType);
    const auto &newGeometry = d->geometryForType(type);
    if (oldGeometry != newGeometry) {
        Q_EMIT geometryChanged(newGeometry);
    }
}

void AnnotationTool::resetType()
{
    setType(AnnotationTool::NoTool);
}

bool AnnotationTool::isNoTool() const
{
    return d->type == NoTool;
}

bool AnnotationTool::isMetaTool() const
{
    return d->type > NoTool && d->type < FreehandTool;
}

bool AnnotationTool::isCreationTool() const
{
    return d->type > SelectTool && d->type < NTools;
}

AnnotationTool::Options AnnotationTool::options() const
{
    return d->options;
}

constexpr AnnotationTool::Options AnnotationToolPrivate::optionsForType(AnnotationTool::Tool type)
{
    switch (type) {
    case CropTool:
        return {GeometryOption, AspectRatioOption};
    case SelectTool:
        return GeometryOption;
    case HighlighterTool:
        return {StrokeOption, TransformOption};
    case FreehandTool:
    case LineTool:
    case ArrowTool:
        return {StrokeOption, ShadowOption, TransformOption};
    case RectangleTool:
    case EllipseTool:
        return {StrokeOption, ShadowOption, FillOption, TransformOption};
    case BlurTool:
        return {StrengthOption, TransformOption};
    case PixelateTool:
        return {StrengthOption, TransformOption};
    case TextTool:
        return {FontOption, TextOption, ShadowOption, TranslateOption};
    case NumberTool:
        return {FillOption, ShadowOption, FontOption, NumberOption, TranslateOption};
    default:
        return NoOptions;
    }
}

int AnnotationTool::strokeWidth() const
{
    return d->strokeWidthForType(d->type);
}

constexpr int AnnotationToolPrivate::defaultStrokeWidthForType(AnnotationTool::Tool type)
{
    switch (type) {
        DEFAULT_STROKE_WIDTH(Freehand)
        DEFAULT_STROKE_WIDTH(Highlighter)
        DEFAULT_STROKE_WIDTH(Line)
        DEFAULT_STROKE_WIDTH(Arrow)
        DEFAULT_STROKE_WIDTH(Rectangle)
        DEFAULT_STROKE_WIDTH(Ellipse)
    default:
        return 0;
    }
}

int AnnotationToolPrivate::strokeWidthForType(AnnotationTool::Tool type) const
{
    switch (type) {
    case FreehandTool:
        return AnnotationConfig::freehandStrokeWidth();
    case HighlighterTool:
        return AnnotationConfig::highlighterStrokeWidth();
    case LineTool:
        return AnnotationConfig::lineStrokeWidth();
    case ArrowTool:
        return AnnotationConfig::arrowStrokeWidth();
    case RectangleTool:
        return AnnotationConfig::rectangleStrokeWidth();
    case EllipseTool:
        return AnnotationConfig::ellipseStrokeWidth();
    default:
        return 0;
    }
}

void AnnotationTool::setStrokeWidth(int width)
{
    if (!d->options.testFlag(Option::StrokeOption) || strokeWidth() == width) {
        return;
    }

    d->setStrokeWidthForType(width, d->type);
    Q_EMIT strokeWidthChanged(width);
}

void AnnotationToolPrivate::setStrokeWidthForType(int width, AnnotationTool::Tool type)
{
    switch (type) {
        SET_STROKE_WIDTH(Freehand)
        SET_STROKE_WIDTH(Highlighter)
        SET_STROKE_WIDTH(Line)
        SET_STROKE_WIDTH(Arrow)
        SET_STROKE_WIDTH(Rectangle)
        SET_STROKE_WIDTH(Ellipse)
    default:
        break;
    }
}

void AnnotationTool::resetStrokeWidth()
{
    setStrokeWidth(d->defaultStrokeWidthForType(d->type));
}

QColor AnnotationTool::strokeColor() const
{
    return d->strokeColorForType(d->type);
}

constexpr QColor AnnotationToolPrivate::defaultStrokeColorForType(AnnotationTool::Tool type)
{
    switch (type) {
        DEFAULT_STROKE_COLOR(Freehand)
        DEFAULT_STROKE_COLOR(Highlighter)
        DEFAULT_STROKE_COLOR(Line)
        DEFAULT_STROKE_COLOR(Arrow)
        DEFAULT_STROKE_COLOR(Rectangle)
        DEFAULT_STROKE_COLOR(Ellipse)
    default:
        return Qt::transparent;
    }
}

QColor AnnotationToolPrivate::strokeColorForType(AnnotationTool::Tool type) const
{
    switch (type) {
    case FreehandTool:
        return AnnotationConfig::freehandStrokeColor();
    case HighlighterTool:
        return AnnotationConfig::highlighterStrokeColor();
    case LineTool:
        return AnnotationConfig::lineStrokeColor();
    case ArrowTool:
        return AnnotationConfig::arrowStrokeColor();
    case RectangleTool:
        return AnnotationConfig::rectangleStrokeColor();
    case EllipseTool:
        return AnnotationConfig::ellipseStrokeColor();
    default:
        return Qt::transparent;
    }
}

void AnnotationTool::setStrokeColor(const QColor &color)
{
    if (!d->options.testFlag(Option::StrokeOption) || strokeColor() == color) {
        return;
    }

    d->setStrokeColorForType(color, d->type);
    Q_EMIT strokeColorChanged(color);
}

void AnnotationToolPrivate::setStrokeColorForType(const QColor &color, AnnotationTool::Tool type)
{
    switch (type) {
        SET_STROKE_COLOR(Freehand)
        SET_STROKE_COLOR(Highlighter)
        SET_STROKE_COLOR(Line)
        SET_STROKE_COLOR(Arrow)
        SET_STROKE_COLOR(Rectangle)
        SET_STROKE_COLOR(Ellipse)
    default:
        break;
    }
}

void AnnotationTool::resetStrokeColor()
{
    setStrokeColor(d->defaultStrokeColorForType(d->type));
}

QColor AnnotationTool::fillColor() const
{
    return d->fillColorForType(d->type);
}

constexpr QColor AnnotationToolPrivate::defaultFillColorForType(AnnotationTool::Tool type)
{
    switch (type) {
        DEFAULT_FILL_COLOR(Rectangle)
        DEFAULT_FILL_COLOR(Ellipse)
        DEFAULT_FILL_COLOR(Number)
    default:
        return Qt::transparent;
    }
}

QColor AnnotationToolPrivate::fillColorForType(AnnotationTool::Tool type) const
{
    switch (type) {
    case RectangleTool:
        return AnnotationConfig::rectangleFillColor();
    case EllipseTool:
        return AnnotationConfig::ellipseFillColor();
    case NumberTool:
        return AnnotationConfig::numberFillColor();
    default:
        return Qt::transparent;
    }
}

void AnnotationTool::setFillColor(const QColor &color)
{
    if (!d->options.testFlag(Option::FillOption) || fillColor() == color) {
        return;
    }

    d->setFillColorForType(color, d->type);
    Q_EMIT fillColorChanged(color);
}

void AnnotationToolPrivate::setFillColorForType(const QColor &color, AnnotationTool::Tool type)
{
    switch (type) {
        SET_FILL_COLOR(Rectangle)
        SET_FILL_COLOR(Ellipse)
        SET_FILL_COLOR(Number)
    default:
        break;
    }
}

void AnnotationTool::resetFillColor()
{
    setFillColor(d->defaultFillColorForType(d->type));
}

qreal AnnotationTool::strength() const
{
    return d->strengthForType(d->type);
}

constexpr qreal AnnotationToolPrivate::defaultStrengthForType(AnnotationTool::Tool type)
{
    switch (type) {
    case BlurTool:
        return AnnotationConfig::defaultBlurStrengthValue();
    case PixelateTool:
        return AnnotationConfig::defaultPixelateStrengthValue();
    default:
        return 0;
    }
}

qreal AnnotationToolPrivate::strengthForType(AnnotationTool::Tool type) const
{
    switch (type) {
    case BlurTool:
        return AnnotationConfig::blurStrength();
    case PixelateTool:
        return AnnotationConfig::pixelateStrength();
    default:
        return 0;
    }
}

void AnnotationTool::setStrength(qreal strength)
{
    if (!d->options.testFlag(Option::StrengthOption) || this->strength() == strength) {
        return;
    }

    d->setStrengthForType(strength, d->type);
    Q_EMIT strengthChanged(strength);
}

void AnnotationToolPrivate::setStrengthForType(qreal strength, AnnotationTool::Tool type)
{
    switch (type) {
    case BlurTool:
        AnnotationConfig::setBlurStrength(strength);
        break;
    case PixelateTool:
        AnnotationConfig::setPixelateStrength(strength);
        break;
    default:
        break;
    }
}

void AnnotationTool::resetStrength()
{
    setStrength(d->defaultStrengthForType(d->type));
}

QFont AnnotationTool::font() const
{
    return d->fontForType(d->type);
}

QFont AnnotationToolPrivate::fontForType(AnnotationTool::Tool type) const
{
    switch (type) {
    case TextTool:
        return AnnotationConfig::textFont();
    case NumberTool:
        return AnnotationConfig::numberFont();
    default:
        return {};
    }
}

void AnnotationTool::setFont(const QFont &font)
{
    if (!d->options.testFlag(Option::FontOption) || this->font() == font) {
        return;
    }

    d->setFontForType(font, d->type);
    Q_EMIT fontChanged(font);
}

void AnnotationToolPrivate::setFontForType(const QFont &font, AnnotationTool::Tool type)
{
    switch (type) {
        SET_FONT(Text)
        SET_FONT(Number)
    default:
        break;
    }
}

void AnnotationTool::resetFont()
{
    setFont({});
}

QColor AnnotationTool::fontColor() const
{
    return d->fontColorForType(d->type);
}

constexpr QColor AnnotationToolPrivate::defaultFontColorForType(AnnotationTool::Tool type)
{
    switch (type) {
        DEFAULT_FONT_COLOR(Text)
        DEFAULT_FONT_COLOR(Number)
    default:
        return Qt::transparent;
    }
}

QColor AnnotationToolPrivate::fontColorForType(AnnotationTool::Tool type) const
{
    switch (type) {
    case TextTool:
        return AnnotationConfig::textFontColor();
    case NumberTool:
        return AnnotationConfig::numberFontColor();
    default:
        return Qt::transparent;
    }
}

void AnnotationTool::setFontColor(const QColor &color)
{
    if (!d->options.testFlag(Option::FontOption) || fontColor() == color) {
        return;
    }

    d->setFontColorForType(color, d->type);
    Q_EMIT fontColorChanged(color);
}

void AnnotationToolPrivate::setFontColorForType(const QColor &color, AnnotationTool::Tool type)
{
    switch (type) {
        SET_FONT_COLOR(Text)
        SET_FONT_COLOR(Number)
    default:
        break;
    }
}

void AnnotationTool::resetFontColor()
{
    setFontColor(d->defaultFontColorForType(d->type));
}

int AnnotationTool::number() const
{
    return d->number;
}

void AnnotationTool::setNumber(int number)
{
    if (d->number == number) {
        return;
    }

    d->number = number;
    Q_EMIT numberChanged(number);
}

void AnnotationTool::resetNumber()
{
    setNumber(1);
}

bool AnnotationToolPrivate::typeHasShadow(AnnotationTool::Tool type) const
{
    switch (type) {
    case FreehandTool:
        return AnnotationConfig::freehandShadow();
    case LineTool:
        return AnnotationConfig::lineShadow();
    case ArrowTool:
        return AnnotationConfig::arrowShadow();
    case RectangleTool:
        return AnnotationConfig::rectangleShadow();
    case EllipseTool:
        return AnnotationConfig::ellipseShadow();
    case TextTool:
        return AnnotationConfig::textShadow();
    case NumberTool:
        return AnnotationConfig::numberShadow();
    default:
        return false;
    }
}

bool AnnotationTool::hasShadow() const
{
    return d->typeHasShadow(d->type);
}

void AnnotationToolPrivate::setTypeHasShadow(AnnotationTool::Tool type, bool shadow)
{
    switch (type) {
        SET_SHADOW(Freehand)
        SET_SHADOW(Line)
        SET_SHADOW(Arrow)
        SET_SHADOW(Rectangle)
        SET_SHADOW(Ellipse)
        SET_SHADOW(Text)
        SET_SHADOW(Number)
    default:
        break;
    }
}

void AnnotationTool::setShadow(bool shadow)
{
    if (!d->options.testFlag(Option::ShadowOption) || hasShadow() == shadow) {
        return;
    }

    d->setTypeHasShadow(d->type, shadow);
    Q_EMIT shadowChanged(shadow);
}

void AnnotationTool::resetShadow()
{
    setShadow(true);
}

QRectF AnnotationToolPrivate::geometryForType(AnnotationTool::Tool type) const
{
    switch (type) {
    case CropTool:
        return cropGeometry;
    default:
        return {};
    }
}

QRectF AnnotationTool::geometry() const
{
    return d->geometryForType(d->type);
}

void AnnotationToolPrivate::setGeometryForType(const QRectF &rect, AnnotationTool::Tool type)
{
    switch (type) {
    case CropTool:
        cropGeometry = rect;
        return;
    default:
        return;
    }
}

void AnnotationTool::setGeometry(const QRectF &rect)
{
    if (!d->options.testFlag(Option::GeometryOption) || geometry() == rect) {
        return;
    }
    d->setGeometryForType(rect, d->type);
    Q_EMIT geometryChanged(rect);
}

void AnnotationTool::resetGeometry()
{
    setGeometry({});
}

qreal AnnotationToolPrivate::aspectRatioForType(AnnotationTool::Tool type) const
{
    switch (type) {
    case CropTool:
        return cropAspectRatio;
    default:
        return {};
    }
}

qreal AnnotationTool::aspectRatio() const
{
    return d->aspectRatioForType(d->type);
}

void AnnotationToolPrivate::setAspectRatioforType(qreal ratio, AnnotationTool::Tool type)
{
    switch (type) {
    case CropTool:
        cropAspectRatio = ratio;
        if (ratio > 0) {
            if (ratio >= 1) {
                cropGeometry.setHeight(cropGeometry.width() / ratio);
            } else {
                cropGeometry.setWidth(cropGeometry.height() * ratio);
            }
        }
        return;
    default:
        return;
    }
}

void AnnotationTool::setAspectRatio(qreal ratio)
{
    if (!d->options.testFlag(Option::AspectRatioOption) || aspectRatio() == ratio) {
        return;
    }
    d->setAspectRatioforType(ratio, d->type);
    Q_EMIT aspectRatioChanged(ratio);
    if (ratio > 0) {
        Q_EMIT geometryChanged(geometry());
    }
}

void AnnotationTool::resetAspectRatio()
{
    setAspectRatio(-1);
}

#include <moc_annotationtool.cpp>
