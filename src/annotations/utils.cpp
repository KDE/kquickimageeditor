#include "utils.h"

#include "stackblur.h"
#include "traits.h"

#include <QImage>
#include <QPainter>

Utils::Utils(QObject *parent)
    : QObject(parent)
{
}

template<>
QImage Utils::shapeShadow(const Traits::OptTuple &traits, qreal devicePixelRatio)
{
    auto &shadowTrait = std::get<Traits::Shadow::Opt>(traits);
    if (!shadowTrait || !Traits::isVisible(traits)) {
        return QImage();
    }

    auto &geometryTrait = std::get<Traits::Geometry::Opt>(traits);
    auto &visualTrait = std::get<Traits::Visual::Opt>(traits);
    auto round = [&](qreal value) -> int {
        // HACK: the stack blur doesn't render horizontal shadow edges well
        // unless the image size is a multiple of 4.
        return std::max(4.0, std::ceil(value * devicePixelRatio / 4) * 4);
    };
    QImage shadow(round(visualTrait->rect.width()), //
                    round(visualTrait->rect.height()),
                    QImage::Format_ARGB32_Premultiplied);
    shadow.setDevicePixelRatio(devicePixelRatio); // also scales QPainter
    shadow.fill(Qt::transparent);
    QPainter p(&shadow);
    p.setRenderHint(QPainter::Antialiasing);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::NoBrush);
    p.translate(-visualTrait->rect.topLeft() //
                + QPointF{Traits::Shadow::xOffset, Traits::Shadow::yOffset});

    static constexpr auto alpha = 0.5;
    // Convenience var so we don't keep multiplying alpha by 255.
    static constexpr uint8_t alpha8bit = alpha * 255;

    auto &fillTrait = std::get<Traits::Fill::Opt>(traits);
    auto &strokeTrait = std::get<Traits::Stroke::Opt>(traits);
    auto *fillBrush = fillTrait && fillTrait->isValid() //
        ? std::get_if<Traits::Fill::Brush>(&fillTrait.value())
        : nullptr;
    bool hasStroke = strokeTrait && strokeTrait->isValid();
    // No need to draw fill and stroke separately if they're both opaque
    if (fillBrush && hasStroke && fillBrush->isOpaque() && strokeTrait->pen.brush().isOpaque()) {
        p.setBrush(QColor(0, 0, 0, alpha8bit));
        p.drawPath((strokeTrait->path | geometryTrait->path).simplified());
    } else {
        if (fillBrush) {
            p.setBrush(QColor(0, 0, 0, std::ceil(alpha8bit * fillBrush->color().alphaF())));
            p.drawPath(geometryTrait->path);
        }
        if (strokeTrait) {
            p.setBrush(QColor(0, 0, 0, std::ceil(alpha8bit * strokeTrait->pen.color().alphaF())));
            p.drawPath(strokeTrait->path);
        }
    }

    auto &textTrait = std::get<Traits::Text::Opt>(traits);
    // No need to paint text/number shadow if fill is opaque.
    if ((!fillTrait || (fillBrush && !fillBrush->isOpaque())) && textTrait) {
        p.setFont(textTrait->font);
        p.setBrush(Qt::NoBrush);
        p.setPen(Qt::black);
        // Color emojis don't get semi-transparent shadows with a semi-transparent pen.
        // setOpacity disables sub-pixel text antialiasing, but we don't need sub-pixel AA here.
        p.setOpacity(alpha * textTrait->brush.color().alphaF());
        p.drawText(geometryTrait->path.boundingRect(), textTrait->textFlags(), textTrait->text());
    }
    p.end();
    const int radius = qRound(Traits::Shadow::radius * devicePixelRatio);
    // We only want black shadows with opacity, so we only need black and 8 bits of alpha.
    // If we don't do this, color emojis won't have black semi-transparent shadows.
    shadow.convertTo(QImage::Format_Alpha8);
    // Blur after converting to save CPU cycles.
    StackBlur::blur(shadow, radius, radius);
    return shadow;
}

#include "moc_utils.cpp"
