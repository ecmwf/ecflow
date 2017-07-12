//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TOOLTIPFORMAT_HPP
#define TOOLTIPFORMAT_HPP

#include <QList>
#include <QString>

class QAction;

namespace Viewer
{
    QString formatShortCut(QAction*);
    void addShortCutToToolTip(QList<QAction*>);
} //namespace Viewer

#endif // TOOLTIPFORMAT_HPP
