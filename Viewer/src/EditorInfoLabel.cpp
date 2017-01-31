//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "EditorInfoLabel.hpp"

#include <QStringList>
#include <QVariant>


EditorInfoLabel::EditorInfoLabel(QWidget* parent) : QLabel(parent)
{
    //Define id for the css
    setProperty("editorInfo","1");
    setWordWrap(true);

    //Set size policy
    /*QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
    setSizePolicy(sizePolicy);
    //setMinimumSize(QSize(0, 60));
    //setMaximumSize(QSize(16777215, 45));*/

    setMargin(4);
    setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

    //Other settings
    setAutoFillBackground(true);

    setFrameShape(QFrame::StyledPanel);
    setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);
}

void EditorInfoLabel::setInfo(QString parent,QString type)
{
    setText(formatKeyLabel("Node: ") + formatNodePath(parent) + "<br>" +
            formatKeyLabel("Attibute type: ") + type);
}

static QColor nodeNameColour(7,108,209);
static QColor serverNameColour(0,0,0);
static QColor labelColour(59,60,61);

QString EditorInfoLabel::formatKeyLabel(QString n)
{
    return "<font color=\'" + labelColour.name() + "\'><b>" + n + "</b></font>";
}

QString EditorInfoLabel::formatNodeName(QString n)
{
    return "<font color=\'" + nodeNameColour.name() + "\'>" + n + "</font>";
}

QString EditorInfoLabel::formatNodePath(QString p)
{
    if(p.endsWith("://"))
    {
        return p;
    }

    QStringList lst=p.split("/");
    if(lst.count() > 0)
    {
        QString n=lst.back();
        lst.pop_back();
        QColor c(80,80,80);
        QString s="<font color=\'" + c.name() + "\'>" + lst.join("/") + "/" +
                "</font><font color=\'" + serverNameColour.name() + "\'>" +  n + "</font>";
        return s;
    }

    return p;
}





