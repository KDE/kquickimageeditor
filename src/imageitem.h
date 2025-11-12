/*
 * SPDX-FileCopyrightText: (C) 2011 Marco Martin <mart@kde.org>
 * SPDX-FileCopyrightText: (C) 2020 Luca Beltrame <lbeltrame@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QImage>
#include <QQuickPaintedItem>

/*!
 * \inqmlmodule org.kde.kquickimageeditor
 * \qmltype ImageItem
 */
class ImageItem : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

    /*!
     * \qmlproperty image ImageItem::image
     */
    Q_PROPERTY(QImage image READ image WRITE setImage NOTIFY imageChanged RESET resetImage)
    /*!
     * \qmlproperty int ImageItem::nativeWidth
     */
    Q_PROPERTY(int nativeWidth READ nativeWidth NOTIFY nativeWidthChanged)
    /*!
     * \qmlproperty int ImageItem::nativeHeight
     */
    Q_PROPERTY(int nativeHeight READ nativeHeight NOTIFY nativeHeightChanged)
    /*!
     * \qmlproperty int ImageItem::paintedWidth
     */
    Q_PROPERTY(int paintedWidth READ paintedWidth NOTIFY paintedWidthChanged)
    /*!
     * \qmlproperty int ImageItem::paintedHeight
     */
    Q_PROPERTY(int paintedHeight READ paintedHeight NOTIFY paintedHeightChanged)
    /*!
     * \qmlproperty int ImageItem::verticalPadding
     */
    Q_PROPERTY(int verticalPadding READ verticalPadding NOTIFY verticalPaddingChanged)
    /*!
     * \qmlproperty int ImageItem::horizontalPadding
     */
    Q_PROPERTY(int horizontalPadding READ horizontalPadding NOTIFY horizontalPaddingChanged)
    /*!
     * \qmlproperty FillMode ImageItem::fillMode
     * \qmlenumeratorsfrom ImageItem::FillMode
     */
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
    /*!
     * \qmlproperty bool ImageItem::null
     */
    Q_PROPERTY(bool null READ isNull NOTIFY nullChanged)

public:
    /*!
     * \enum ImageItem::FillMode
     * \value Stretch
     *        The image is scaled to fit.
     * \value PreserveAspectFit
     *        The image is scaled uniformly to fit without cropping.
     * \value PreserveAspectCrop
     *        The image is scaled uniformly to fill, cropping if necessary.
     * \value Tile
     *        The image is duplicated horizontally and vertically.
     * \value TileVertically
     *        The image is stretched horizontally and tiled vertically.
     * \value TileHorizontally
     *        The image is stretched vertically and tiled horizontally.
     */
    enum FillMode {
        Stretch,
        PreserveAspectFit,
        PreserveAspectCrop,
        Tile,
        TileVertically,
        TileHorizontally
    };
    Q_ENUM(FillMode)

    explicit ImageItem(QQuickItem *parent = nullptr);
    ~ImageItem() override = default;

    void setImage(const QImage &image);
    QImage image() const;
    void resetImage();

    int nativeWidth() const;
    int nativeHeight() const;

    int paintedWidth() const;
    int paintedHeight() const;
    int verticalPadding() const;
    int horizontalPadding() const;

    FillMode fillMode() const;
    void setFillMode(FillMode mode);

    void paint(QPainter *painter) override;

    bool isNull() const;

Q_SIGNALS:
    void nativeWidthChanged();
    void nativeHeightChanged();
    void fillModeChanged();
    void imageChanged();
    void nullChanged();
    void paintedWidthChanged();
    void paintedHeightChanged();
    void verticalPaddingChanged();
    void horizontalPaddingChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    QImage m_image;
    bool m_smooth;
    FillMode m_fillMode;
    QRect m_paintedRect;

private Q_SLOTS:
    void updatePaintedRect();
};
