/*
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <QObject>

class ImageDocument : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(QImage image READ image WRITE setImage NOTIFY imageNotify)
    
public:
    ImageDocument();
    ~ImageDocument() = default;
    
};
