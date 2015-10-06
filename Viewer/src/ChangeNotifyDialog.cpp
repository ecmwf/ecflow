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

#include <QCloseEvent>
#include <QDebug>
#include <QPainter>
#include <QSettings>
#include <QVariant>

ChangeNotifyDialog::ChangeNotifyDialog(QWidget *parent) :
	QDialog(parent),
	ignoreCurrentChange_(false)
{
	setupUi(this);

	tab_->setProperty("notify","1");

#ifdef ECFLOW_QT5
	tab_->tabBar()->setExpanding(false);
#endif

	clearOnCloseCb_->setChecked(true);

	grad_.setCoordinateMode(QGradient::ObjectBoundingMode);
	grad_.setStart(0,0);
	grad_.setFinalStop(0,1);

	readSettings();
}

ChangeNotifyDialog::~ChangeNotifyDialog()
{
	writeSettings();
}

void ChangeNotifyDialog::addTab(ChangeNotify* notifier)
{
	const std::string& id=notifier->id();

	TreeView* tree=new TreeView(this);
	tree->setRootIsDecorated(false);
	tree->setUniformRowHeights(true);
	tree->setModel(notifier->model());

	ignoreCurrentChange_=true;
	tab_->addTab(tree,"");
	ignoreCurrentChange_=false;

	int idx=tab_->count()-1;
	ntfToTabMap_[notifier]=idx;
	tabToNtfMap_[idx]=notifier;

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
	int margin=3;

	QColor bgCol(Qt::gray);
	if(VProperty *p=prop->findChild("fill_colour"))
		bgCol=p->value().value<QColor>();

	QColor fgCol(Qt::black);
	if(VProperty *p=prop->findChild("font_colour"))
		fgCol=p->value().value<QColor>();


	QColor bgLight=bgCol.lighter(150);
	grad_.setColorAt(0,bgLight);
	grad_.setColorAt(1,bgCol);
	QBrush bgBrush(grad_);

	QPixmap pix;

	//Create icon for tab
	if(1) //tabIdx == tab_->currentIndex())
	{
		pix=QPixmap(2*margin+w,2*margin+h);

		pix.fill(Qt::transparent);

		QRect labelRect(0,0,pix.width(),pix.height());

		QPainter painter(&pix);

		painter.fillRect(labelRect,bgBrush);
		painter.setPen(fgCol);
		painter.drawText(labelRect,Qt::AlignVCenter|Qt::AlignHCenter,labelText);
	}
	else
	{
		pix=QPixmap(2*margin+w,2*margin+h+2);

		pix.fill(Qt::transparent);

		QRect labelRect(margin,margin+1,w,h);

		QPainter painter(&pix);

		painter.setPen(fgCol);
		painter.drawText(labelRect,Qt::AlignVCenter|Qt::AlignHCenter,labelText);

		QRect lineRect(labelRect.left(),labelRect.bottom()+1,
					   labelRect.width(),3);

		painter.fillRect(lineRect,bgBrush);
	}

	tab_->setCustomIcon(tabIdx,pix);
}


void ChangeNotifyDialog::setCurrentTab(ChangeNotify *ntf)
{
	int tabIdx=ntfToTab(ntf);
	if(tabIdx != -1)
	{
		tab_->setCurrentIndex(tabIdx);
	}
}

void ChangeNotifyDialog::setEnabledTab(ChangeNotify* ntf,bool b)
{
	int tabIdx=ntfToTab(ntf);
	if(tabIdx != -1)
	{
		tab_->setTabEnabled(tabIdx,b);
	}
}

void ChangeNotifyDialog::on_tab__currentChanged(int)
{
	if(ignoreCurrentChange_)
		return;
}


void ChangeNotifyDialog::on_closePb__clicked(bool b)
{
	hide();

	if(clearOnCloseCb_->isChecked())
	{
		int idx=tab_->currentIndex();
		if(idx != -1)
		{
			if(ChangeNotify *ntf=tabToNtf(idx))
				ntf->clearData();
		}
	}
}

void ChangeNotifyDialog::on_clearPb__clicked(bool b)
{
	int idx=tab_->currentIndex();
	if(idx != -1)
	{
		if(ChangeNotify *ntf=tabToNtf(idx))
			ntf->clearData();
	}
}

ChangeNotify* ChangeNotifyDialog::tabToNtf(int tabIdx)
{
	std::map<int,ChangeNotify*>::const_iterator it=tabToNtfMap_.find(tabIdx);
		if(it != tabToNtfMap_.end())
			return it->second;

	return 0;
}

int ChangeNotifyDialog::ntfToTab(ChangeNotify* ntf)
{
	std::map<ChangeNotify*,int>::const_iterator it=ntfToTabMap_.find(ntf);
	if(it != ntfToTabMap_.end())
		return it->second;

	return -1;
}

void ChangeNotifyDialog::updateSettings(ChangeNotify* notifier)
{
	std::map<ChangeNotify*,int>::const_iterator it=ntfToTabMap_.find(notifier);
	if(it != ntfToTabMap_.end())
	{
		decorateTab(it->second,notifier->prop());
	}
}

void ChangeNotifyDialog::closeEvent(QCloseEvent* e)
{
	writeSettings();
	e->accept();
}

void ChangeNotifyDialog::writeSettings()
{
	QSettings settings("ECMWF","ecflowUI-ChangeNotifyDialog");

	//We have to clear it so that should not remember all the previous values
	settings.clear();

	settings.beginGroup("main");
	settings.setValue("size",size());
	settings.setValue("clearOnClose",clearOnCloseCb_->isChecked());
	settings.endGroup();
}

void ChangeNotifyDialog::readSettings()
{
	QSettings settings("ECMWF","ecflowUI-ChangeNotifyDialog");

	settings.beginGroup("main");
	if(settings.contains("size"))
	{
		resize(settings.value("size").toSize());
	}
	else
	{
	  	resize(QSize(440,380));
	}

	if(settings.contains("clearOnClose"))
	{
		clearOnCloseCb_->setChecked(settings.value("clearOnClose").toBool());
	}

	settings.endGroup();
}
