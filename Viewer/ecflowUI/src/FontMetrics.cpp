//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "FontMetrics.hpp"

#include <QImage>
#include <QPainter>

FontMetrics::FontMetrics(const QFont &font) :
    QFontMetrics(font),
    realHeight_(height()),
    topPadding_(0),
    bottomPadding_(0)
{
    computeRealHeight(font);
}

void FontMetrics::computeRealHeight(QFont f)
{
    QFontMetrics fm(f);
    QString txt="Ayfgl";
    QImage img(fm.width(txt)+6,fm.height(),QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::white);
    QPainter p(&img);
    p.setPen(Qt::black);
    f.setBold(true);
    p.setFont(f);
    p.drawText(QRect(0,0,img.width(),img.height()),Qt::AlignCenter,txt);

    int minRow=img.height()+100;
    int maxRow=-1;
    for(int i=0; i < img.height(); i++)
        for(int j=0; j < img.width(); j++)
        {
            QRgb c=img.pixel(j,i);
            if(qRed(c) != 255 || qGreen(c) != 255 ||  qBlue(c) != 255)
            {
                if(i > maxRow)
                    maxRow=i;
                if(i < minRow)
                    minRow=i;
            }
        }

    if(minRow >=0 && maxRow < img.height())
    {
        realHeight_=maxRow-minRow+1;
        topPadding_=minRow;
        bottomPadding_=img.height()-1-maxRow;
    }
}
