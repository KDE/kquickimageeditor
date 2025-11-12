/* SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>
 * SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QColor>
#include <QFont>
#include <QObject>
#include <qqmlregistration.h>
#include "kquickimageeditor_export.h"

class AnnotationToolPrivate;

/*!
 * \inqmlmodule org.kde.kquickimageeditor
 * \qmltype AnnotationTool
 * \brief The data structure that controls the creation of the next item.
 *
 * From QML its parameter will be set by the app toolbars, and then drawing on the
 * screen with the mouse will lead to the creation of a new item based on those parameters.
 */
class KQUICKIMAGEEDITOR_EXPORT AnnotationTool : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by AnnotationDocument")

    /*!
     * \qmlproperty Tool AnnotationTool::type
     */
    Q_PROPERTY(Tool type READ type WRITE setType RESET resetType NOTIFY typeChanged)
    /*!
     * \qmlproperty bool AnnotationTool::isNoTool
     */
    Q_PROPERTY(bool isNoTool READ isNoTool NOTIFY typeChanged)
    /*!
     * \qmlproperty bool AnnotationTool::isMetaTool
     */
    Q_PROPERTY(bool isMetaTool READ isMetaTool NOTIFY typeChanged)
    /*!
     * \qmlproperty bool AnnotationTool::isCreationTool
     */
    Q_PROPERTY(bool isCreationTool READ isCreationTool NOTIFY typeChanged)
    /*!
     * \qmlproperty Options AnnotationTool::options
     */
    Q_PROPERTY(Options options READ options NOTIFY optionsChanged)
    /*!
     * \qmlproperty int AnnotationTool::strokeWidth
     */
    Q_PROPERTY(int strokeWidth READ strokeWidth WRITE setStrokeWidth RESET resetStrokeWidth NOTIFY strokeWidthChanged)
    /*!
     * \qmlproperty color AnnotationTool::strokeColor
     */
    Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor RESET resetStrokeColor NOTIFY strokeColorChanged)
    /*!
     * \qmlproperty color AnnotationTool::fillColor
     */
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor RESET resetFillColor NOTIFY fillColorChanged)
    /*!
     * \qmlproperty real AnnotationTool::strength
     */
    Q_PROPERTY(qreal strength READ strength WRITE setStrength RESET resetStrength NOTIFY strengthChanged)
    /*!
     * \qmlproperty font AnnotationTool::font
     */
    Q_PROPERTY(QFont font READ font WRITE setFont RESET resetFont NOTIFY fontChanged)
    /*!
     * \qmlproperty color AnnotationTool::fontColor
     */
    Q_PROPERTY(QColor fontColor READ fontColor WRITE setFontColor RESET resetFontColor NOTIFY fontColorChanged)
    /*!
     * \qmlproperty int AnnotationTool::number
     */
    Q_PROPERTY(int number READ number WRITE setNumber RESET resetNumber NOTIFY numberChanged)
    /*!
     * \qmlproperty bool AnnotationTool::shadow
     */
    Q_PROPERTY(bool shadow READ hasShadow WRITE setShadow RESET resetShadow NOTIFY shadowChanged)
    Q_PROPERTY(QRectF geometry READ geometry WRITE setGeometry RESET resetGeometry NOTIFY geometryChanged)
    Q_PROPERTY(qreal aspectRatio READ aspectRatio WRITE setAspectRatio RESET resetAspectRatio NOTIFY aspectRatioChanged)

public:
    /*!
     * \qmlproperty enumeration AnnotationTool::Tool
     * These tools are meant to be shown as selectable tool types in the UI.
     * They can also affect the types of traits a drawable object is allowed to have.
     * \value NoTool
     * \value SelectTool
     * \value FreehandTool
     * \value HighlighterTool
     * \value LineTool
     * \value ArrowTool
     * \value RectangleTool
     * \value EllipseTool
     * \value BlurTool
     * \value PixelateTool
     * \value TextTool
     * \value NumberTool
     * \value NTools
     */
    enum Tool {
        NoTool,
        // Meta tools
        CropTool,
        SelectTool,
        // Creation tools
        FreehandTool,
        HighlighterTool,
        LineTool,
        ArrowTool,
        RectangleTool,
        EllipseTool,
        BlurTool,
        PixelateTool,
        TextTool,
        NumberTool,
        NTools,
    };
    Q_ENUM(Tool)

    /*!
     * \qmlproperty enumeration AnnotationTool::Option
     * These options are meant to help control which options are visible in the UI
     * and what kinds of traits a drawable object should have.
     * \value NoOptions
     * \value StrokeOption
     * \value FillOption
     * \value StrengthOption
     * \value FontOption
     * \value TextOption
     * \value NumberOption
     * \value ShadowOption
     */
    enum Option {
        NoOptions = 0,
        StrokeOption = 1,
        FillOption = 1 << 1,
        StrengthOption = 1 << 2,
        FontOption = 1 << 3,
        TextOption = 1 << 4,
        NumberOption = 1 << 5,
        ShadowOption = 1 << 6,
        GeometryOption = 1 << 7,
        AspectRatioOption = 1 << 8,
        TranslateOption = 1 << 9,
        ResizeOption = 1 << 10,
        RotateOption = 1 << 11,
        TransformOption = TranslateOption | ResizeOption | RotateOption,
    };
    Q_DECLARE_FLAGS(Options, Option)
    Q_FLAG(Options)

    AnnotationTool(QObject *parent);
    ~AnnotationTool();

    Tool type() const;
    void setType(Tool type);
    void resetType();

    bool isNoTool() const;

    // Whether the current tool type is for modifying the document's attributes.
    bool isMetaTool() const;

    // Whether the current tool type is for creating annotation objects.
    bool isCreationTool() const;

    Options options() const;

    int strokeWidth() const;
    void setStrokeWidth(int width);
    void resetStrokeWidth();

    QColor strokeColor() const;
    void setStrokeColor(const QColor &color);
    void resetStrokeColor();

    QColor fillColor() const;
    void setFillColor(const QColor &color);
    void resetFillColor();

    qreal strength() const;
    void setStrength(qreal strength);
    void resetStrength();

    QFont font() const;
    void setFont(const QFont &font);
    void resetFont();

    QColor fontColor() const;
    void setFontColor(const QColor &color);
    void resetFontColor();

    int number() const;
    void setNumber(int number);
    void resetNumber();

    bool hasShadow() const;
    void setShadow(bool shadow);
    void resetShadow();

    QRectF geometry() const;
    void setGeometry(const QRectF &rect);
    void resetGeometry();

    qreal aspectRatio() const;
    void setAspectRatio(qreal ratio);
    void resetAspectRatio();

Q_SIGNALS:
    void typeChanged();
    void optionsChanged();
    void strokeWidthChanged(int width);
    void strokeColorChanged(const QColor &color);
    void fillColorChanged(const QColor &color);
    void strengthChanged(qreal strength);
    void fontChanged(const QFont &font);
    void fontColorChanged(const QColor &color);
    void numberChanged(const int number);
    void shadowChanged(bool hasShadow);
    void geometryChanged(const QRectF &rect);
    void aspectRatioChanged(qreal ratio);

private:
    std::unique_ptr<AnnotationToolPrivate> d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AnnotationTool::Options)
