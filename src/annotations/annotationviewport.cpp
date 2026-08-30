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
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGTextureMaterial>
#include <QSGMaterialShader>
#include <QSGImageNode>
#include <QSGTexture>
#include <QScreen>

using namespace Qt::StringLiterals;

static QList<AnnotationViewport *> s_viewportInstances{};
static bool s_synchronizingAnyPressed = false;
static bool s_isAnyPressed = false;

using QMatrix4x4Data = float[4][4];

struct DataInfo {
    size_t offet;
    size_t size;
};

struct UniformData {
    using Ptr = std::shared_ptr<UniformData>;
    enum class DirtyFlag {
        None = 0,
        ColorMatrix = 1,
        Gamma = 1 << 1,
    };
    Q_DECLARE_FLAGS(DirtyFlags, DirtyFlag)
    static constexpr DataInfo qt_Matrix_info{0uz, sizeof(QMatrix4x4Data)};
    static constexpr DataInfo colorMatrix_info{qt_Matrix_info.offet + qt_Matrix_info.size, sizeof(QMatrix4x4Data)};
    static constexpr DataInfo gamma_info{colorMatrix_info.offet + colorMatrix_info.size, sizeof(float)};
    static constexpr DataInfo qt_Opacity_info{gamma_info.offet + gamma_info.size, sizeof(float)};

    QMatrix4x4 colorMatrix;
    float gamma = 1.0f;
    // For keeping track of which data changed
    DirtyFlags dirtyFlags = DirtyFlag::None;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(UniformData::DirtyFlags)

class BaseImageMaterialShader : public QSGMaterialShader
{
public:
    BaseImageMaterialShader()
    {
        setShaderFileName(VertexStage, ":/qt/qml/org/kde/kquickimageeditor/private/coloradjustment.vert.qsb"_L1);
        setShaderFileName(FragmentStage, ":/qt/qml/org/kde/kquickimageeditor/private/coloradjustment.frag.qsb"_L1);
    }
    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    void updateSampledImage(RenderState &state, int binding, QSGTexture **sampledTexture, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

class BaseImageMaterial : public QSGTextureMaterial
{
public:
    UniformData::Ptr uniformData;

    BaseImageMaterial(const UniformData::Ptr &uniformData)
        : uniformData(uniformData)
    {
    }
    ~BaseImageMaterial() override
    {
        delete texture();
    }
    QSGMaterialType *type() const override
    {
        static QSGMaterialType type;
        return &type;
    }
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode [[maybe_unused]]) const override
    {
        return new BaseImageMaterialShader;
    }
    int compare(const QSGMaterial *o) const override
    {
        if (auto cmp = QSGTextureMaterial::compare(o); cmp != 0) {
            return cmp;
        }
        auto other = static_cast<const BaseImageMaterial *>(o);
        if (auto diff = uniformData->gamma - other->uniformData->gamma; diff != 0) {
            return diff < 0 ? -1 : 1;
        }
        auto diffMatrix = uniformData->colorMatrix - other->uniformData->colorMatrix;
        auto matrixData = diffMatrix.constData();
        for (int i = 0; i < 16; ++i) {
            if (auto diff = matrixData[i]; diff != 0) {
                return diff < 0 ? -1 : 1;
            }
        }
        return 0;
    }
};

bool BaseImageMaterialShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = false;
    auto uniformBuffer = state.uniformData();
    const auto uniformData = uniformBuffer->data();

    if (state.isMatrixDirty()) {
        const QMatrix4x4 qt_Matrix = state.combinedMatrix();
        memcpy(uniformData + UniformData::qt_Matrix_info.offet, //
               qt_Matrix.constData(),
               UniformData::qt_Matrix_info.size);
        changed = true;
    }

    auto baseImageMaterial = static_cast<BaseImageMaterial *>(newMaterial);
    bool materialChanged = oldMaterial != newMaterial;
    auto &dirty = baseImageMaterial->uniformData->dirtyFlags;
    if (materialChanged || dirty != UniformData::DirtyFlag::None) {
        if (materialChanged || dirty.testFlag(UniformData::DirtyFlag::ColorMatrix)) {
            memcpy(uniformData + UniformData::colorMatrix_info.offet, //
                   baseImageMaterial->uniformData->colorMatrix.constData(),
                   UniformData::colorMatrix_info.size);
            dirty.setFlag(UniformData::DirtyFlag::ColorMatrix, false);
        }
        if (materialChanged || dirty.testFlag(UniformData::DirtyFlag::Gamma)) {
            memcpy(uniformData + UniformData::gamma_info.offet, //
                   &baseImageMaterial->uniformData->gamma,
                   UniformData::gamma_info.size);
            dirty.setFlag(UniformData::DirtyFlag::Gamma, false);
        }
        changed = true;
    }

    if (state.isOpacityDirty()) {
        const float qt_Opacity = state.opacity();
        memcpy(uniformData + UniformData::qt_Opacity_info.offet, //
               &qt_Opacity,
               UniformData::qt_Opacity_info.size);
        changed = true;
    }
    return changed;
}

void BaseImageMaterialShader::updateSampledImage(RenderState &renderState [[maybe_unused]], int binding, QSGTexture **sampledTexture, QSGMaterial *newMaterial, QSGMaterial *oldMaterial [[maybe_unused]])
{
    if (binding == 1) {
        auto *material = static_cast<BaseImageMaterial *>(newMaterial);
        auto *materialTexture = material->texture();
        materialTexture->setFiltering(material->filtering());
        materialTexture->setMipmapFiltering(material->mipmapFiltering());
        materialTexture->setHorizontalWrapMode(material->horizontalWrapMode());
        materialTexture->setVerticalWrapMode(material->verticalWrapMode());
        materialTexture->commitTextureOperations(renderState.rhi(), renderState.resourceUpdateBatch());
        *sampledTexture = materialTexture;
    }
}

class BaseImageNode : public QSGGeometryNode
{
    QRectF m_rect;
    static constexpr QRectF m_normalizedSourceRect{0.0, 0.0, 1.0, 1.0};

public:

    BaseImageNode(const UniformData::Ptr &uniformData)
    {
        setMaterial(new BaseImageMaterial(uniformData));
        setFlag(OwnsMaterial, true);

        QSGGeometry *g = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);
        QSGGeometry::updateTexturedRectGeometry(g, QRect(), QRect());
        setGeometry(g);
        setFlag(OwnsGeometry, true);
    }

    void setRect(const QRectF &rect)
    {
        if (rect == m_rect) {
            return;
        }
        m_rect = rect;
        QSGGeometry::updateTexturedRectGeometry(geometry(), rect, m_normalizedSourceRect);
        markDirty(DirtyGeometry);
    }

    const UniformData::Ptr &uniformData() const
    {
        return static_cast<BaseImageMaterial *>(material())->uniformData;
    }

    QSGTexture *texture() const
    {
        return static_cast<BaseImageMaterial *>(material())->texture();
    }

    void setTexture(QSGTexture *texture)
    {
        auto *m = static_cast<BaseImageMaterial *>(material());
        delete m->texture();
        m->setTexture(texture);
        markDirty(DirtyMaterial);
    }
};

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
    bool updateBaseImageTexture = true;
    bool updateAnnotationsTexture = true;
    UniformData::Ptr uniformData = std::make_shared<UniformData>();

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
    return viewportRect.topLeft();
}

class AnnotationViewportNode : public QSGNode
{
    BaseImageNode *m_baseImageNode;
    QSGImageNode *m_annotationsNode;

public:
    AnnotationViewportNode(BaseImageNode *baseImageNode, QSGImageNode *annotationsNode)
        : QSGNode()
        , m_baseImageNode(baseImageNode)
        , m_annotationsNode(annotationsNode)
    {
        appendChildNode(baseImageNode);
        annotationsNode->setOwnsTexture(true);
        appendChildNode(annotationsNode);
    }
    BaseImageNode *baseImageNode() const
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
    d->updateBaseImageTexture = true;
    d->updateAnnotationsTexture = true;
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
    auto updateTextures = [this](AnnotationDocument::RepaintTypes types) {
        using RepaintType = AnnotationDocument::RepaintType;
        d->updateBaseImageTexture |= types.testFlag(RepaintType::BaseImage);
        d->updateAnnotationsTexture |= types.testFlag(RepaintType::Annotations);
        if (d->updateBaseImageTexture || d->updateAnnotationsTexture) {
            update();
        }
    };
    connect(doc, &AnnotationDocument::repainted, this, updateTextures);
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

QMatrix4x4 AnnotationViewport::colorMatrix() const
{
    return d->uniformData->colorMatrix;
}

void AnnotationViewport::setColorMatrix(const QMatrix4x4 &matrix)
{
    if (qFuzzyCompare(d->uniformData->colorMatrix, matrix)) {
        return;
    }
    d->uniformData->colorMatrix = matrix;
    d->uniformData->dirtyFlags.setFlag(UniformData::DirtyFlag::ColorMatrix, true);
    Q_EMIT colorMatrixChanged();
}

qreal AnnotationViewport::gammaAdjustment() const
{
    // we invert because gamma stored as an inverted power
    return 1.0 / d->uniformData->gamma;
}

void AnnotationViewport::setGammaAdjustment(qreal gammaAdjustment)
{
    // we invert because gamma is applied inversely in linear colorspaces
    const auto inv = 1.0 / gammaAdjustment;
    if (!std::isfinite(inv) || Utils::fuzzyCompareF32(d->uniformData->gamma, inv)) {
        return;
    }
    d->uniformData->gamma = inv;
    d->uniformData->dirtyFlags.setFlag(UniformData::DirtyFlag::Gamma, true);
    Q_EMIT gammaAdjustmentChanged();
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
        auto transform = d->document->d->inputTransform;
        auto [dx, dy] = d->inputOffset();
        transform.translate(dx, dy);
        if (auto item = d->document->d->itemAt(transform.mapRect(forgivingRect))) {
            auto &interactive = std::get<Traits::Interactive::Opt>(item->traits());
            d->setHoveredMousePath(interactive->path);
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
    auto transform = d->document->d->inputTransform;
    auto [dx, dy] = d->inputOffset();
    transform.translate(dx, dy);
    d->lastDocumentPressPos = transform.map(pressPos);

    if (toolType == AnnotationTool::SelectTool) {
        auto margin = 4;
        QRectF forgivingRect{pressPos, QSizeF{0, 0}};
        forgivingRect.adjust(-margin, -margin, margin, margin);
        d->document->selectItem(transform.mapRect(forgivingRect));
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
    auto transform = d->document->d->inputTransform;
    auto [dx, dy] = d->inputOffset();
    transform.translate(dx, dy);
    auto wrapper = d->document->selectedItemWrapper();
    if (tool->type() == AnnotationTool::SelectTool && wrapper->hasSelection() && d->allowDraggingSelection) {
        auto documentMousePos = transform.map(mousePos);
        auto delta = wrapper->d->transform.inverted().map(documentMousePos - d->lastDocumentPressPos);
        QMatrix4x4 matrix;
        matrix.translate(delta.x(), delta.y());
        wrapper->applyTransform(matrix);
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
        d->document->continueItem(transform.map(mousePos), options);
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
        node = new AnnotationViewportNode(new BaseImageNode(d->uniformData), //
                                          window->createImageNode());
        auto baseImageMaterial = static_cast<BaseImageMaterial *>(node->baseImageNode()->material());
        baseImageMaterial->setFiltering(QSGTexture::Linear);
        node->annotationsNode()->setFiltering(QSGTexture::Linear);
        // Setting the mipmap filter type also enables mipmaps.
        // Super useful for scaling down smoothly.
        baseImageMaterial->setMipmapFiltering(QSGTexture::Linear);
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
    if (d->uniformData->dirtyFlags != UniformData::DirtyFlag::None) {
        baseImageNode->markDirty(BaseImageNode::DirtyMaterial);
    }
    if (!baseImageNode->texture() || d->updateBaseImageTexture) {
        auto image = getImage(d->document->canvasBaseImage());
        QQuickWindow::CreateTextureOptions createTextureOptions = QQuickWindow::TextureHasMipmaps;
        if (image.hasAlphaChannel()) {
            createTextureOptions |= QQuickWindow::TextureHasAlphaChannel;
        } else {
            createTextureOptions |= QQuickWindow::TextureIsOpaque;
        }
        baseImageNode->setTexture(window->createTextureFromImage(image, createTextureOptions));
        d->updateBaseImageTexture = false;
    }

    auto annotationsNode = node->annotationsNode();
    if (!annotationsNode->texture() || d->updateAnnotationsTexture) {
        annotationsNode->setTexture(window->createTextureFromImage(getImage(d->document->annotationsImage())));
        d->updateAnnotationsTexture = false;
    }

    auto setupImageNode = [&](auto *node) {
        auto size = node->texture()->textureSize().toSizeF() / windowDpr;
        if (!size.isEmpty()) {
            QPointF pos(std::round((width() - size.width()) / 2 * windowDpr) / windowDpr, //
                        std::round((height() - size.height()) / 2 * windowDpr) / windowDpr);
            node->setRect({pos, size});
        } else {
            node->setRect({});
        }
    };

    setupImageNode(baseImageNode);
    setupImageNode(annotationsNode);

    return node;
}

void AnnotationViewport::itemChange(ItemChange change, const ItemChangeData &value)
{
    if (change == ItemDevicePixelRatioHasChanged) {
        d->updateBaseImageTexture = true;
        d->updateAnnotationsTexture = true;
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
