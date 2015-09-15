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

#include "ChangeNotify.hpp"
#include "ChangeNotifyModel.hpp"
#include "TreeView.hpp"
#include "VNodeList.hpp"
#include "VProperty.hpp"

#include <QDebug>
#include <QPainter>
#include <QSettings>
#include <QVariant>

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
	int idx=tab_->count()-1;
	idToTabMap_[id]=idx;
	tabToIdMap_[idx]=id;
}

void ChangeNotifyDialog::setCurrentTab(const std::string& id)
{
	int tabIdx=idToTab(id);
	if(tabIdx != -1)
	{
		tab_->setCurrentIndex(tabIdx);
	}
}

void ChangeNotifyDialog::setEnabledTab(const std::string& id,bool b)
{
	int tabIdx=idToTab(id);
	if(tabIdx != -1)
	{
		tab_->setTabEnabled(tabIdx,b);
	}
}

void ChangeNotifyDialog::on_closePb__clicked(bool b)
{
	hide();
}

void ChangeNotifyDialog::on_clearCPb__clicked(bool b)
{
	hide();
	int idx=tab_->currentIndex();
	if(idx != -1)
	{
		ChangeNotify::clearData(tabToId(idx));
	}
}

std::string ChangeNotifyDialog::tabToId(int tabIdx)
{
	std::map<int,std::string>::const_iterator it=tabToIdMap_.find(tabIdx);
		if(it != tabToIdMap_.end())
			return it->second;

	return std::string();
}

int ChangeNotifyDialog::idToTab(const std::string& id)
{
	std::map<std::string,int>::const_iterator it=idToTabMap_.find(id);
	if(it != idToTabMap_.end())
		return it->second;

	return -1;
}


void ChangeNotifyDialog::writeSettings()
{
	QSettings settings("ECMWF","ecflowUI-PropertyDialog");

	//We have to clear it so that should not remember all the previous values
	settings.clear();

	settings.beginGroup("main");
	settings.setValue("size",size());
	settings.endGroup();
}

void ChangeNotifyDialog::readSettings()
{
	QSettings settings("ECMWF","ecflowUI-PropertyDialog");

	settings.beginGroup("main");
	if(settings.contains("size"))
	{
		resize(settings.value("size").toSize());
	}
	else
	{
	  	resize(QSize(440,380));
	}

	settings.endGroup();
}








