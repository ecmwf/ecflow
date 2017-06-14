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


    /*QFont f("Monospace");
    f.setStyleHint(QFont::TypeWriter);
    f.setFixedPitch(true);
    f.setPointSize(10);
    f.setStyleStrategy(QFont::PreferAntialias);
    setFont(f);*/

    //connect(triggerBrowser_,SIGNAL(anchorClicked(const QUrl&)),
    //        this,SLOT(anchorClicked(const QUrl&)));


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
    bool firstDirectTrigger=true;
    const std::vector<TriggerDependencyItem>& items=ti->dependencies();

    int prevH=0;
QString h;
    for(unsigned int i=0; i < items.size(); i++)
    {
        VItem *t=items[i].dep();
        TriggerCollector::Mode mode=items[i].mode();

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

#if 0
        if(h != prevH)
        {
            s+="<tr><td class=\'title\' colspan=\'2\'>" + h + "</td></tr>";
            prevH=h;
        }

        type=QString::fromStdString(t->typeName());
        path=QString::fromStdString(t->fullPath());
        anchor=QString::fromStdString(VItemPathParser::encode(t->fullPath(),t->typeName()));

        s+="<tr>";
        s+="<td width=\'100\'>" + type + "</td>";
        s+="<td><a href=\'" +  anchor + "\'>" + path +"</a>";
#endif
    }

    return s;
}


