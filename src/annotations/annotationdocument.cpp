/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "annotationdocument_p.h"
#include "utils.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <memory>
#include <source_location>

using namespace Qt::StringLiterals;

QImage defaultImage(const QSize &size, qreal dpr)
{
    // RGBA is better for use with stackblur
    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    if (!image.isNull()) {
        image.setDevicePixelRatio(dpr);
        image.fill(Qt::transparent);
    }
    return image;
}

inline QRectF deviceIndependentRect(const QImage &image)
{
    return {{0, 0}, image.deviceIndependentSize()};
}

AnnotationDocument::AnnotationDocument(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<AnnotationDocumentPrivate>(this))
{
}

AnnotationDocument::~AnnotationDocument() = default;

AnnotationTool *AnnotationDocument::tool() const
{
    return d->tool;
}

SelectedItemWrapper *AnnotationDocument::selectedItemWrapper() const
{
    return d->selectedItemWrapper;
}

int AnnotationDocument::undoStackDepth() const
{
    return d->history.undoList().size();
}

int AnnotationDocument::redoStackDepth() const
{
    return d->history.redoList().size();
}

bool AnnotationDocument::isModified() const
{
    return d->history.isModified();
}

void AnnotationDocument::setModified(bool modified)
{
    if (modified == d->history.isModified()) {
        return;
    }
    const auto undoList = d->history.undoList();
    const auto &item = modified || undoList.empty() ? nullptr : undoList.back();
    if (d->history.setUnmodifiedId(item)) {
        Q_EMIT modifiedChanged();
    }
}

QRectF AnnotationDocument::canvasRect() const
{
    return d->canvasRect;
}

void AnnotationDocumentPrivate::setCanvas(const QRectF &rect, qreal dpr, const std::optional<QMatrix4x4> &newTransform)
{
    // Don't allow an invalid canvas rect or device pixel ratio.
    if (rect.isEmpty()) {
        qWarning() << '`' << std::source_location::current().function_name() << "`:\n\t`rect` is empty. This should not happen.";
        return;
    } else if (dpr <= 0) {
        qWarning() << '`' << std::source_location::current().function_name() << "`:\n\t`dpr` <= 0. This should not happen.";
        return;
    }
    const bool posChanged = canvasRect.topLeft() != rect.topLeft();
    const bool sizeChanged = canvasRect.size() != rect.size();
    const bool dprChanged = imageDpr != dpr;
    const bool transformChanged = newTransform && *newTransform != transform;
    if (posChanged || sizeChanged) {
        canvasRect = rect;
        Q_EMIT q->canvasRectChanged();
    }
    if (dprChanged) {
        imageDpr = dpr;
        Q_EMIT q->imageDprChanged();
    }
    if (sizeChanged || dprChanged) {
        imageSize = (rect.size() * dpr).toSize();
        Q_EMIT q->imageSizeChanged();
    }
    if (transformChanged) {
        transform = *newTransform;
        invertedTransform = transform.inverted();
    }
    if (transformChanged || posChanged) {
        auto [dx, dy] = -invertedTransform.map(canvasRect.topLeft());
        renderTransform = transform;
        renderTransform.translate(dx, dy);
        inputTransform = invertedTransform;
        inputTransform.translate(canvasRect.x(), canvasRect.y());
        Q_EMIT q->transformChanged();
    }
    // Reset image cache
    baseImageCache = [this]() -> QImage {
        if (baseImage.isNull()) {
            return {};
        }
        auto imageRect = deviceIndependentRect(baseImage);
        const auto untransformedCanvasRect = invertedTransform.mapRect(canvasRect);
        auto image = baseImage;
        if (!untransformedCanvasRect.contains(imageRect)) {
            imageRect = Utils::rectScaled(untransformedCanvasRect.intersected(imageRect), imageDpr);
            image = image.copy(imageRect.toRect());
        }
        if (transform.isIdentity()) {
            return image;
        }
        return image.transformed(transform.toTransform(), Qt::SmoothTransformation);
    }();
    annotationsImage = defaultImage(imageSize, imageDpr);
    annotationsImage = q->annotationsImage();
    // Unconditionally repaint the whole canvas area
    setRepaintRegion();
}

QSizeF AnnotationDocument::imageSize() const
{
    return d->imageSize;
}

qreal AnnotationDocument::imageDpr() const
{
    return d->imageDpr;
}

QImage AnnotationDocument::baseImage() const
{
    return d->baseImage;
}

QImage AnnotationDocument::canvasBaseImage() const
{
    if (d->baseImage.isNull() || d->baseImageCache.isNull()) {
        return d->baseImage;
    }
    return d->baseImageCache;
}

void AnnotationDocument::setBaseImage(const QImage &image)
{
    if (d->baseImage.cacheKey() == image.cacheKey()) {
        return;
    }
    d->baseImage = image;
    d->setCanvas(deviceIndependentRect(d->baseImage), d->baseImage.devicePixelRatio(), QTransform{});
}

void AnnotationDocument::setBaseImage(const QString &path)
{
    setBaseImage(QImage(path));
}

void AnnotationDocument::setBaseImage(const QUrl &localFile)
{
    setBaseImage(QImage(localFile.toLocalFile()));
}

void AnnotationDocument::cropCanvas(const QRectF &cropRect)
{
    // Can't crop to nothing
    if (cropRect.isEmpty()) {
        return;
    }

    // In the UI, (0,0) for the crop rect will be the top left of the current canvas rect.
    // A crop can only make the canvas smaller.
    auto newCanvasRect = cropRect.translated(d->canvasRect.topLeft()).intersected(d->canvasRect);
    if (newCanvasRect == d->canvasRect) {
        return;
    }

    auto newItem = std::make_shared<HistoryItem>();
    QPainterPath path;
    path.addRect(newCanvasRect);
    std::get<Traits::Geometry::Opt>(newItem->traits()).emplace(path);
    std::get<Traits::Meta::Crop::Opt>(newItem->traits()).emplace();
    const auto &undoList = d->history.undoList();
    for (auto it = std::ranges::crbegin(undoList); it != std::ranges::crend(undoList); ++it) {
        const auto item = *it;
        if (!item) {
            continue;
        }
        if (std::get<Traits::Meta::Crop::Opt>(item->traits()).has_value()) {
            HistoryItem::setItemRelations(item, newItem);
            break;
        }
    }
    d->setCanvas(newCanvasRect, d->imageDpr);
    d->addItem(newItem);
}

QMatrix4x4 AnnotationDocument::transform() const
{
    return d->transform;
}

QMatrix4x4 AnnotationDocument::renderTransform() const
{
    return d->renderTransform;
}

QMatrix4x4 AnnotationDocument::inputTransform() const
{
    return d->inputTransform;
}

void AnnotationDocumentPrivate::setTransform(const QMatrix4x4 &newTransform)
{
    if (transform == newTransform) {
        return;
    }
    // NOTE: the order of arguments for operator* is important.
    // With a different order, the wrong scale/shear would be applied to translations.
    auto diffTransform = invertedTransform * newTransform;
    setCanvas(diffTransform.mapRect(canvasRect), imageDpr, newTransform);
}

void AnnotationDocument::applyTransform(const QMatrix4x4 &matrix)
{
    if (matrix.isIdentity()) {
        return;
    }
    auto newItem = std::make_shared<HistoryItem>();
    // NOTE: the order of arguments for operator* is important.
    // With a different order, the wrong scale/shear would be applied to translations.
    auto &transform = std::get<Traits::Meta::Transform::Opt>(newItem->traits()).emplace(d->transform * matrix);
    const auto &undoList = d->history.undoList();
    for (auto it = std::ranges::crbegin(undoList); it != std::ranges::crend(undoList); ++it) {
        const auto &item = *it;
        if (!item) {
            continue;
        }
        if (std::get<Traits::Meta::Transform::Opt>(item->traits()).has_value()) {
            HistoryItem::setItemRelations(item, newItem);
            break;
        }
    }
    d->addItem(newItem);
    d->setTransform(transform);
}

void AnnotationDocument::clearAnnotations()
{
    auto wasModified = d->history.isModified();
    d->setTransform({});
    auto result = d->history.clearLists();
    d->tool->resetType();
    d->tool->resetNumber();
    deselectItem();
    if (result.undoListChanged) {
        Q_EMIT undoStackDepthChanged();
    }
    if (result.redoListChanged) {
        Q_EMIT redoStackDepthChanged();
    }
    if (wasModified != d->history.isModified()) {
        Q_EMIT modifiedChanged();
    }
    d->setRepaintRegion(RepaintType::Annotations);
}

void AnnotationDocument::clear()
{
    clearAnnotations();
    setBaseImage(QImage{});
}

void AnnotationDocumentPrivate::paintImageView(QPainter *painter, const QImage &image, const QRectF &viewport) const
{
    if (!painter || image.isNull()) {
        return;
    }
    // Enable smooth transform for fractional scales.
    painter->setRenderHint(QPainter::SmoothPixmapTransform, fmod(imageDpr, 1) != 0);
    if (viewport.isNull()) {
        painter->drawImage(QPointF{0, 0}, image);
    } else {
        painter->drawImage({0, 0}, image, Utils::rectScaled(viewport, imageDpr));
    }
}

void AnnotationDocumentPrivate::paintAnnotations(QPainter *painter, const QRegion &region, std::optional<History::SubRange> range) const
{
    if (!painter || region.isEmpty()) {
        return;
    }
    const auto &undoList = history.undoList();
    if (undoList.empty() || (range && range->empty())) {
        return;
    }
    if (!range) {
        range.emplace(undoList);
    }

    const auto begin = range->begin();
    const auto end = range->end();
    // Only highlighter needs the base image to be rendered underneath itself to function correctly.
    const bool hasHighlighter = std::any_of(begin, end, [this, &region](const HistoryItem::const_shared_ptr &item) {
        const auto &renderedItem = item == selectedItemWrapper->d->selectedItem ? tempItem : item;
        if (!renderedItem) {
            return false;
        }
        auto &visual = std::get<Traits::Visual::Opt>(renderedItem->traits());
        if (!visual) {
            return false;
        }
        return std::get<Traits::Highlight::Opt>(renderedItem->traits()).has_value() //
            && history.itemVisible(item) && region.intersects(visual->rect.toAlignedRect());
    });
    if (hasHighlighter) {
        bool hasDifferentClip = false;
        QRegion oldRegion;
        if (painter->hasClipping()) {
            oldRegion = painter->clipRegion();
            hasDifferentClip = oldRegion != region;
            if (hasDifferentClip) {
                painter->setClipRegion(region);
            }
        }
        auto transform = painter->transform();
        painter->setTransform({});
        paintImageView(painter, q->canvasBaseImage());
        painter->setTransform(transform);
        if (hasDifferentClip) {
            painter->setClipRegion(oldRegion);
        }
    }
    for (auto it = begin; it != end; ++it) {
        const auto item = *it;
        if (!history.itemVisible(item)) {
            continue;
        }
        // Render the temporary item instead if this item is selected.
        const auto isSelected = item == selectedItemWrapper->d->selectedItem;
        const auto &renderedItem = isSelected ? tempItem : item;
        if (!renderedItem) {
            continue;
        }
        auto &visual = std::get<Traits::Visual::Opt>(renderedItem->traits());
        if (!region.intersects(visual->rect.toAlignedRect())) {
            continue;
        }

        painter->setRenderHints({QPainter::Antialiasing, QPainter::TextAntialiasing});
        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::NoBrush);

        auto &highlight = std::get<Traits::Highlight::Opt>(renderedItem->traits());
        painter->setCompositionMode(highlight ? highlight->compositionMode : QPainter::CompositionMode_SourceOver);

        // Draw the shadow if existent
        auto &shadow = std::get<Traits::Shadow::Opt>(renderedItem->traits());
        if (shadow && shadow->enabled) {
            QImage image = Utils::shapeShadow(renderedItem->traits());
            painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
            painter->drawImage(visual->rect, image);
        }

        auto &geometry = std::get<Traits::Geometry::Opt>(renderedItem->traits());
        if (auto &fillOpt = std::get<Traits::Fill::Opt>(renderedItem->traits())) {
            using namespace Traits;
            auto &fill = fillOpt.value();
            switch (fill.index()) {
            case Fill::Brush:
                painter->setBrush(std::get<Fill::Brush>(fill));
                painter->drawPath(geometry->path);
                break;
            case Traits::Fill::Blur: {
                auto &blur = std::get<Fill::Blur>(fill);
                auto untilNow = History::SubRange{begin, it};
                auto getImage = [this, untilNow] {
                    return rangeImage(untilNow);
                };
                const auto &rect = geometry->path.boundingRect();
                const auto &image = blur.image(getImage, rect, imageDpr);
                painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
                painter->drawImage(rect, image);
            } break;
            case Traits::Fill::Pixelate: {
                auto &pixelate = std::get<Fill::Pixelate>(fill);
                auto untilNow = History::SubRange{begin, it};
                auto getImage = [this, untilNow] {
                    return rangeImage(untilNow);
                };
                const auto &rect = geometry->path.boundingRect();
                const auto &image = pixelate.image(getImage, rect, imageDpr);
                painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
                painter->drawImage(rect, image);
            } break;
            default:
                break;
            }
        }

        if (auto &stroke = std::get<Traits::Stroke::Opt>(renderedItem->traits())) {
            painter->setBrush(stroke->pen.brush());
            painter->drawPath(stroke->path);
        }

        if (auto &text = std::get<Traits::Text::Opt>(renderedItem->traits())) {
            painter->setBrush(Qt::NoBrush);
            painter->setPen(text->brush.color());
            painter->setFont(text->font);
            painter->drawText(geometry->path.boundingRect(), text->textFlags(), text->text());
        }
    }
}

QImage AnnotationDocument::annotationsImage() const
{
    if (d->annotationsImage.isNull()) {
        return {};
    }
    if (!d->repaintRegion.isEmpty()) {
        QPainter painter(&d->annotationsImage);
        painter.setTransform(d->renderTransform.toTransform());
        // Set clip region to prevent over-painting shadows or semi-transparent annotations near the region.
        painter.setClipRegion(d->repaintRegion);
        // Clear mode is needed to actually clear the region.
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        // The painter is clipped to the region, so we can just use eraseRect.
        painter.eraseRect(d->repaintRegion.boundingRect());
        // Restore default composition mode.
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        d->paintAnnotations(&painter, d->repaintRegion);
        painter.end();
        d->repaintRegion = {};
    }
    return d->annotationsImage;
}

QImage AnnotationDocument::renderToImage() const
{
    auto image = canvasBaseImage();
    QPainter painter(&image);
    d->paintImageView(&painter, annotationsImage());
    painter.end();
    return image;
}

bool AnnotationDocument::saveImage(const QString &path) const
{
    return renderToImage().save(path);
}

QImage AnnotationDocumentPrivate::rangeImage(History::SubRange range) const
{
    auto image = baseImage;
    QPainter p(&image);
    paintAnnotations(&p, deviceIndependentRect(image).toAlignedRect(), range);
    p.end();
    return image;
}

bool AnnotationDocument::isCurrentItemValid() const
{
    return d->history.currentItem() && d->history.currentItem()->isValid();
}

HistoryItem::shared_ptr AnnotationDocumentPrivate::popCurrentItem()
{
    auto wasModified = history.isModified();
    auto result = history.pop();
    if (result.item) {
        if (result.item == selectedItemWrapper->d->selectedItem.lock()) {
            q->deselectItem();
        }
        Q_EMIT q->undoStackDepthChanged();
        setRepaintRegion(result.item->renderRect());
    }
    if (result.redoListChanged) {
        Q_EMIT q->redoStackDepthChanged();
    }
    if (wasModified != history.isModified()) {
        Q_EMIT q->modifiedChanged();
    }
    return result.item;
}

HistoryItem::const_shared_ptr AnnotationDocumentPrivate::itemAt(const QRectF &rect) const
{
    const auto &undoList = history.undoList();
    // Precisely the first time so that users can get exactly what they click.
    for (auto it = std::ranges::crbegin(undoList); it != std::ranges::crend(undoList); ++it) {
        const auto item = *it;
        if (history.itemVisible(item)) {
            auto &interactive = std::get<Traits::Interactive::Opt>(item->traits());
            if (interactive->path.contains(rect.center())) {
                return item;
            }
        }
    }
    // If rect has no width or height
    if (rect.isNull()) {
        return nullptr;
    }
    // Forgiving if that failed so that you don't need to be perfect.
    for (auto it = std::ranges::crbegin(undoList); it != std::ranges::crend(undoList); ++it) {
        const auto item = *it;
        if (history.itemVisible(item)) {
            QPainterPath path(rect.topLeft());
            path.addEllipse(rect);
            auto &interactive = std::get<Traits::Interactive::Opt>(item->traits());
            if (interactive->path.intersects(path)) {
                return item;
            }
        }
    }
    return nullptr;
}

void AnnotationDocument::undo()
{
    const auto &undoList = d->history.undoList();
    const auto undoCount = undoList.size();
    if (!undoCount) {
        return;
    }

    auto wasModified = d->history.isModified();
    auto currentItem = d->history.currentItem();
    auto prevItem = undoCount > 1 ? undoList[undoCount - 2] : nullptr;
    d->setRepaintRegion(currentItem->renderRect());
    if (prevItem) {
        d->setRepaintRegion(prevItem->renderRect());
    }
    if (auto text = std::get<Traits::Text::Opt>(currentItem->traits())) {
        if (text->index() == Traits::Text::Number) {
            d->tool->setNumber(std::get<Traits::Text::Number>(text.value()));
        }
    }
    if (std::get<Traits::Meta::Transform::Opt>(currentItem->traits())) {
        auto parent = currentItem->parent().lock();
        if (parent) {
            d->setTransform(std::get<Traits::Meta::Transform::Opt>(parent->traits()).value_or(Traits::Meta::Transform{}));
        } else {
            d->setTransform({});
        }
    }
    if (std::get<Traits::Meta::Crop::Opt>(currentItem->traits()).has_value()) {
        auto parent = currentItem->parent().lock();
        if (parent) {
            d->setCanvas(Traits::geometryPathBounds(parent->traits()), d->imageDpr);
        } else {
            d->setCanvas(deviceIndependentRect(d->baseImage), d->imageDpr);
        }
    }
    if (currentItem == d->selectedItemWrapper->d->selectedItem.lock()) {
        if (prevItem && currentItem->hasParent() && (prevItem == currentItem->parent())) {
            d->selectedItemWrapper->d->setSelectedItem(prevItem);
        } else {
            deselectItem();
        }
    }
    d->history.undo();

    Q_EMIT undoStackDepthChanged();
    Q_EMIT redoStackDepthChanged();
    if (wasModified != d->history.isModified()) {
        Q_EMIT modifiedChanged();
    }
}

void AnnotationDocument::redo()
{
    const auto &redoList = d->history.redoList();
    if (redoList.empty()) {
        return;
    }

    auto wasModified = d->history.isModified();
    auto currentItem = d->history.currentItem();
    auto nextItem = *std::ranges::crbegin(redoList);
    d->setRepaintRegion(nextItem->renderRect());
    if (currentItem) {
        d->setRepaintRegion(currentItem->renderRect());
    }
    if (auto text = std::get<Traits::Text::Opt>(nextItem->traits())) {
        if (text->index() == Traits::Text::Number) {
            d->tool->setNumber(std::get<Traits::Text::Number>(text.value()) + 1);
        }
    }
    if (auto &transform = std::get<Traits::Meta::Transform::Opt>(nextItem->traits())) {
        d->setTransform(std::get<Traits::Meta::Transform::Opt>(nextItem->traits()).value_or(Traits::Meta::Transform{}));
    }
    if (std::get<Traits::Meta::Crop::Opt>(nextItem->traits()).has_value()) {
        d->setCanvas(Traits::geometryPathBounds(nextItem->traits()), d->imageDpr);
    }
    if (currentItem && currentItem == d->selectedItemWrapper->d->selectedItem) {
        if (nextItem == currentItem->child()) {
            d->selectedItemWrapper->d->setSelectedItem(nextItem);
        } else {
            deselectItem();
        }
    }
    d->history.redo();

    Q_EMIT undoStackDepthChanged();
    Q_EMIT redoStackDepthChanged();
    if (wasModified != d->history.isModified()) {
        Q_EMIT modifiedChanged();
    }
}

bool isAnyOfToolType(AnnotationTool::Tool type, auto... args)
{
    return ((type == args) || ...);
}

void AnnotationDocument::beginItem(const QPointF &point)
{
    if (!d->tool->isCreationTool()) {
        return;
    }

    auto wasModified = d->history.isModified();
    // if the last item was not valid, discard it (for instance a rectangle with 0 size)
    if (!isCurrentItemValid()) {
        auto result = d->history.pop();
        if (result.item) {
            d->setRepaintRegion(result.item->renderRect());
        }
    }

    using enum AnnotationTool::Tool;
    using enum AnnotationTool::Option;
    HistoryItem temp;
    auto &geometry = std::get<Traits::Geometry::Opt>(temp.traits());
    geometry.emplace(QPainterPath{point});
    auto &interactive = std::get<Traits::Interactive::Opt>(temp.traits());
    interactive.emplace(QPainterPath{point});
    auto &visual = std::get<Traits::Visual::Opt>(temp.traits());
    visual.emplace(QRectF{point, point});

    auto toolType = d->tool->type();
    auto toolOptions = d->tool->options();
    if (toolType == BlurTool) {
        auto &fill = std::get<Traits::Fill::Opt>(temp.traits());
        fill.emplace(Traits::ImageEffects::Blur{d->tool->strength()});
    } else if (toolType == PixelateTool) {
        auto &fill = std::get<Traits::Fill::Opt>(temp.traits());
        fill.emplace(Traits::ImageEffects::Pixelate{d->tool->strength()});
    } else if (toolOptions.testFlag(FillOption)) {
        auto &fill = std::get<Traits::Fill::Opt>(temp.traits());
        fill.emplace(d->tool->fillColor());
    }

    if (toolOptions.testFlag(StrokeOption)) {
        auto &stroke = std::get<Traits::Stroke::Opt>(temp.traits());
        auto pen = Traits::Stroke::defaultPen();
        pen.setBrush(d->tool->strokeColor());
        pen.setWidthF(d->tool->strokeWidth());
        stroke.emplace(pen);
    }

    if (toolOptions.testFlag(ShadowOption)) {
        auto &shadow = std::get<Traits::Shadow::Opt>(temp.traits());
        shadow.emplace(d->tool->hasShadow());
    }

    if (isAnyOfToolType(toolType, FreehandTool, HighlighterTool)) {
        geometry->path = Traits::minPath(geometry->path);
    }
    if (toolType == HighlighterTool) {
        std::get<Traits::Highlight::Opt>(temp.traits()).emplace();
    } else if (toolType == ArrowTool) {
        std::get<Traits::Arrow::Opt>(temp.traits()).emplace();
    } else if (toolType == NumberTool) {
        std::get<Traits::Text::Opt>(temp.traits()).emplace(d->tool->number(), d->tool->fontColor(), d->tool->font());
        d->tool->setNumber(d->tool->number() + 1);
    } else if (toolType == TextTool) {
        std::get<Traits::Text::Opt>(temp.traits()).emplace(QString{}, d->tool->fontColor(), d->tool->font());
    }

    Traits::initOptTuple(temp.traits());

    auto newItem = std::make_shared<HistoryItem>(std::move(temp));
    d->setRepaintRegion(newItem->renderRect());
    d->addItem(newItem);
    d->selectedItemWrapper->d->setSelectedItem(newItem);
    if (!wasModified) {
        Q_EMIT modifiedChanged();
    }
}

void AnnotationDocument::continueItem(const QPointF &point, ContinueOptions options)
{
    const auto &currentItem = d->history.currentItem();
    bool isSelected = d->selectedItemWrapper->d->selectedItem == currentItem;
    const auto &item = isSelected ? d->tempItem : currentItem;
    if (!d->tool->isCreationTool() || !item || !Traits::canBeVisible(item->traits())) {
        return;
    }

    d->setRepaintRegion(item->renderRect());
    auto &geometry = std::get<Traits::Geometry::Opt>(item->traits());
    auto &path = geometry->path;
    const auto toolType = d->tool->type();
    switch (toolType) {
    case AnnotationTool::FreehandTool:
    case AnnotationTool::HighlighterTool: {
        const auto lastIndex = path.elementCount() - 1;
        const auto lastElement = path.elementAt(lastIndex);
        if (options & ContinueOption::Snap) {
            if (!lastElement.isLineTo()) {
                // Make a line if we don't have one
                path.lineTo(point);
            }
            path.setElementPositionAt(lastIndex, point.x(), point.y());
        } else {
            // smooth path as we go.
            path.quadTo(lastElement, (lastElement + point) / 2);
        }
        if (auto &stroke = std::get<Traits::Stroke::Opt>(item->traits()); //
            stroke && toolType == AnnotationTool::HighlighterTool) {
            bool flatCap = options & ContinueOption::Snap && path.elementCount() == 2;
            stroke->pen.setCapStyle(flatCap ? Qt::FlatCap : Qt::RoundCap);
        }
    } break;
    case AnnotationTool::LineTool:
    case AnnotationTool::ArrowTool: {
        auto count = path.elementCount();
        auto lastElement = path.elementAt(count - 1);
        QPointF endPoint = point;
        if (options & ContinueOption::Snap) {
            const auto &prevElement = count > 1 ? path.elementAt(count - 2) : lastElement;
            QPointF posDiff = point - prevElement;
            if (qAbs(posDiff.x()) / 1.5 > qAbs(posDiff.y())) {
                // horizontal
                endPoint.setY(prevElement.y);
            } else if (qAbs(posDiff.x()) < qAbs(posDiff.y()) / 1.5) {
                // vertical
                endPoint.setX(prevElement.x);
            } else {
                // diagonal when 1/3 in between horizontal and vertical
                auto xSign = std::copysign(1.0, posDiff.x());
                auto ySign = std::copysign(1.0, posDiff.y());
                qreal max = qMax(qAbs(posDiff.x()), qAbs(posDiff.y()));
                endPoint = prevElement + QPointF(max * xSign, max * ySign);
            }
        }
        if (count > 1 && !lastElement.isMoveTo()) {
            path.setElementPositionAt(count - 1, endPoint.x(), endPoint.y());
        } else {
            path.lineTo(endPoint);
        }
    } break;
    case AnnotationTool::RectangleTool:
    case AnnotationTool::EllipseTool:
    case AnnotationTool::BlurTool:
    case AnnotationTool::PixelateTool: {
        const auto count = path.elementCount();
        // We always make the real start point the last point so we can easily keep it without
        // needing to keep a separate point or rectangle. Qt automatically moves the first MoveTo
        // element if one exists, so we can't just keep it at the start.
        auto start = path.currentPosition();
        // Can have a negative size with bottom right being visually top left.
        QRectF rect(start, point);
        if (options & ContinueOption::Snap) {
            auto wSign = std::copysign(1.0, rect.width());
            auto hSign = std::copysign(1.0, rect.height());
            qreal max = qMax(qAbs(rect.width()), qAbs(rect.height()));
            rect.setSize({max * wSign, max * hSign});
        }

        if (options & ContinueOption::CenterResize) {
            if (count > 1) {
                auto oldBounds = path.boundingRect();
                rect.moveCenter(oldBounds.center());
            } else {
                rect.moveCenter(start);
            }
        }
        path.clear();
        if (d->tool->type() == AnnotationTool::EllipseTool) {
            path.addEllipse(rect);
        } else {
            path.addRect(rect);
        }
        // the top left is now the real start point
        path.moveTo(rect.topLeft());
    } break;
    case AnnotationTool::TextTool: {
        const auto count = path.elementCount();
        auto rect = path.boundingRect();
        if (count == 1) {
            // BUG: boundingRect won't have the correct position if the only element is a MoveTo.
            // Fixed in https://codereview.qt-project.org/c/qt/qtbase/+/534966.
            rect.moveTo(path.elementAt(0));
        }
        path.translate(point - QPointF{rect.x(), rect.center().y()});
    } break;
    case AnnotationTool::NumberTool: {
        const auto count = path.elementCount();
        auto rect = path.boundingRect();
        if (count == 1) {
            // BUG: boundingRect won't have the correct position if the only element is a MoveTo.
            // Fixed in https://codereview.qt-project.org/c/qt/qtbase/+/534966.
            rect.moveTo(path.elementAt(0));
        }
        path.translate(point - rect.center());
    } break;
    default:
        return;
    }

    Traits::clearForInit(item->traits());
    Traits::fastInitOptTuple(item->traits());

    if (isSelected) {
        *currentItem = *item;
        d->selectedItemWrapper->d->reset();
        d->selectedItemWrapper->d->setSelectedItem(currentItem);
    }
    d->setRepaintRegion(item->renderRect());
}

void AnnotationDocument::finishItem()
{
    const auto &currentItem = d->history.currentItem();
    bool isSelected = d->selectedItemWrapper->d->selectedItem == currentItem;
    const auto &item = isSelected ? d->tempItem : currentItem;
    if (!d->tool->isCreationTool() || !item || !Traits::canBeVisible(item->traits())) {
        return;
    }

    Traits::initOptTuple(item->traits());
    if (isSelected) {
        *currentItem = *item;
        d->selectedItemWrapper->d->reset();
        d->selectedItemWrapper->d->setSelectedItem(currentItem);
        Q_EMIT selectedItemWrapperChanged(); // re-evaluate qml bindings
    }
}

void AnnotationDocument::selectItem(const QRectF &rect)
{
    d->selectedItemWrapper->d->setSelectedItem(d->itemAt(rect));
}

void AnnotationDocument::deselectItem()
{
    d->selectedItemWrapper->d->setSelectedItem(nullptr);
}

void AnnotationDocument::deleteSelectedItem()
{
    auto selectedItem = d->selectedItemWrapper->d->selectedItem.lock();
    if (!selectedItem) {
        return;
    }

    auto newItem = std::make_shared<HistoryItem>();
    HistoryItem::setItemRelations(selectedItem, newItem);
    std::get<Traits::Meta::Delete::Opt>(newItem->traits()).emplace();
    d->addItem(newItem);
    deselectItem();
    d->setRepaintRegion(selectedItem->renderRect());
}

void AnnotationDocumentPrivate::addItem(const HistoryItem::shared_ptr &item)
{
    auto wasModified = history.isModified();
    auto result = history.push(item);
    if (result.undoListChanged) {
        Q_EMIT q->undoStackDepthChanged();
    }
    if (result.redoListChanged) {
        Q_EMIT q->redoStackDepthChanged();
    }
    if (wasModified != history.isModified()) {
        Q_EMIT q->modifiedChanged();
    }
}

void AnnotationDocumentPrivate::setRepaintRegion(const QRectF &rect, AnnotationDocument::RepaintTypes types)
{
    if (rect.isNull() || !canvasRect.intersects(transform.mapRect(rect))) {
        // No point in trying to transform or add to the region if not in the canvas rect.
        return;
    }
    // HACK: workaround not always repainting everywhere it should with fractional scaling.
    auto biggerRect = rect.normalized().adjusted(-1, -1, 0, 0).toAlignedRect();
    /* QRegion has a QRect overload for operator+=, but not for operator|=.
     *
     * `region += rect` is not the same as `region = region.united(rect)`. It will try to add the
     * rect directly instead of making a copy of itself with the rect added.
     *
     * QRegion only works with ints, so we need to convert the rect to image coordinates or ensure
     * The region contains a bit more than the rect with toAlignedRect.
     * We normalize the rect because operator+= will no-op if `rect.isEmpty()`.
     * `QRectF::isEmpty()` is true when the size is 0 or negative.
     */
    if (!canvasRect.intersects(transform.mapRect(biggerRect))) {
        // No point in trying to transform or add to the region if true.
        return;
    }
    const bool emitRepaintNeeded = repaintRegion.isEmpty() || lastRepaintTypes != types;
    repaintRegion += biggerRect;
    lastRepaintTypes = types;
    if (emitRepaintNeeded) {
        Q_EMIT q->repaintNeeded(lastRepaintTypes);
    }
}

void AnnotationDocumentPrivate::setRepaintRegion(AnnotationDocument::RepaintTypes types)
{
    const bool emitRepaintNeeded = repaintRegion.isEmpty() || lastRepaintTypes != types;
    repaintRegion = invertedTransform.mapRect(canvasRect).toAlignedRect();
    lastRepaintTypes = types;
    if (emitRepaintNeeded) {
        Q_EMIT q->repaintNeeded(lastRepaintTypes);
    }
}

//////////////////////////

SelectedItemWrapper::SelectedItemWrapper(AnnotationDocument *document)
    : QObject(document)
    , d(std::make_unique<SelectedItemWrapperPrivate>(this, document))
{
}

SelectedItemWrapper::~SelectedItemWrapper() = default;

void SelectedItemWrapperPrivate::setSelectedItem(const HistoryItem::const_shared_ptr &historyItem)
{
    if (selectedItem == historyItem //
        || (historyItem && !Traits::canBeVisible(historyItem->traits()))) {
        return;
    }

    selectedItem = historyItem;
    if (historyItem) {
        auto &temp = document->d->tempItem;
        temp = std::make_shared<HistoryItem>(*historyItem);
        options.setFlag(AnnotationTool::StrokeOption, //
                          std::get<Traits::Stroke::Opt>(temp->traits()).has_value());

        auto &fill = std::get<Traits::Fill::Opt>(temp->traits());
        options.setFlag(AnnotationTool::FillOption, //
                          fill.has_value() && fill->index() == Traits::Fill::Brush);
        options.setFlag(AnnotationTool::StrengthOption, //
                          fill.has_value()
                              && (fill->index() == Traits::Fill::Blur //
                                  || fill->index() == Traits::Fill::Pixelate));

        auto &text = std::get<Traits::Text::Opt>(temp->traits());
        options.setFlag(AnnotationTool::FontOption, text.has_value());
        options.setFlag(AnnotationTool::TextOption, //
                          text && text->index() == Traits::Text::String);
        options.setFlag(AnnotationTool::NumberOption, //
                          text && text->index() == Traits::Text::Number);

        options.setFlag(AnnotationTool::ShadowOption, //
                          std::get<Traits::Shadow::Opt>(temp->traits()).has_value());
        transform = {};
    } else {
        reset();
    }
    // all bindings using the selectedItem property should be re-evalulated when emitted
    Q_EMIT document->selectedItemWrapperChanged();
}

void SelectedItemWrapper::applyTransform(const QMatrix4x4 &matrix)
{
    auto selectedItem = d->selectedItem.lock();
    auto &temp = d->document->d->tempItem;
    if (!selectedItem || !temp || matrix.isIdentity()) {
        return;
    }
    d->document->d->setRepaintRegion(temp->renderRect());
    auto appliedTransform = matrix.toTransform();
    if (appliedTransform.type() == QTransform::TxTranslate) {
        // This is less expensive since we don't regenerate stroke or mousePath when translating.
        Traits::transformTraits(appliedTransform, temp->traits());
    } else {
        auto &path = std::get<Traits::Geometry::Opt>(temp->traits())->path;
        // origin for transformation
        const auto [ox, oy] = path.boundingRect().center();
        // Eliminate unintentional translation.
        // It's unintuitive, but this applies the translation without scaling/shearing it.
        appliedTransform *= QTransform::fromTranslate(ox, oy);
        // This does a scaled/sheared translation.
        appliedTransform.translate(-ox, -oy);
        path = appliedTransform.map(path);
        Traits::reInitTraits(temp->traits());
    }
    // NOTE: the order of arguments for operator* is important.
    // With a different order, the wrong scale/shear would be applied to translations.
    d->transform = d->transform * matrix;
    d->transform.optimize();
    d->document->d->setRepaintRegion(temp->renderRect());
    Q_EMIT transformChanged();
    Q_EMIT geometryPathChanged();
    Q_EMIT mousePathChanged();
}

bool SelectedItemWrapper::commitChanges()
{
    auto selectedItem = d->selectedItem.lock();
    auto &temp = d->document->d->tempItem;
    if (!selectedItem || !temp || !temp->isValid() //
        || temp->traits() == selectedItem->traits()) {
        return false;
    }

    if (!selectedItem->isValid() && selectedItem == d->document->d->history.currentItem()) {
        auto wasModified = d->document->isModified();
        auto result = d->document->d->history.pop();
        if (result.redoListChanged) {
            Q_EMIT d->document->redoStackDepthChanged();
        }
        if (!wasModified) {
            Q_EMIT d->document->modifiedChanged();
        }
    } else {
        HistoryItem::setItemRelations(selectedItem, temp);
    }
    d->document->d->addItem(temp);
    d->setSelectedItem(temp);
    return true;
}

bool SelectedItemWrapperPrivate::reset()
{
    auto &temp = document->d->tempItem;
    if (!q->hasSelection() && options == AnnotationTool::NoOptions) {
        return {};
    }
    bool selectionChanged = false;
    auto selectedItem = this->selectedItem.lock();
    if (selectedItem) {
        selectionChanged = true;
        document->d->setRepaintRegion(selectedItem->renderRect());
    }
    if (temp) {
        document->d->setRepaintRegion(temp->renderRect());
    }
    temp.reset();
    this->selectedItem.reset();
    options = AnnotationTool::NoOptions;
    transform = {};
    // Not emitting selectedItemWrapperChanged.
    // Use the return value to determine if that should be done when necessary.
    return selectionChanged;
}

bool SelectedItemWrapper::hasSelection() const
{
    return !d->selectedItem.expired() && d->document->d->tempItem;
}

AnnotationTool::Options SelectedItemWrapper::options() const
{
    return d->options;
}

int SelectedItemWrapper::strokeWidth() const
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::StrokeOption) || !temp) {
        return 0;
    }
    auto &stroke = std::get<Traits::Stroke::Opt>(temp->traits());
    return stroke->pen.widthF();
}

void SelectedItemWrapper::setStrokeWidth(int width)
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::StrokeOption) || !temp) {
        return;
    }
    auto &stroke = std::get<Traits::Stroke::Opt>(temp->traits());
    if (stroke->pen.widthF() == width) {
        return;
    }
    d->document->d->setRepaintRegion(temp->renderRect());
    stroke->pen.setWidthF(width);
    Traits::reInitTraits(temp->traits());
    d->document->d->setRepaintRegion(temp->renderRect());
    Q_EMIT strokeWidthChanged();
    Q_EMIT geometryPathChanged();
    Q_EMIT mousePathChanged();
}

QColor SelectedItemWrapper::strokeColor() const
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::StrokeOption) || !temp) {
        return {};
    }
    auto &stroke = std::get<Traits::Stroke::Opt>(temp->traits());
    return stroke->pen.color();
}

void SelectedItemWrapper::setStrokeColor(const QColor &color)
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::StrokeOption) || !temp) {
        return;
    }
    auto &stroke = std::get<Traits::Stroke::Opt>(temp->traits());
    if (stroke->pen.color() == color) {
        return;
    }
    stroke->pen.setColor(color);
    Q_EMIT strokeColorChanged();
    d->document->d->setRepaintRegion(temp->renderRect());
}

QColor SelectedItemWrapper::fillColor() const
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::FillOption) || !temp) {
        return {};
    }
    auto &fill = std::get<Traits::Fill::Opt>(temp->traits()).value();
    auto &brush = std::get<Traits::Fill::Brush>(fill);
    return brush.color();
}

void SelectedItemWrapper::setFillColor(const QColor &color)
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::FillOption) || !temp) {
        return;
    }
    auto &fill = std::get<Traits::Fill::Opt>(temp->traits()).value();
    auto &brush = std::get<Traits::Fill::Brush>(fill);
    if (brush.color() == color) {
        return;
    }
    brush = color;
    Q_EMIT fillColorChanged();
    d->document->d->setRepaintRegion(temp->renderRect());
}

qreal SelectedItemWrapper::strength() const
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::StrengthOption) || !temp) {
        return {};
    }
    auto &fill = std::get<Traits::Fill::Opt>(temp->traits()).value();
    if (auto blur = std::get_if<Traits::Fill::Blur>(&fill)) {
        return blur->strength();
    } else if (auto pixelate = std::get_if<Traits::Fill::Pixelate>(&fill)) {
        return pixelate->strength();
    }
    return 0;
}

void SelectedItemWrapper::setStrength(qreal strength)
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::StrengthOption) || !temp) {
        return;
    }
    auto &fill = std::get<Traits::Fill::Opt>(temp->traits()).value();
    if (auto blur = std::get_if<Traits::Fill::Blur>(&fill); blur && blur->strength() != strength) {
        blur->setStrength(strength);
        Q_EMIT strengthChanged();
        d->document->d->setRepaintRegion(temp->renderRect());
    } else if (auto pixelate = std::get_if<Traits::Fill::Pixelate>(&fill); pixelate && pixelate->strength() != strength) {
        pixelate->setStrength(strength);
        Q_EMIT strengthChanged();
        d->document->d->setRepaintRegion(temp->renderRect());
    }
}

QFont SelectedItemWrapper::font() const
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::FontOption) || !temp) {
        return {};
    }
    auto &text = std::get<Traits::Text::Opt>(temp->traits());
    return text->font;
}

void SelectedItemWrapper::setFont(const QFont &font)
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::FontOption) || !temp) {
        return;
    }
    auto &text = std::get<Traits::Text::Opt>(temp->traits());
    if (text->font == font) {
        return;
    }
    d->document->d->setRepaintRegion(temp->renderRect());
    text->font = font;
    Traits::reInitTraits(temp->traits());
    d->document->d->setRepaintRegion(temp->renderRect());
    Q_EMIT fontChanged();
    Q_EMIT geometryPathChanged();
    Q_EMIT mousePathChanged();
}

QColor SelectedItemWrapper::fontColor() const
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::FontOption) || !temp) {
        return {};
    }
    auto &text = std::get<Traits::Text::Opt>(temp->traits());
    return text->brush.color();
}

void SelectedItemWrapper::setFontColor(const QColor &color)
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::FontOption) || !temp) {
        return;
    }
    auto &text = std::get<Traits::Text::Opt>(temp->traits());
    if (text->brush.color() == color) {
        return;
    }
    text->brush = color;
    Q_EMIT fontColorChanged();
    d->document->d->setRepaintRegion(temp->renderRect());
}

int SelectedItemWrapper::number() const
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::NumberOption) || !temp) {
        return 0;
    }
    auto &text = std::get<Traits::Text::Opt>(temp->traits());
    const auto *number = std::get_if<Traits::Text::Number>(&text.value());
    return number ? *number : 0;
}

void SelectedItemWrapper::setNumber(int number)
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::NumberOption) || !temp) {
        return;
    }
    auto &text = std::get<Traits::Text::Opt>(temp->traits());
    const auto *oldNumber = std::get_if<Traits::Text::Number>(&text.value());
    if (!oldNumber || *oldNumber == number) {
        return;
    }
    d->document->d->setRepaintRegion(temp->renderRect());
    text.value().emplace<Traits::Text::Number>(number);
    Traits::reInitTraits(temp->traits());
    d->document->d->setRepaintRegion(temp->renderRect());
    Q_EMIT numberChanged();
    Q_EMIT geometryPathChanged();
    Q_EMIT mousePathChanged();
}

QString SelectedItemWrapper::text() const
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::TextOption) || !temp) {
        return {};
    }
    auto &text = std::get<Traits::Text::Opt>(temp->traits());
    const auto *string = std::get_if<Traits::Text::String>(&text.value());
    return string ? *string : QString{};
}

void SelectedItemWrapper::setText(const QString &string)
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::TextOption) || !temp) {
        return;
    }
    auto &text = std::get<Traits::Text::Opt>(temp->traits());
    const auto *oldString = std::get_if<Traits::Text::String>(&text.value());
    if (!oldString || *oldString == string) {
        return;
    }
    d->document->d->setRepaintRegion(temp->renderRect());
    text.value().emplace<Traits::Text::String>(string);
    Traits::reInitTraits(temp->traits());
    d->document->d->setRepaintRegion(temp->renderRect());
    Q_EMIT textChanged();
    Q_EMIT geometryPathChanged();
    Q_EMIT mousePathChanged();
}

bool SelectedItemWrapper::hasShadow() const
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::ShadowOption) || !temp) {
        return false;
    }
    auto &shadow = std::get<Traits::Shadow::Opt>(temp->traits());
    return shadow ? shadow->enabled : false;
}

void SelectedItemWrapper::setShadow(bool enabled)
{
    auto &temp = d->document->d->tempItem;
    if (!d->options.testFlag(AnnotationTool::ShadowOption) || !temp) {
        return;
    }
    auto &shadow = std::get<Traits::Shadow::Opt>(temp->traits());
    if (shadow->enabled == enabled) {
        return;
    }
    d->document->d->setRepaintRegion(temp->renderRect());
    shadow->enabled = enabled;
    Traits::reInitTraits(temp->traits());
    d->document->d->setRepaintRegion(temp->renderRect());
    Q_EMIT shadowChanged();
}

QPainterPath SelectedItemWrapper::geometryPath() const
{
    auto &temp = d->document->d->tempItem;
    if (!hasSelection()) {
        return {};
    }
    return Traits::geometryPath(temp->traits());
}

QPainterPath SelectedItemWrapper::mousePath() const
{
    auto &temp = d->document->d->tempItem;
    if (!hasSelection()) {
        return {};
    }
    return Traits::interactivePath(temp->traits());
}

QMatrix4x4 SelectedItemWrapper::transform() const
{
    return d->transform;
}

QDebug operator<<(QDebug debug, const SelectedItemWrapper *wrapper)
{
    QDebugStateSaver stateSaver(debug);
    debug.nospace();
    debug << "SelectedItemWrapper(";
    if (!wrapper) {
        return debug << "0x0)";
    }
    debug << (const void *)wrapper;
    // debug << ",\n  selectedItem=" << wrapper->d->selectedItem().lock().get();
    debug << ')';
    return debug;
}

#include <moc_annotationdocument.cpp>
