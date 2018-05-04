//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TextFormat.hpp"

#include <QAction>
#include <QColor>

static QColor shortCutCol(147,165,202);

namespace Viewer
{

QString formatShortCut(QString text)
{
    return "<code><font color=\'" + shortCutCol.name() + "\'>" + text + "</font></code>";
}


QString formatShortCut(QAction* ac)
{
    Q_ASSERT(ac);

    return "<code><font color=\'" + shortCutCol.name() + "\'>&nbsp;(" +
        ac->shortcut().toString() + ")</font></code>";
}

void addShortCutToToolTip(QList<QAction*> alst)
{
    Q_FOREACH(QAction *ac,alst)
    {
        if(!ac->shortcut().isEmpty())
        {
            QString tt=ac->toolTip();
            if(!tt.isEmpty())
            {
                tt+=" " + formatShortCut(ac);
                ac->setToolTip(tt);
            }
        }
    }
}

QString formatBoldText(QString txt,QColor col)
{
    return "<b><font color=\'" + col.name() + "\'>" +txt + "</font></b>";
}

QString formatText(QString txt,QColor col)
{
    return "<font color=\'" + col.name() + "\'>" +txt + "</font>";
}

QString formatTableThText(QString txt,QColor col)
{
    return "<th><font color=\'" + col.name() + "\'>" +txt + "</font></th>";
}

QString formatTableTrBg(QString txt,QColor col)
{
    return "<tr bgcolor=\'" + col.name() + "\'>" +txt + "</tr>";
}

QString formatTableTdBg(QString txt,QColor col)
{
    return "<td bgcolor=\'" + col.name() + "\'>" +txt + "</td>";
}

} //namespace Viewer
