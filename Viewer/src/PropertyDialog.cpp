//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "PropertyDialog.hpp"

#include <QAbstractButton>
#include <QCloseEvent>
#include <QSettings>

#include "ConfigListDelegate.hpp"
#include "IconProvider.hpp"
#include "PropertyEditor.hpp"
#include "VConfig.hpp"
#include "VConfigLoader.hpp"
#include "VProperty.hpp"

VProperty* PropertyDialog::prop_=0;

PropertyDialog::PropertyDialog(QWidget* parent) :
		QDialog(parent),
		configChanged_(false)
{
	setupUi(this);

	QFont f;
	QFontMetrics fm(f);
	int maxW=fm.width("Server options ATAT");
	list_->setMaximumWidth(maxW+6);

	list_->setItemDelegate(new ConfigListDelegate(32,maxW,this));

	connect(list_,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(slotChangePage(QListWidgetItem*,QListWidgetItem*)));

	connect(buttonBox_,SIGNAL(clicked(QAbstractButton*)),
			this,SLOT(slotButton(QAbstractButton*)));

	build();

	readSettings();

	list_->setCurrentRow(0);
}

void PropertyDialog::closeEvent(QCloseEvent * event)
{
	event->accept();
	writeSettings();
}

//Build the property tree from the the definitions
void PropertyDialog::build()
{
	if(prop_)
	{
		Q_FOREACH(VProperty* vPage,prop_->children())
		{
			  if(vPage->param("visible") == "false")
                  continue;
            
              PropertyEditor* ed=new PropertyEditor(this);
			  ed->edit(vPage);
			  QPixmap pix(32,32);
              QString iconStr=vPage->param("icon");
              if(!iconStr.isEmpty())
              {
            	  IconProvider::add(":/viewer/" + iconStr,iconStr);
            	  pix=IconProvider::pixmap(iconStr,32);
              }    
              addPage(ed,pix,vPage->param("label"));
			  editors_ << ed;
		}
	}
}

void PropertyDialog::apply()
{
	manageChange();
}

void PropertyDialog::accept()
{
	manageChange();
	writeSettings();
    QDialog::accept();
}    


void PropertyDialog::reject()
{
	writeSettings();
	QDialog::reject();
}

void PropertyDialog::addPage(QWidget *w,QPixmap pix,QString txt)
{
	QListWidgetItem *item = new QListWidgetItem(list_);
    item->setData(Qt::DecorationRole, pix);
    item->setData(Qt::DisplayRole,txt);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

	page_->addWidget(w);
}

void PropertyDialog::slotChangePage(QListWidgetItem *current, QListWidgetItem *previous)
{
     if (!current)
         current = previous;

     page_->setCurrentIndex(list_->row(current));
}

void PropertyDialog::slotButton(QAbstractButton* pb)
{
	if(buttonBox_->buttonRole(pb) == QDialogButtonBox::ApplyRole)
	{
		apply();
	}
}


void PropertyDialog::manageChange()
{
	Q_FOREACH(PropertyEditor* ed,editors_)
	{
		if(ed->applyChange())
		{
			VProperty* p=ed->property();
			if(p && p->name() != "server")
			{
				configChanged_=true;
			}
		}
	}
}

void PropertyDialog::load(VProperty* p)
{
    prop_=p;
}

void PropertyDialog::writeSettings()
{
	QSettings settings("ECMWF","ecflowUI-PropertyDialog");

	//We have to clear it so that should not remember all the previous values
	settings.clear();

	settings.beginGroup("main");
	settings.setValue("size",size());
	settings.endGroup();
}

void PropertyDialog::readSettings()
{
	QSettings settings("ECMWF","ecflowUI-PropertyDialog");

	settings.beginGroup("main");
	if(settings.contains("size"))
	{
		resize(settings.value("size").toSize());
	}
	else
	{
	  	resize(QSize(550,540));
	}

	settings.endGroup();
}


static SimpleLoader<PropertyDialog> loader("gui");
