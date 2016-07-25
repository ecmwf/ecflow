//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "EditItemWidget.hpp"

#include "EditProvider.hpp"
#include "Highlighter.hpp"
#include "PropertyMapper.hpp"
#include "VConfig.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "UserMessage.hpp"

//========================================================
//
// EditItemWidget
//
//========================================================

EditItemWidget::EditItemWidget(QWidget *parent) :
   QWidget(parent),
   preproc_(false),
   alias_(false)
{
	setupUi(this);

	infoProvider_=new EditProvider(this);

	Highlighter* ih=new Highlighter(textEdit_->document(),"script");

	searchLine_->setEditor(textEdit_);

	searchLine_->setVisible(false);

	externTb_->hide();

	//connect(submitTb_,SIGNAL(clicked(bool)),
	//		this,SLOT(on_submitTb__clicked(bool)));

    textEdit_->setProperty("edit","1");
	textEdit_->setFontProperty(VConfig::instance()->find("panel.edit.font"));
}

QWidget* EditItemWidget::realWidget()
{
	return this;
}

void EditItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();
    info_=info;

    //Info must be a node
    if(info_ && info_->isNode() && info_->node())
    {
        //Get file contents
        EditProvider* ep=static_cast<EditProvider*>(infoProvider_);
        ep->preproc(preproc());
        infoProvider_->info(info_);
    }
}

void EditItemWidget::clearContents()
{
	InfoPanelItem::clear();
	textEdit_->clear();
}

void EditItemWidget::infoReady(VReply* reply)
{
	QString s=QString::fromStdString(reply->text());
	textEdit_->setPlainText(s);
}

void EditItemWidget::infoFailed(VReply* reply)
{
	UserMessage::message(UserMessage::ERROR, true, reply->errorText());
}

void EditItemWidget::infoProgress(VReply*)
{

}

void EditItemWidget::on_preprocTb__toggled(bool)
{
	reload(info_);
}

void EditItemWidget::on_submitTb__clicked(bool)
{
	QStringList lst=textEdit_->toPlainText().split("\n");
	std::vector<std::string> txt;
	Q_FOREACH(QString s,lst)
	{
		txt.push_back(s.toStdString());
	}

	EditProvider* ep=static_cast<EditProvider*>(infoProvider_);
	ep->submit(txt,alias());
}

void EditItemWidget::on_searchTb__clicked()
{
	searchLine_->setVisible(true);
	searchLine_->setFocus();
	searchLine_->selectAll();
}

void EditItemWidget::on_gotoLineTb__clicked()
{
	textEdit_->gotoLine();
}

bool EditItemWidget::alias() const
{
	return aliasTb_->isChecked();
}

bool EditItemWidget::preproc() const
{
	return preprocTb_->isChecked();
}

//-----------------------------------------
// Fontsize management
//-----------------------------------------

void EditItemWidget::on_fontSizeUpTb__clicked()
{
	//We need to call a custom slot here instead of "zoomIn"!!!
	textEdit_->slotZoomIn();
}

void EditItemWidget::on_fontSizeDownTb__clicked()
{
	//We need to call a custom slot here instead of "zoomOut"!!!
	textEdit_->slotZoomOut();
}

static InfoPanelItemMaker<EditItemWidget> maker1("edit");
