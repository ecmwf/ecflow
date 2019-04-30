//============================================================================
// Copyright 2009-2019 ECMWF.
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

//This item will be updated (the why? regenerated) at the end of each sync, so it does not need
//to observe the node it stores. The reason for this is that the why? can basically depend on
//anything in the tree. So anything in a sync can potentally have an impact on it.
WhyItemWidget::WhyItemWidget(QWidget *parent) : HtmlItemWidget(parent)
{
    //We will not keep the contents when the item becomes unselected
    unselectedFlags_.clear();

    messageLabel_->hide();
	fileLabel_->hide();

    //Will be used for ECFLOW-901
    infoProvider_=new WhyProvider(this);

    textEdit_->setProperty("trigger","1");
    textEdit_->setFontProperty(VConfig::instance()->find("panel.why.font"));

    //Read css for the text formatting
    QString cssDoc;
    QFile f(":/viewer/trigger.css");
    //QTextStream in(&f);
    if(f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        cssDoc=QString(f.readAll());
    }
    f.close();

    //Add css for state names
    std::vector<VParam*> states=VNState::filterItems();
    for(std::vector<VParam*>::const_iterator it=states.begin(); it!=states.end();++it)
    {     
       cssDoc+="font." + (*it)->name() +
               " {background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " +
               (*it)->colour().lighter(120).name() + ", stop: 1 " +  (*it)->colour().name() +
               "); color: " +  (*it)->typeColour().name() + ";}";

    }

    //Add css for false statements
    QColor falseCol(218,219,219);
    cssDoc+="font.false {background-color: " + falseCol.name() + ";}";

    textEdit_->document()->setDefaultStyleSheet(cssDoc);

    connect(textEdit_,SIGNAL(anchorClicked(QUrl)),
            this,SLOT(anchorClicked(QUrl)));

    //Define the mapping for <state>stateName</state> tag replacement with
    //<font class='stateName'>stateName</font>
    for(std::vector<VParam*>::const_iterator it=states.begin(); it!=states.end();++it)
    {
       stateMap_["<state>" + (*it)->name() + "</state>"]="<font class=\'"+ (*it)->name() + "\'>" + (*it)->name() + "</font>";
    }


}

WhyItemWidget::~WhyItemWidget()
{
    clearContents();
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

    messageLabel_->hide();

    if(info && info->server() && info->server()->isDisabled())
    {
        setEnabled(false);
        return;
    }
    else
    {
        setEnabled(true);
    }

    //set the info. we do not need to observe the node!!!
    info_=info;

    //Info must be a node
    if(info_)
    {
        infoProvider_->info(info_);
    }
}

void WhyItemWidget::load()
{
    textEdit_->clear();
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

void WhyItemWidget::infoReady(VReply* reply)
{
    Q_ASSERT(reply);
    messageLabel_->clear();
    messageLabel_->hide();
    load();
}

void WhyItemWidget::infoFailed(VReply* reply)
{
    QString s="Failed to refresh server before building the Why? output.\
                     Time-related attributes might show out-of-date values.";

    QString err=QString::fromStdString(reply->errorText());
    if(!err.isEmpty())
        s = s + "\n" + err;

    messageLabel_->showWarning(s);
    load();
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
    QString s;
    for(const auto & it : rawTxt)
    {
        QString line=QString::fromStdString(it);

        //UiLog().dbg() << line;

        QMap<QString,QString>::const_iterator stIt = stateMap_.constBegin();
        while (stIt != stateMap_.constEnd())
        {
            line.replace(stIt.key(),stIt.value());
            ++stIt;
        }

        line.replace("<false>","<font class=\'false\'>");
        line.replace("</false>","</font>");

        //UiLog().dbg() << " -->" << line;
        s+="<tr><td width=\'100%\'>" + line + "</td></tr>";
    }
    return s;
}

void WhyItemWidget::updateState(const FlagSet<ChangeFlag>& flags)
{
    if(flags.isSet(SuspendedChanged))
    {
        //If we are here this item is active but not selected!

        //When it becomes suspended we need to clear everything since the
        //tree is probably cleared at this point
        if(suspended_)
        {
            textEdit_->clear();
        }
        //When we leave the suspended state we need to reload everything
        else
        {
            load();
        }
    }

}

void WhyItemWidget::anchorClicked(const QUrl& link)
{
    linkSelected(link.path().toStdString());
}

//After each sync we need to reaload the contents
void WhyItemWidget::serverSyncFinished()
{
    if(frozen_)
        return;

    //We do not track changes when the item is not selected
    if(!selected_ || !active_)
        return;

    if(!info_)
        return;

    //For any change we nee to reload
    load();
}

static InfoPanelItemMaker<WhyItemWidget> maker1("why");

