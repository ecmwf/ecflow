//============================================================================
// Copyright 2009-2019 ECMWF.
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

QString formatItalicText(QString txt,QColor col)
{
    return "<i><font color=\'" + col.name() + "\'>" +txt + "</font></i>";
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

QString formatTableTrText(QString txt)
{
    return "<tr>" + txt + "</tr>";
}

QString formatTableTdText(QString txt,QColor col)
{
    return "<td><font color=\'" + col.name() + "\'>" +txt + "</font></td>";
}

QString formatTableTdText(QString txt)
{
    return "<td>" + txt + "</td>";
}

QString formatTableTdBg(QString txt,QColor col)
{
    return "<td bgcolor=\'" + col.name() + "\'>" +txt + "</td>";
}

QString formatTableRow(QString col1Text,QString col2Text,QColor bg,QColor fg, bool boldCol1)
{
    QString txt;
    if(boldCol1)
        txt="<td>" + formatBoldText(col1Text,fg) + "</td>";
    else
        txt=formatTableTdText(col1Text,fg);

    txt+=formatTableTdText(col2Text,fg);

    return formatTableTrBg(txt,bg);
}

QString formatTableRow(QString col1Text,QString col2Text, bool boldCol1)
{
    if(boldCol1)
        col1Text = "<b>" + col1Text + "</b>";

    return formatTableTrText(formatTableTdText(col1Text) +formatTableTdText(col2Text));
}


} //namespace Viewer
