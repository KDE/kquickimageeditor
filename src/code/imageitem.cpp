
/*
 * SPDX-FileCopyrightText: (C) 2011 Marco Martin <mart@kde.org>
 * SPDX-FileCopyrightText: (C) 2020 Luca Beltrame <lbeltrame@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "imageitem.h"

#include <QPainter>
#include <QDebug>


ImageItem::ImageItem(QQuickItem *parent)
    : QQuickPaintedItem(parent),
      m_smooth(false),
      m_fillMode(ImageItem::Stretch)
{
    setFlag(ItemHasContents, true);
}

void ImageItem::setImage(const QImage &image)
{
    bool oldImageNull = m_image.isNull();
    m_image = image;
    updatePaintedRect();
    update();
    Q_EMIT nativeWidthChanged();
    Q_EMIT nativeHeightChanged();
    Q_EMIT imageChanged();
    if (oldImageNull != m_image.isNull()) {
        Q_EMIT nullChanged();
    }
}

QImage ImageItem::image() const
{
    return m_image;
}

void ImageItem::resetImage()
{
    setImage(QImage());
}

void ImageItem::setSmooth(const bool smooth)
{
    if (smooth == m_smooth) {
        return;
    }
    m_smooth = smooth;
    update();
}

bool ImageItem::smooth() const
{
    return m_smooth;
}

int ImageItem::nativeWidth() const
{
    return m_image.size().width() / m_image.devicePixelRatio();
}

int ImageItem::nativeHeight() const
{
    return m_image.size().height() / m_image.devicePixelRatio();
}

ImageItem::FillMode ImageItem::fillMode() const
{
    return m_fillMode;
}

void ImageItem::setFillMode(ImageItem::FillMode mode)
{
    if (mode == m_fillMode) {
        return;
    }

    m_fillMode = mode;
    updatePaintedRect();
    update();
    Q_EMIT fillModeChanged();
}

void ImageItem::paint(QPainter *painter)
{
    if (m_image.isNull()) {
        return;
    }
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, m_smooth);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, m_smooth);

    if (m_fillMode == TileVertically) {
        painter->scale(width()/(qreal)m_image.width(), 1);
    }

    if (m_fillMode == TileHorizontally) {
        painter->scale(1, height()/(qreal)m_image.height());
    }

    if (m_fillMode >= Tile) {
        painter->drawTiledPixmap(m_paintedRect, QPixmap::fromImage(m_image));
    } else {
        painter->drawImage(m_paintedRect, m_image, m_image.rect());
    }

    painter->restore();
}

bool ImageItem::isNull() const
{
    return m_image.isNull();
}

int ImageItem::paintedWidth() const
{
    if (m_image.isNull()) {
        return 0;
    }

    return m_paintedRect.width();
}

int ImageItem::paintedHeight() const
{
    if (m_image.isNull()) {
        return 0;
    }

    return m_paintedRect.height();
}

int ImageItem::verticalPadding() const
{
    if (m_image.isNull()) {
        return 0;
    }

    return (height() - m_paintedRect.height()) / 2;
}

int ImageItem::horizontalPadding() const
{
    if (m_image.isNull()) {
        return 0;
    }

    return (width() - m_paintedRect.width()) / 2;
}

void ImageItem::updatePaintedRect()
{

    if (m_image.isNull()) {
        return;
    }

    QRect sourceRect = m_paintedRect;

    QRect destRect;

    switch (m_fillMode) {
    case PreserveAspectFit: {
        QSize scaled = m_image.size();

        scaled.scale(boundingRect().size().toSize(), Qt::KeepAspectRatio);
        destRect = QRect(QPoint(0, 0), scaled);
        destRect.moveCenter(boundingRect().center().toPoint());
        break;
    }
    case PreserveAspectCrop: {
        QSize scaled = m_image.size();

        scaled.scale(boundingRect().size().toSize(), Qt::KeepAspectRatioByExpanding);
        destRect = QRect(QPoint(0, 0), scaled);
        destRect.moveCenter(boundingRect().center().toPoint());
        break;
    }
    case TileVertically: {
        destRect = boundingRect().toRect();
        destRect.setWidth(destRect.width() / (width()/(qreal)m_image.width()));
        break;
    }
    case TileHorizontally: {
        destRect = boundingRect().toRect();
        destRect.setHeight(destRect.height() / (height()/(qreal)m_image.height()));
        break;
    }
    case Stretch:
    case Tile:
    default:
        destRect = boundingRect().toRect();
    }

    if (destRect != sourceRect) {
        m_paintedRect = destRect;
        Q_EMIT paintedHeightChanged();
        Q_EMIT paintedWidthChanged();
        Q_EMIT verticalPaddingChanged();
    }
}

void ImageItem::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickPaintedItem::geometryChanged(newGeometry, oldGeometry);
    updatePaintedRect();
}
