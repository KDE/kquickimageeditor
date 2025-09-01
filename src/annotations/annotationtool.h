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

/*!
 * \inqmlmodule org.kde.kquickimageeditor
 * \qmltype AnnotationTool
 * \brief The data structure that controls the creation of the next item.
 *
 * From QML its parameter
 * will be set by the app toolbars, and then drawing on the screen with the mouse will lead to the
 * creation of a new item based on those parameters
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

Q_SIGNALS:
    /*!
     * \qmlsignal AnnotationTool::typeChanged()
     */
    void typeChanged();
    /*!
     * \qmlsignal AnnotationTool::optionsChanged()
     */
    void optionsChanged();
    /*!
     * \qmlsignal AnnotationTool::strokeWidthChanged(int width)
     */
    void strokeWidthChanged(int width);
    /*!
     * \qmlsignal AnnotationTool::strokeColorChanged(color color)
     */
    void strokeColorChanged(const QColor &color);
    /*!
     * \qmlsignal AnnotationTool::fillColorChanged(color color)
     */
    void fillColorChanged(const QColor &color);
    /*!
     * \qmlsignal AnnotationTool::strengthChanged(real strength)
     */
    void strengthChanged(qreal strength);
    /*!
     * \qmlsignal AnnotationTool::fontChanged(font font);
     */
    void fontChanged(const QFont &font);
    /*!
     * \qmlsignal AnnotationTool::fontColorChanged(color color);
     */
    void fontColorChanged(const QColor &color);
    /*!
     * \qmlsignal AnnotationTool::numberChanged(int number)
     */
    void numberChanged(const int number);
    /*!
     * \qmlsignal AnnotationTool::shadowChanged(bool hasShadow)
     */
    void shadowChanged(bool hasShadow);

private:
    static constexpr AnnotationTool::Options optionsForType(AnnotationTool::Tool type);

    static constexpr int defaultStrokeWidthForType(AnnotationTool::Tool type);
    int strokeWidthForType(Tool type) const;
    void setStrokeWidthForType(int width, Tool type);

    static constexpr QColor defaultStrokeColorForType(AnnotationTool::Tool type);
    QColor strokeColorForType(Tool type) const;
    void setStrokeColorForType(const QColor &color, Tool type);

    static constexpr QColor defaultFillColorForType(AnnotationTool::Tool type);
    QColor fillColorForType(Tool type) const;
    void setFillColorForType(const QColor &color, Tool type);

    static constexpr qreal defaultStrengthForType(AnnotationTool::Tool type);
    qreal strengthForType(Tool type) const;
    void setStrengthForType(qreal strength, Tool type);

    QFont fontForType(Tool type) const;
    void setFontForType(const QFont &font, Tool type);

    static constexpr QColor defaultFontColorForType(AnnotationTool::Tool type);
    QColor fontColorForType(Tool type) const;
    void setFontColorForType(const QColor &color, Tool type);

    bool typeHasShadow(Tool type) const;
    void setTypeHasShadow(Tool type, bool shadow);

    Tool m_type = Tool::NoTool;
    Options m_options = Option::NoOptions;
    int m_number = 1;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AnnotationTool::Options)
