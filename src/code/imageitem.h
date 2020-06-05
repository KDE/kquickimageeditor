/*
 * SPDX-FileCopyrightText: (C) 2011 Marco Martin <mart@kde.org>
 * SPDX-FileCopyrightText: (C) 2020 Luca Beltrame <lbeltrame@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QQuickPaintedItem>
#include <QImage>

class ImageItem : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(QImage image READ image WRITE setImage NOTIFY imageChanged RESET resetImage)
    Q_PROPERTY(bool smooth READ smooth WRITE setSmooth)
    Q_PROPERTY(int nativeWidth READ nativeWidth NOTIFY nativeWidthChanged)
    Q_PROPERTY(int nativeHeight READ nativeHeight NOTIFY nativeHeightChanged)
    Q_PROPERTY(int paintedWidth READ paintedWidth NOTIFY paintedWidthChanged)
    Q_PROPERTY(int paintedHeight READ paintedHeight NOTIFY paintedHeightChanged)
    Q_PROPERTY(int verticalPadding READ verticalPadding NOTIFY verticalPaddingChanged)
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
    Q_PROPERTY(bool null READ isNull NOTIFY nullChanged)

public:
    enum FillMode {
        Stretch, // the image is scaled to fit
        PreserveAspectFit, // the image is scaled uniformly to fit without cropping
        PreserveAspectCrop, // the image is scaled uniformly to fill, cropping if necessary
        Tile, // the image is duplicated horizontally and vertically
        TileVertically, // the image is stretched horizontally and tiled vertically
        TileHorizontally //the image is stretched vertically and tiled horizontally
    };
    Q_ENUM(FillMode)

    explicit ImageItem(QQuickItem *parent=nullptr);
    ~ImageItem() override = default;

    void setImage(const QImage &image);
    QImage image() const;
    void resetImage();

    void setSmooth(const bool smooth);
    bool smooth() const;

    int nativeWidth() const;
    int nativeHeight() const;

    int paintedWidth() const;
    int paintedHeight() const;
    int verticalPadding() const;

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

protected:
    void geometryChanged(const QRectF & newGeometry, const QRectF & oldGeometry) override;

private:
    QImage m_image;
    bool m_smooth;
    FillMode m_fillMode;
    QRect m_paintedRect;

private Q_SLOTS:
    void updatePaintedRect();

};
