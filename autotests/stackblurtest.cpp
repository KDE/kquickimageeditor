// SPDX-FileCopyrightText: 2025 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "../src/annotations/QtCV.h"

#include <QObject>
#include <QPainter>
#include <QTest>

class StackBlurTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void benchmarkStackBlur();
};

void StackBlurTest::benchmarkStackBlur()
{
    QImage img(QSize{1000, 1000}, QImage::Format_ARGB32);
    QPainter qPainter(&img);
    qPainter.setBrush(Qt::NoBrush);
    for (auto x = 0; x < 20; x++) {
        for (auto y = 0; y < 20; y++) {
            qPainter.setPen(Qt::red);
            qPainter.drawRect(x * 50, y * 50, 50, 50);
        }
    }

    QVERIFY(!img.isNull());

    QBENCHMARK {
        img.convertTo(QImage::Format_RGBA8888_Premultiplied);
        auto mat = QtCV::qImageToMat(img);
        QtCV::stackBlur(mat, mat, {}, 20, 20);
        QVERIFY(!img.isNull());
    }
}

QTEST_GUILESS_MAIN(StackBlurTest)

#include "stackblurtest.moc"
