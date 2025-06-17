/* SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "annotationdocument.h"
#include "history.h"

class SelectedItemWrapperPrivate
{
    friend class SelectedItemWrapper;
    friend class AnnotationDocument;
    friend class AnnotationDocumentPrivate;
public:
    SelectedItemWrapper *const q;
    AnnotationDocument *const document;
    AnnotationTool::Options options;
    HistoryItem::const_weak_ptr selectedItem;

    SelectedItemWrapperPrivate(SelectedItemWrapper *q, AnnotationDocument *document)
        : q(q)
        , document(document)
    {}

    void setSelectedItem(const HistoryItem::const_shared_ptr &item);
    // Resets the selected item, temp item and options.
    bool reset();
};

class AnnotationDocumentPrivate
{
    friend class AnnotationDocument;
    friend class SelectedItemWrapper;
    friend class SelectedItemWrapperPrivate;
    friend class AnnotationViewport;
public:
    AnnotationDocument *const q = nullptr;
    AnnotationTool *const tool = nullptr;
    SelectedItemWrapper *const selectedItemWrapper = nullptr;

    // The rectangle that contains the document area.
    QRectF canvasRect;
    // The device pixel ratio for the document's coordinate system.
    qreal imageDpr = 1;
    // An image size based on the canvas size and device pixel ratio.
    QSize imageSize{0, 0};
    // The base screenshot image
    QImage baseImage;
    // A cache for a crop of the base image.
    QImage croppedBaseImage;
    // An image containing just the annotations.
    // It is separate so that we don't need to keep repainting the image underneath.
    QImage annotationsImage;
    // The last types of things to repaint. Used to determine when to emit repaintNeeded.
    AnnotationDocument::RepaintTypes lastRepaintTypes = AnnotationDocument::RepaintType::NoTypes;
    // Where a repaint is needed. Used to determine when to repaint or emit repaintNeeded.
    QRegion repaintRegion;

    // A temporary version of the item we want to edit so we can modify at will. This will be used
    // instead of the original item when rendering, but the original item will remain in history
    // until the changes are committed.
    HistoryItem::shared_ptr tempItem;
    History history;

    AnnotationDocumentPrivate(AnnotationDocument *q)
        : q(q)
        , tool(new AnnotationTool(q))
        , selectedItemWrapper(new SelectedItemWrapper(q))
    {}

    // Set the canvas rect, device pixel ratio, image size and reset the annotation image.
    // The image size and image device pixel ratio are also set based on these.
    void setCanvas(const QRectF &rect, qreal dpr);
    void resetCanvas();

    HistoryItem::shared_ptr popCurrentItem();

    // The first item with a mouse path intersecting the specified rectangle.
    // The rectangle is meant to be used as a way to make selecting an item more forgiving
    // by adding margins around the center of where the actual target point is.
    HistoryItem::const_shared_ptr itemAt(const QRectF &rect) const;

    // Paint the section of the image intersecting the viewport.
    void paintImageView(QPainter *painter, const QImage &image, const QRectF &viewport = {}) const;

    // Paint the annotations intersecting the region.
    // The region is expected to be in image coordinates.
    // If the span is not set, all annotations intersecting the region will be painted.
    void paintAnnotations(QPainter *painter, const QRegion &imageRegion, std::optional<History::SubRange> range = std::nullopt) const;

    // Get an image that only uses a part of the history.
    QImage rangeImage(History::SubRange range) const;

    void addItem(const HistoryItem::shared_ptr &item);

    // Repaint if rect size is more than 0x0 and intersects with the canvas.
    // Takes a rectangle with document coordinates.
    // Defaults to Annotations because those are the most common.
    void setRepaintRegion(const QRectF &rect, AnnotationDocument::RepaintTypes types = AnnotationDocument::RepaintType::Annotations);
    // Unconditionally repaint. Defaults to All because that is most common for this function.
    void setRepaintRegion(AnnotationDocument::RepaintTypes types = AnnotationDocument::RepaintType::All);
};
