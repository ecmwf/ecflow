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
	QDialog(parent)
{
	setupUi(this);
}

void ChangeNotifyDialog::addTab(ChangeNotify* notifier)
{
	const std::string& id=notifier->id();

	TreeView* tree=new TreeView(this);
	tree->setRootIsDecorated(false);
	tree->setUniformRowHeights(true);
	tree->setModel(notifier->model());

	tab_->addTab(tree,"");

	int idx=tab_->count()-1;
	idToTabMap_[id]=idx;
	tabToIdMap_[idx]=id;

	decorateTab(idx,notifier->prop());
}

void ChangeNotifyDialog::decorateTab(int tabIdx,VProperty *prop)
{
	//Create icon for tab
	QFont f;
	QFontMetrics fm(f);
	QString labelText=prop->param("labelText");
	int h=fm.height();
	int w=fm.width(labelText);
	int margin=4;
	QPixmap pix(2*margin+w,2*margin+h);

	QColor bgCol(Qt::gray);
	if(VProperty *p=prop->findChild("fill_colour"))
		bgCol=p->value().value<QColor>();

	QColor fgCol(Qt::black);
	if(VProperty *p=prop->findChild("font_colour"))
		fgCol=p->value().value<QColor>();

	pix.fill(bgCol);

	QRect labelRect(margin,margin+1,w,h);

	QPainter painter(&pix);
	//painter.setBrush(prop->paramToColour("background"));
	//painter.setPen(prop->paramToColour("border"));
	//painter.drawRect(labelRect);

	painter.setPen(fgCol);
	painter.drawText(labelRect,labelText);

	tab_->setCustomIcon(tabIdx,pix);
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

void ChangeNotifyDialog::updateSettings(ChangeNotify* notifier)
{
	std::map<std::string,int>::const_iterator it=idToTabMap_.find(notifier->id());
	if(it != idToTabMap_.end())
	{
		decorateTab(it->second,notifier->prop());
	}
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
