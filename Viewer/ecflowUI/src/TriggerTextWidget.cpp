//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerTextWidget.hpp"

#include <QFile>
#include <QTextStream>

#include "TriggerCollector.hpp"
#include "VItemPathParser.hpp"

TriggerTextWidget::TriggerTextWidget(QWidget* parent) : QTextBrowser(parent)
{
    setOpenExternalLinks(false);
    setOpenLinks(false);
    setReadOnly(true);

    setProperty("trigger","1");

    //Set css for the text formatting
    QString cssDoc;
    QFile f(":/viewer/trigger.css");
    //QTextStream in(&f);
    if(f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        cssDoc=QString(f.readAll());
    }
    f.close();
    document()->setDefaultStyleSheet(cssDoc);
}

void TriggerTextWidget::reload(TriggerTableItem* item)
{
    QString s="<table width=\'100%\'>";
    s+=makeHtml(item,"Triggers directly triggering the selected node","Triggers");
    s+="</table>";
    setHtml(s);
}

QString TriggerTextWidget::makeHtml(TriggerTableItem *ti,QString directTitle,QString modeText) const
{
    QString s;
    const std::vector<TriggerDependencyItem>& items=ti->dependencies();

    for(auto item : items)
    {
        VItem *t=item.dep();
        TriggerCollector::Mode mode=item.mode();

        if(!t)
            continue;

        s+="<tr><td>";
        if(mode == TriggerCollector::Parent)
           s+="parent";
        else
           s+="child";

        QString type=QString::fromStdString(t->typeName());
        QString path=QString::fromStdString(t->fullPath());
        QString anchor=QString::fromStdString(VItemPathParser::encode(t->fullPath(),t->typeName()));

        s+="  " + type;
        //s+=" <a class=\'chp\' href=\'" + anchor + "\'>" + path +"</a>";
        s+=" <a href=\'" + anchor + "\'>" + path +"</a>";
        s+="</td></tr>";
    }

    return s;
}


