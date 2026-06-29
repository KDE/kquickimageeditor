// SPDX-FileCopyrightText: 2025 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "../src/annotations/stackblur.h"

#include <QObject>
#include <QTest>

class StackBlurTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
};

QTEST_GUILESS_MAIN(StackBlurTest)

#include "stackblurtest.moc"
