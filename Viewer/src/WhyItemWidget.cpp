//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "WhyItemWidget.hpp"

#include <QDebug>

#include "Node.hpp"

#include "InfoProvider.hpp"
#include "ServerHandler.hpp"
#include "UiLog.hpp"
#include "VConfig.hpp"
#include "VItemPathParser.hpp"
#include "VNode.hpp"
#include "VNState.hpp"

//========================================================
//
// WhyItemWidget
//
//========================================================

WhyItemWidget::WhyItemWidget(QWidget *parent) : HtmlItemWidget(parent)
{
	messageLabel_->hide();
	fileLabel_->hide();

    //Will be used for ECFLOW-901
    infoProvider_=new WhyProvider(this);

    textEdit_->setProperty("trigger","1");
    textEdit_->setFontProperty(VConfig::instance()->find("panel.why.font"));

    //Set css for the text formatting
    QString cssDoc;
    QFile f(":/viewer/trigger.css");
    //QTextStream in(&f);
    if(f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        cssDoc=QString(f.readAll());
    }
    f.close();

    QMap<QString,QString> stateMap;
    std::vector<VParam*> states=VNState::filterItems();
    for(std::vector<VParam*>::const_iterator it=states.begin(); it!=states.end();++it)
    {
       cssDoc+="font." + (*it)->name() + " {background-color: " + (*it)->colour().name() + ";}";
    }

    textEdit_->document()->setDefaultStyleSheet(cssDoc);


#if 0
    //Set css for the text formatting
    QString cssDoc="a:link { text-decoration:none; color: #0645AD;}";
    textEdit_->document()->setDefaultStyleSheet(cssDoc);
#endif

    connect(textEdit_,SIGNAL(anchorClicked(QUrl)),
            this,SLOT(anchorClicked(QUrl)));
}

WhyItemWidget::~WhyItemWidget()
{
}


QWidget* WhyItemWidget::realWidget()
{
	return this;
}

void WhyItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();
	info_=info;

    if(info_)
    {
        textEdit_->insertHtml(why());
    }
}

void WhyItemWidget::clearContents()
{
	InfoPanelItem::clear();
	textEdit_->clear();
}

QString WhyItemWidget::why() const
{
	QString s;

    std::vector<std::string> bottomUpWhy,topDownWhy;

    if(info_ && info_->server())
	{
        //This stops the queue on the serverhandler so that no update
        //could happen while we generate the why? information
        info_->server()->searchBegan();

        if(info_->isServer())
		{
            info_->node()->why(topDownWhy);
		}
		else if(info_->isNode() && info_->node())
		{
            info_->node()->why(bottomUpWhy,topDownWhy);
		}

        //Resume the queue on the serverhandler
        info_->server()->searchFinished();
	}

    s=makeHtml(bottomUpWhy,topDownWhy);
    return s;
}

QString WhyItemWidget::makeHtml(const std::vector<std::string>& bottomUpTxt,
                                const std::vector<std::string>& topDownTxt) const
{
    if(bottomUpTxt.empty() && topDownTxt.empty())
        return QString();

    QString s="<table width=\'100%\'>";

    if(!bottomUpTxt.empty())
    {
        s+="<tr><td class=\'direct_title\'>Bottom-up why? - through the parents</td></tr>";
        s+=makeHtml(bottomUpTxt);
    }

    if(!topDownTxt.empty())
    {
        s+="<tr><td class=\'direct_title\'>Top-down why? - through the children</td></tr>";
        s+=makeHtml(topDownTxt);
    }

    s+="</table>";
    return s;
}

QString WhyItemWidget::makeHtml(const std::vector<std::string>& rawTxt) const
{
    QMap<QString,QString> stateMap;
    std::vector<VParam*> states=VNState::filterItems();
    for(std::vector<VParam*>::const_iterator it=states.begin(); it!=states.end();++it)
    {
       stateMap["<state>" + (*it)->name() + "</state>"]="<font class=\'"+ (*it)->name() + "\'>" + (*it)->name() + "</font>";
    }

    QString s;
    for(std::vector<std::string>::const_iterator it=rawTxt.begin(); it != rawTxt.end(); ++it)
    {
        QString line=QString::fromStdString(*it);
//        QRegExp rx("'\\S+:(\\S+)'");
//        if(rx.indexIn(line) > -1 && rx.captureCount() == 1)
//        {
//            QString path=rx.cap(1);
//            rx=QRegExp("'(\\S+):");
//            QString type,typeOri;
//            if(rx.indexIn(line) > -1 && rx.captureCount() == 1)
//            {
//                typeOri=rx.cap(1);
//                type=typeOri.toLower();
//            }
//
//            QString anchor=QString::fromStdString(VItemPathParser::encode(path.toStdString(),type.toStdString()));
//            line.replace("\'" + typeOri + ":"," " + typeOri + " ");
//            line.replace(path + "'","<a href=\'" + anchor  + "\'>" + path +"</a>");
//
//        }
//        else
//        {
//            rx=QRegExp("\\s+(/\\S+)\\b");
//            if(rx.indexIn(line) > -1 && rx.captureCount() == 1)
//            {
//                QString path=rx.cap(1);
//                rx=QRegExp("(SUITE|FAMILY|TASK|ALIAS)");
//                QString type;
//                if(rx.indexIn(line) > -1 && rx.captureCount() == 1)
//                {
//                    type=rx.cap(1);
//                }
//
//                QString anchor=QString::fromStdString(VItemPathParser::encode(path.toStdString(),type.toStdString()));
//                line.replace(path,"<a href=\'" + anchor  + "\'>" + path +"</a>");
//            }
//        }



        s+="<tr><td width=\'100%\'>" + line + "</td></tr>";
    }
    return s;
}

void WhyItemWidget::anchorClicked(const QUrl& link)
{
    linkSelected(link.path().toStdString());
}

static InfoPanelItemMaker<WhyItemWidget> maker1("why");

