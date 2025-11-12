/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QPainterPath>
#include <QQmlEngine>

/*!
 * \inqmlmodule org.kde.kquickimageeditor
 * \qmltype QmlPainterPath
 * \brief A class for making QPainterPaths available in QML.
 */
class QmlPainterPath
{
    QPainterPath m_path;
    Q_GADGET

    /*!
     * \qmlproperty string QmlPainterPath::svgPath
     * The path in the form of an SVG path string for use with a Qt Quick SvgPath.
     */
    Q_PROPERTY(QString svgPath READ svgPath FINAL)

    /*!
     * \qmlproperty bool QmlPainterPath::empty
     */
    Q_PROPERTY(bool empty READ empty FINAL)
    /*!
     * \qmlproperty int QmlPainterPath::elementCount
     */
    Q_PROPERTY(int elementCount READ elementCount FINAL)
    /*!
     * \qmlproperty point QmlPainterPath::start
     */
    Q_PROPERTY(QPointF start READ start FINAL)
    /*!
     * \qmlproperty point QmlPainterPath::end
     */
    Q_PROPERTY(QPointF end READ end FINAL)
    /*!
     * \qmlproperty rect QmlPainterPath::boundingRect
     */
    Q_PROPERTY(QRectF boundingRect READ boundingRect FINAL)

    QML_ELEMENT
    QML_FOREIGN(QPainterPath)
    QML_EXTENDED(QmlPainterPath)

public:
    /*!
     * \qmlmethod string QmlPainterPath::toString()
     */
    Q_INVOKABLE QString toString() const;

    /*!
     * \qmlmethod string QmlPainterPath::contains(point point)
     */
    Q_INVOKABLE bool contains(const QPointF &point) const;

    /*!
     * \qmlmethod string QmlPainterPath::contains(rect rect)
     */
    Q_INVOKABLE bool contains(const QRectF &rect) const;

    /*!
     * \qmlmethod string QmlPainterPath::intersects(rect rect)
     */
    Q_INVOKABLE bool intersects(const QRectF &rect) const;

    /*!
     * \qmlmethod string QmlPainterPath::map(matrix4x4 transform)
     */
    Q_INVOKABLE QPainterPath map(const QMatrix4x4 &transform) const;

    /*!
     * \qmlmethod string QmlPainterPath::mapBoundingRect(matrix4x4 transform)
     */
    Q_INVOKABLE QRectF mapBoundingRect(const QMatrix4x4 &transform) const;

    static QString toSvgPathElement(const QPainterPath::Element &element);

    static QString toSvgPath(const QPainterPath &path);

    QString svgPath() const;

    bool empty() const;

    int elementCount() const;

    QPointF start() const;

    QPointF end() const;

    QRectF boundingRect() const;

    operator QPainterPath() const;
};
