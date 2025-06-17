/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "annotationviewport.h"
#include "annotationdocument_p.h"
#include "utils.h"

#include <QCursor>
#include <QPainter>
#include <QQuickWindow>
#include <QSGImageNode>
#include <QScreen>

static QList<AnnotationViewport *> s_viewportInstances{};
static bool s_synchronizingAnyPressed = false;
static bool s_isAnyPressed = false;

class AnnotationViewportPrivate
{
public:
    AnnotationViewport *const q = nullptr;
    QPointer<AnnotationDocument> document;
    QRectF viewportRect;
    QPointF localHoverPosition;
    QPointF localPressPosition;
    QPointF lastDocumentPressPos;
    bool isHovered = false;
    bool isPressed = false;
    bool allowDraggingSelection = false;
    bool acceptKeyReleaseEvents = false;
    QPainterPath hoveredMousePath;
    bool repaintBaseImage = true;
    bool repaintAnnotations = true;

    AnnotationViewportPrivate(AnnotationViewport *q)
        : q(q)
    {}

    QPointF inputOffset() const;
    bool shouldIgnoreInput() const;
    void setHoverPosition(const QPointF &point);
    void setHovered(bool hovered);
    void setPressPosition(const QPointF &point);
    void setPressed(bool pressed);
    void setAnyPressed();
    void setHoveredMousePath(const QPainterPath &path);
    void setCursorForToolType();
};

QPointF AnnotationViewportPrivate::inputOffset() const
{
    if (!document) {
        return viewportRect.topLeft();
    }
    return viewportRect.topLeft() + document->canvasRect().topLeft();
}

class AnnotationViewportNode : public QSGNode
{
    QSGImageNode *m_baseImageNode;
    QSGImageNode *m_annotationsNode;

public:
    AnnotationViewportNode(QSGImageNode *baseImageNode, QSGImageNode *annotationsNode)
        : QSGNode()
        , m_baseImageNode(baseImageNode)
        , m_annotationsNode(annotationsNode)
    {
        baseImageNode->setOwnsTexture(true);
        appendChildNode(baseImageNode);
        annotationsNode->setOwnsTexture(true);
        appendChildNode(annotationsNode);
    }
    QSGImageNode *baseImageNode() const
    {
        return m_baseImageNode;
    }
    QSGImageNode *annotationsNode() const
    {
        return m_annotationsNode;
    }
};

AnnotationViewport::AnnotationViewport(QQuickItem *parent)
    : QQuickItem(parent)
    , d(std::make_unique<AnnotationViewportPrivate>(this))
{
    s_viewportInstances.append(this);
    setFlags({ItemIsFocusScope, ItemHasContents, ItemIsViewport, ItemObservesViewport});
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

AnnotationViewport::~AnnotationViewport() noexcept
{
    d->setPressed(false);
    s_viewportInstances.removeOne(this);
}

QRectF AnnotationViewport::viewportRect() const
{
    return d->viewportRect;
}

void AnnotationViewport::setViewportRect(const QRectF &rect)
{
    if (rect == d->viewportRect) {
        return;
    }
    d->viewportRect = rect;
    Q_EMIT viewportRectChanged();
    d->repaintBaseImage = true;
    d->repaintAnnotations = true;
    update();
}

AnnotationDocument *AnnotationViewport::document() const
{
    return d->document;
}

void AnnotationViewport::setDocument(AnnotationDocument *doc)
{
    if (d->document == doc) {
        return;
    }

    if (d->document) {
        disconnect(d->document, nullptr, this, nullptr);
    }

    d->document = doc;
    auto repaint = [this](AnnotationDocument::RepaintTypes types) {
        using RepaintType = AnnotationDocument::RepaintType;
        if (types.testFlag(RepaintType::BaseImage)) {
            d->repaintBaseImage = true;
        }
        if (types.testFlag(RepaintType::Annotations)) {
            d->repaintAnnotations = true;
        }
        update();
    };
    connect(doc, &AnnotationDocument::repaintNeeded, this, repaint);
    connect(doc->tool(), &AnnotationTool::typeChanged, this, [this] {
        d->setCursorForToolType();
    });
    Q_EMIT documentChanged();
    update();
}

QPointF AnnotationViewport::hoverPosition() const
{
    return d->localHoverPosition;
}

void AnnotationViewportPrivate::setHoverPosition(const QPointF &point)
{
    if (localHoverPosition == point) {
        return;
    }
    localHoverPosition = point;
    Q_EMIT q->hoverPositionChanged();
}

bool AnnotationViewport::isHovered() const
{
    return d->isHovered;
}

void AnnotationViewportPrivate::setHovered(bool hovered)
{
    if (isHovered == hovered) {
        return;
    }

    isHovered = hovered;
    Q_EMIT q->hoveredChanged();
}

void setHovered(bool hovered);

QPointF AnnotationViewport::pressPosition() const
{
    return d->localPressPosition;
}

void AnnotationViewportPrivate::setPressPosition(const QPointF &point)
{
    if (localPressPosition == point) {
        return;
    }
    localPressPosition = point;
    Q_EMIT q->pressPositionChanged();
}

bool AnnotationViewport::isPressed() const
{
    return d->isPressed;
}

void AnnotationViewportPrivate::setPressed(bool pressed)
{
    if (isPressed == pressed) {
        return;
    }

    isPressed = pressed;
    Q_EMIT q->pressedChanged();
    setAnyPressed();
}

bool AnnotationViewport::isAnyPressed() const
{
    return s_isAnyPressed;
}

void AnnotationViewportPrivate::setAnyPressed()
{
    if (s_synchronizingAnyPressed || s_isAnyPressed == isPressed) {
        return;
    }
    s_synchronizingAnyPressed = true;
    // If pressed is true, anyPressed is guaranteed to be true.
    // If pressed is false, anyPressed may still be true if another viewport is pressed.
    const bool oldAnyPressed = s_isAnyPressed;
    if (isPressed) {
        s_isAnyPressed = isPressed;
    } else {
        for (const auto viewport : std::as_const(s_viewportInstances)) {
            s_isAnyPressed = viewport->isPressed();
            if (s_isAnyPressed) {
                break;
            }
        }
    }
    // Don't emit if s_isAnyPressed still hasn't changed
    if (oldAnyPressed != s_isAnyPressed) {
        for (const auto viewport : std::as_const(s_viewportInstances)) {
            Q_EMIT viewport->anyPressedChanged();
        }
    }
    s_synchronizingAnyPressed = false;
}

QPainterPath AnnotationViewport::hoveredMousePath() const
{
    return d->hoveredMousePath;
}

void AnnotationViewportPrivate::setHoveredMousePath(const QPainterPath &path)
{
    if (path == hoveredMousePath) {
        return;
    }
    hoveredMousePath = path;
    Q_EMIT q->hoveredMousePathChanged();
}

void AnnotationViewport::hoverEnterEvent(QHoverEvent *event)
{
    if (d->shouldIgnoreInput()) {
        QQuickItem::hoverEnterEvent(event);
        return;
    }
    auto position = Utils::dprRound(event->position(), window()->devicePixelRatio());
    d->setHoverPosition(position);
    d->setHovered(true);
}

void AnnotationViewport::hoverMoveEvent(QHoverEvent *event)
{
    if (d->shouldIgnoreInput()) {
        QQuickItem::hoverMoveEvent(event);
        return;
    }
    auto position = Utils::dprRound(event->position(), window()->devicePixelRatio());
    d->setHoverPosition(position);

    if (d->document->tool()->type() == AnnotationTool::SelectTool) {
        auto margin = 4;
        QRectF forgivingRect{position, QSizeF{0, 0}};
        forgivingRect.adjust(-margin, -margin, margin, margin);
        if (auto item = d->document->d->itemAt(forgivingRect.translated(d->inputOffset()))) {
            auto &interactive = std::get<Traits::Interactive::Opt>(item->traits());
            d->setHoveredMousePath(interactive->path.translated(d->inputOffset()));
        } else {
            d->setHoveredMousePath({});
        }
    } else {
        d->setHoveredMousePath({});
    }
}

void AnnotationViewport::hoverLeaveEvent(QHoverEvent *event)
{
    if (d->shouldIgnoreInput()) {
        QQuickItem::hoverLeaveEvent(event);
        return;
    }
    d->setHovered(false);
}

void AnnotationViewport::mousePressEvent(QMouseEvent *event)
{
    if (d->shouldIgnoreInput() || event->buttons() & ~acceptedMouseButtons() || event->buttons() == Qt::NoButton) {
        QQuickItem::mousePressEvent(event);
        return;
    }

    auto toolType = d->document->tool()->type();
    auto wrapper = d->document->selectedItemWrapper();
    auto pressPos = Utils::dprRound(event->position(), window()->devicePixelRatio());
    d->lastDocumentPressPos = pressPos + d->inputOffset();

    if (toolType == AnnotationTool::SelectTool) {
        auto margin = 4;
        QRectF forgivingRect{pressPos, QSizeF{0, 0}};
        forgivingRect.adjust(-margin, -margin, margin, margin);
        d->document->selectItem(forgivingRect.translated(d->inputOffset()));
    } else {
        wrapper->commitChanges();
        d->document->beginItem(d->lastDocumentPressPos);
    }

    d->allowDraggingSelection = toolType == AnnotationTool::SelectTool && wrapper->hasSelection();

    d->setHoveredMousePath({});
    d->setPressPosition(pressPos);
    d->setPressed(true);
    event->accept();
}

void AnnotationViewport::mouseMoveEvent(QMouseEvent *event)
{
    if (d->shouldIgnoreInput() || event->buttons() & ~acceptedMouseButtons() || event->buttons() == Qt::NoButton) {
        QQuickItem::mouseMoveEvent(event);
        return;
    }

    auto tool = d->document->tool();
    auto mousePos = Utils::dprRound(event->position(), window()->devicePixelRatio());
    auto wrapper = d->document->selectedItemWrapper();
    if (tool->type() == AnnotationTool::SelectTool && wrapper->hasSelection() && d->allowDraggingSelection) {
        auto documentMousePos = mousePos + d->inputOffset();
        auto dx = documentMousePos.x() - d->lastDocumentPressPos.x();
        auto dy = documentMousePos.y() - d->lastDocumentPressPos.y();
        wrapper->transform(dx, dy);
    } else if (tool->isCreationTool()) {
        using ContinueOptions = AnnotationDocument::ContinueOptions;
        using ContinueOption = AnnotationDocument::ContinueOption;
        ContinueOptions options;
        if (event->modifiers() & Qt::ShiftModifier) {
            options |= ContinueOption::Snap;
        }
        if (event->modifiers() & Qt::ControlModifier) {
            options |= ContinueOption::CenterResize;
        }
        d->document->continueItem(mousePos + d->inputOffset(), options);
    }

    d->setHoveredMousePath({});
    event->accept();
}

void AnnotationViewport::mouseReleaseEvent(QMouseEvent *event)
{
    if (d->shouldIgnoreInput() || event->button() & ~acceptedMouseButtons()) {
        QQuickItem::mouseReleaseEvent(event);
        return;
    }

    d->document->finishItem();

    auto toolType = d->document->tool()->type();
    auto wrapper = d->document->selectedItemWrapper();
    auto selectedOptions = wrapper->options();
    if (!selectedOptions.testFlag(AnnotationTool::TextOption) //
        && !d->document->isCurrentItemValid()) {
        d->document->d->popCurrentItem();
    } else if (toolType == AnnotationTool::SelectTool && wrapper->hasSelection()) {
        wrapper->commitChanges();
    } else if (!selectedOptions.testFlag(AnnotationTool::TextOption)) {
        d->document->deselectItem();
    }

    d->setPressed(false);
    event->accept();
}

void AnnotationViewport::keyPressEvent(QKeyEvent *event)
{
    // For some reason, events are already accepted when they arrive.
    QQuickItem::keyPressEvent(event);
    if (d->shouldIgnoreInput()) {
        d->acceptKeyReleaseEvents = false;
        return;
    }

    const auto wrapper = d->document->selectedItemWrapper();
    const auto selectedOptions = wrapper->options();
    const auto toolType = d->document->tool()->type();
    if (wrapper->hasSelection()) {
        if (event->matches(QKeySequence::Cancel)) {
            d->document->deselectItem();
            if (!d->document->isCurrentItemValid()) {
                d->document->d->popCurrentItem();
            }
            event->accept();
        } else if (event->matches(QKeySequence::Delete) //
                   && toolType == AnnotationTool::SelectTool //
                   && (!selectedOptions.testFlag(AnnotationTool::TextOption) || wrapper->text().isEmpty())) {
            // Only use delete shortcut when not using the text tool.
            // We don't want users trying to delete text to accidentally delete the item.
            d->document->deleteSelectedItem();
            event->accept();
        }
    }
    d->acceptKeyReleaseEvents = event->isAccepted();
}

void AnnotationViewport::keyReleaseEvent(QKeyEvent *event)
{
    // For some reason, events are already accepted when they arrive.
    if (d->shouldIgnoreInput()) {
        QQuickItem::keyReleaseEvent(event);
    } else {
        event->setAccepted(d->acceptKeyReleaseEvents);
    }
    d->acceptKeyReleaseEvents = false;
}

QSGNode *AnnotationViewport::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (!d->document || width() <= 0 || height() <= 0) {
        delete oldNode;
        return nullptr;
    }

    const auto window = this->window();
    auto node = static_cast<AnnotationViewportNode *>(oldNode);
    if (!node) {
        node = new AnnotationViewportNode(window->createImageNode(), //
                                          window->createImageNode());
        node->baseImageNode()->setFiltering(QSGTexture::Linear);
        node->annotationsNode()->setFiltering(QSGTexture::Linear);
        // Setting the mipmap filter type also enables mipmaps.
        // Super useful for scaling down smoothly.
        node->baseImageNode()->setMipmapFiltering(QSGTexture::Linear);
        node->annotationsNode()->setMipmapFiltering(QSGTexture::Linear);
    }

    const auto imageDpr = d->document->imageDpr();
    const auto windowDpr = window->effectiveDevicePixelRatio();
    const auto imageScale = windowDpr / imageDpr;
    const auto canvasRect = d->document->canvasRect();
    const auto canvasView = canvasRect.intersected(d->viewportRect.translated(canvasRect.topLeft()));
    const auto logicalImageView = canvasView.translated(-canvasRect.topLeft());
    auto windowImageSize = (logicalImageView.size() * windowDpr).toSize();
    const auto imageView = QRectF(logicalImageView.topLeft() * imageDpr, windowImageSize.toSizeF() / imageScale).toRect();
    windowImageSize = {imageView.size() * imageScale};

    auto getImage = [&](const QImage &source) -> QImage {
        const auto sourceBounds = source.rect();
        auto image = imageView == sourceBounds ? source : source.copy(imageView);
        if (image.isNull() || qFuzzyCompare(imageScale, 1)) {
            return image;
        }
        return image.scaled(windowImageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    };

    auto baseImageNode = node->baseImageNode();
    if (!baseImageNode->texture() || d->repaintBaseImage) {
        baseImageNode->setTexture(window->createTextureFromImage(getImage(d->document->canvasBaseImage())));
        d->repaintBaseImage = false;
    }

    auto annotationsNode = node->annotationsNode();
    if (!annotationsNode->texture() || d->repaintAnnotations) {
        annotationsNode->setTexture(window->createTextureFromImage(getImage(d->document->annotationsImage())));
        d->repaintAnnotations = false;
    }

    auto setupImageNode = [&](QSGImageNode *node) {
        auto size = node->texture()->textureSize().toSizeF() / windowDpr;
        if (!size.isEmpty()) {
            QPointF pos(std::round((width() - size.width()) / 2 * windowDpr) / windowDpr, //
                        std::round((height() - size.height()) / 2 * windowDpr) / windowDpr);
            node->setRect({pos, size});
        }
    };

    setupImageNode(baseImageNode);
    setupImageNode(annotationsNode);

    return node;
}

void AnnotationViewport::itemChange(ItemChange change, const ItemChangeData &value)
{
    if (change == ItemDevicePixelRatioHasChanged) {
        d->repaintBaseImage = true;
        d->repaintAnnotations = true;
        update();
    }
    QQuickItem::itemChange(change, value);
}

bool AnnotationViewportPrivate::shouldIgnoreInput() const
{
    return !q->isEnabled() || !document || document->tool()->isNoTool();
}

void AnnotationViewportPrivate::setCursorForToolType()
{
    if (document && !shouldIgnoreInput()) {
        if (document->tool()->type() == AnnotationTool::SelectTool) {
            q->setCursor(Qt::ArrowCursor);
        } else {
            q->setCursor(Qt::CrossCursor);
        }
    } else {
        q->unsetCursor();
    }
}

#include <moc_annotationviewport.cpp>
