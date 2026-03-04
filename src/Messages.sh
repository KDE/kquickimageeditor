#! /bin/sh
#SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
#SPDX-License-Identifier: LGPL-2.0-or-later

$XGETTEXT `find . -name \*.cpp -o -name \*.h -o -name \*.qml` -o $podir/kquickimageeditor6.pot
