//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ChangeNotifyDialog.hpp"

#include "ChangeNotifyModel.hpp"
#include "TreeView.hpp"
#include "VNodeList.hpp"
#include "VProperty.hpp"

#include <QDebug>
#include <QPainter>

ChangeNotifyDialog::ChangeNotifyDialog(QWidget *parent) :
	QDialog(parent),
	model_(0)
{
	setupUi(this);

	//model_=new ChangeNotifyModel(this);
	//treeView_->setModel(model_);
}

void ChangeNotifyDialog::init(VProperty* prop, ChangeNotifyModel* model)
{
	/*model_=model;
	treeView_->setModel(model_);

	if(prop)
	{
		QString sh="QLabel{ \
		   background: " + prop->paramToColour("background").name() + "; \
		   color: " + prop->paramToColour("foreground").name() + "; \
	       border: 1px solid " +  prop->paramToColour("border").name() + "; \
	       padding: 4 px; }";

		label_->setStyleSheet(sh);
		label_->setText(prop->param("labelText"));
		setWindowTitle(prop->param("title"));
	}*/
}

void ChangeNotifyDialog::addTab(const std::string id,VProperty* prop, ChangeNotifyModel* model)
{
	TreeView* tree=new TreeView(this);
	tree->setRootIsDecorated(false);
	tree->setUniformRowHeights(true);
	tree->setModel(model);

	//Create icon for tab
	QFont f;
	QFontMetrics fm(f);
	QString labelText=prop->param("labelText");
	int h=fm.height();
	int w=fm.width(labelText);
	int margin=4;
	QPixmap pix(2*margin+w,2*margin+h);
	pix.fill(prop->paramToColour("background"));

	QRect labelRect(margin,margin+1,w,h);

	QPainter painter(&pix);
	//painter.setBrush(prop->paramToColour("background"));
	//painter.setPen(prop->paramToColour("border"));
	//painter.drawRect(labelRect);

	painter.setPen(prop->paramToColour("foreground"));
	painter.drawText(labelRect,labelText);

	tab_->addTab(tree,"");
	tab_->setCustomIcon(tab_->count()-1,pix);
	tabMap_[id]=tab_->count()-1;
}

void ChangeNotifyDialog::setCurrentTab(const std::string& id)
{
	std::map<std::string,int>::const_iterator it=tabMap_.find(id);
	if(it != tabMap_.end())
	{
		tab_->setCurrentIndex(it->second);
	}
}

void ChangeNotifyDialog::setEnabledTab(const std::string& id,bool b)
{
	std::map<std::string,int>::const_iterator it=tabMap_.find(id);
	if(it != tabMap_.end())
	{
		tab_->setTabEnabled(it->second,b);
	}
}

void ChangeNotifyDialog::on_closePb__clicked(bool b)
{
	hide();
}

void ChangeNotifyDialog::on_clearCPb__clicked(bool b)
{
	hide();
	//model_->data()->clear();
}











