/*
 *  SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "annotationtool.h"

#include <QColor>
#include <QFont>
#include <QImage>
#include <QMatrix4x4>
#include <QObject>
#include <QVariant>
#include <qqmlregistration.h>
#include "kquickimageeditor_export.h"

class AnnotationDocumentPrivate;
class SelectedItemWrapperPrivate;
class AnnotationTool;
class SelectedItemWrapper;
class AnnotationViewport;
class QPainter;

/**
 * This class is used to render an image with annotations. The annotations are vector graphics
 * and image effects created from a stack of history items that can be undone or redone.
 * `paint()` and `renderToImage()` will be used by clients (e.g., AnnotationViewport) to render
 * their own content. There can be any amount of clients sharing the same AnnotationDocument.
 */
class KQUICKIMAGEEDITOR_EXPORT AnnotationDocument : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(AnnotationTool *tool READ tool CONSTANT)
    Q_PROPERTY(SelectedItemWrapper *selectedItem READ selectedItemWrapper NOTIFY selectedItemWrapperChanged)

    Q_PROPERTY(int redoStackDepth READ redoStackDepth NOTIFY redoStackDepthChanged)
    Q_PROPERTY(int undoStackDepth READ undoStackDepth NOTIFY undoStackDepthChanged)
    Q_PROPERTY(QRectF canvasRect READ canvasRect NOTIFY canvasRectChanged)
    Q_PROPERTY(QSizeF imageSize READ imageSize NOTIFY imageSizeChanged)
    Q_PROPERTY(qreal imageDpr READ imageDpr NOTIFY imageDprChanged)
    Q_PROPERTY(QMatrix4x4 transform READ transform NOTIFY transformChanged)
    Q_PROPERTY(QMatrix4x4 renderTransform READ renderTransform NOTIFY transformChanged)
    Q_PROPERTY(QMatrix4x4 inputTransform READ inputTransform NOTIFY transformChanged)

public:
    enum class ContinueOption {
        NoOptions = 0,
        Snap = 1,
        CenterResize = 1 << 1,
    };
    Q_DECLARE_FLAGS(ContinueOptions, ContinueOption)
    Q_FLAG(ContinueOption)

    enum class RepaintType {
        NoTypes = 0,
        BaseImage = 1,
        Annotations = 1 << 1,
        All = BaseImage | Annotations,
    };
    Q_DECLARE_FLAGS(RepaintTypes, RepaintType)
    Q_FLAG(RepaintType)

    explicit AnnotationDocument(QObject *parent = nullptr);
    ~AnnotationDocument();

    AnnotationTool *tool() const;
    SelectedItemWrapper *selectedItemWrapper() const;

    int undoStackDepth() const;
    int redoStackDepth() const;

    QRectF canvasRect() const;

    /// Image size in raw pixels
    QSizeF imageSize() const;

    /// Image device pixel ratio
    qreal imageDpr() const;

    QImage baseImage() const;
    // Get the base image section for the current canvas rect.
    QImage canvasBaseImage() const;
    /// Set the base image. Based on the base image, also set image size, image device pixel ratio
    // and canvas rect. Cannot be undone.
    void setBaseImage(const QImage &image);

    /// Set the base image from the given file path.
    Q_INVOKABLE void setBaseImage(const QString &path);

    /// Set the base image from the given local file URL.
    /// We only support local files because QImage can only load directly from local files.
    /// This overload exists so that we don't have to convert URLs into path strings.
    Q_INVOKABLE void setBaseImage(const QUrl &localFile);

    /// Hide annotations that do not intersect with the rectangle and crop the image.
    Q_INVOKABLE void cropCanvas(const QRectF &cropRect);

    /// Get the whole image transform
    QMatrix4x4 transform() const;

    // A transform that is good for rendering annotations
    QMatrix4x4 renderTransform() const;
    // A transform that is good for processing input for annotations
    QMatrix4x4 inputTransform() const;

    /// Apply a transform and combine it with the existing transform.
    /// This is not part of the `transform` property. It adds an item to
    /// history and we don't want this to be rapidly called through bindings.
    Q_INVOKABLE void applyTransform(const QMatrix4x4 &matrix);

    /// Clear all annotations. Cannot be undone.
    Q_INVOKABLE void clearAnnotations();

    /// Clear all annotations and the image. Cannot be undone.
    Q_INVOKABLE void clear();

    // Get an image containing just the annotations.
    // This is lazily computed based on an internal paint region of areas needing to be repainted.
    QImage annotationsImage() const;

    QImage renderToImage() const;

    /// Render to an image and save it to the given path.
    Q_INVOKABLE bool saveImage(const QString &path) const;

    // True when there is an item at the end of the undo stack and it is invalid.
    bool isCurrentItemValid() const;

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    // For starting a new item
    void beginItem(const QPointF &point);
    void continueItem(const QPointF &point, AnnotationDocument::ContinueOptions options = ContinueOption::NoOptions);
    void finishItem();

    // For managing an existing item
    Q_INVOKABLE void selectItem(const QRectF &rect);
    Q_INVOKABLE void deselectItem();
    Q_INVOKABLE void deleteSelectedItem();

Q_SIGNALS:
    void selectedItemWrapperChanged();
    void undoStackDepthChanged();
    void redoStackDepthChanged();
    void canvasRectChanged();
    void imageSizeChanged();
    void imageDprChanged();
    void transformChanged();

    void repaintNeeded(AnnotationDocument::RepaintTypes types);

private:
    friend class AnnotationDocumentPrivate;
    friend class SelectedItemWrapper;
    friend class SelectedItemWrapperPrivate;
    friend class AnnotationViewport;

    std::unique_ptr<AnnotationDocumentPrivate> d;
};

/**
 * When the user selects an existing shape with the mouse, this wraps all the parameters of the associated item, so that they can be modified from QML
 */
class KQUICKIMAGEEDITOR_EXPORT SelectedItemWrapper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by AnnotationDocument")

    Q_PROPERTY(bool hasSelection READ hasSelection CONSTANT)
    Q_PROPERTY(AnnotationTool::Options options READ options CONSTANT)
    Q_PROPERTY(int strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY strokeWidthChanged)
    Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY strokeColorChanged)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)
    Q_PROPERTY(qreal strength READ strength WRITE setStrength NOTIFY strengthChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(QColor fontColor READ fontColor WRITE setFontColor NOTIFY fontColorChanged)
    Q_PROPERTY(int number READ number WRITE setNumber NOTIFY numberChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(bool shadow READ hasShadow WRITE setShadow NOTIFY shadowChanged)
    Q_PROPERTY(QPainterPath geometryPath READ geometryPath NOTIFY geometryPathChanged)
    Q_PROPERTY(QPainterPath mousePath READ mousePath NOTIFY mousePathChanged)
    Q_PROPERTY(QMatrix4x4 transform READ transform NOTIFY transformChanged)

public:
    SelectedItemWrapper(AnnotationDocument *document);
    ~SelectedItemWrapper();

    // Transform the item with the given matrix.
    // The argument will be combined with the existing transform.
    // The origin will be the center of the geometry path bounding rect.
    Q_INVOKABLE void applyTransform(const QMatrix4x4 &matrix);

    // Pushes the temporary item to history and sets the selected item as the temporary item parent.
    // Returns whether the commit actually happened.
    Q_INVOKABLE bool commitChanges();

    bool hasSelection() const;

    AnnotationTool::Options options() const;

    int strokeWidth() const;
    void setStrokeWidth(int width);

    QColor strokeColor() const;
    void setStrokeColor(const QColor &color);

    QColor fillColor() const;
    void setFillColor(const QColor &color);

    qreal strength() const;
    void setStrength(qreal strength);

    QFont font() const;
    void setFont(const QFont &font);

    QColor fontColor() const;
    void setFontColor(const QColor &color);

    int number() const;
    void setNumber(int number);

    QString text() const;
    void setText(const QString &text);

    bool hasShadow() const;
    void setShadow(bool shadow);

    QPainterPath geometryPath() const;
    QPainterPath mousePath() const;

    // The combination of all transforms applied directly to this item.
    // This is needed to know how much this item has changed.
    QMatrix4x4 transform() const;

Q_SIGNALS:
    void strokeWidthChanged();
    void strokeColorChanged();
    void fillColorChanged();
    void strengthChanged();
    void fontChanged();
    void fontColorChanged();
    void numberChanged();
    void textChanged();
    void shadowChanged();
    void geometryPathChanged();
    void mousePathChanged();
    void transformChanged();

private:
    friend class AnnotationDocument;
    friend class AnnotationDocumentPrivate;
    friend class SelectedItemWrapperPrivate;
    friend class AnnotationViewport;
    std::unique_ptr<SelectedItemWrapperPrivate> d;
};

QDebug operator<<(QDebug debug, const SelectedItemWrapper *);

Q_DECLARE_OPERATORS_FOR_FLAGS(AnnotationDocument::ContinueOptions)
Q_DECLARE_OPERATORS_FOR_FLAGS(AnnotationDocument::RepaintTypes)
