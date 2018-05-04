//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TEXTFORMAT_HPP
#define TEXTFORMAT_HPP

#include <QColor>
#include <QList>
#include <QString>

class QAction;

namespace Viewer
{
    QString formatShortCut(QString);
    QString formatShortCut(QAction*);
    void addShortCutToToolTip(QList<QAction*>);
    QString formatBoldText(QString,QColor);
    QString formatText(QString,QColor);
    QString formatTableThText(QString txt,QColor col);
    QString formatTableTrBg(QString txt,QColor col);
    QString formatTableTdBg(QString txt,QColor col);
} //namespace Viewer

#endif // TEXTFORMAT_HPP
