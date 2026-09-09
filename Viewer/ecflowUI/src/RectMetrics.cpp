/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "RectMetrics.hpp"

#include <QImage>
#include <QPainter>

RectMetrics::RectMetrics(int penWidth)
    : topOffset_(0),
      bottomOffset_(0) {
    if (penWidth >= 0 && penWidth <= 4) {
        compute(penWidth);
    }
}

void RectMetrics::compute(int penWidth) {
    QImage img(24, 24, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::white);
    QPainter p(&img);
    p.setPen(QPen(Qt::black, penWidth));

    int top    = 6;
    int bottom = img.height() - top;
    int left   = 6;
    int right  = img.width() - left;
    p.drawRect(QRect(left, top, right - left, bottom - top));

    int j = 12;

    // top
    for (int i = 0; i < img.height(); i++) {
        QRgb c = img.pixel(j, i);
        if (qRed(c) != 255 || qGreen(c) != 255 || qBlue(c) != 255) {
            topOffset_ = i - top;
            break;
        }
    }

    // bottom
    for (int i = img.height() - 1; i >= 0; i--) {
        QRgb c = img.pixel(j, i);
        if (qRed(c) != 255 || qGreen(c) != 255 || qBlue(c) != 255) {
            bottomOffset_ = i - bottom;
            break;
        }
    }
}
