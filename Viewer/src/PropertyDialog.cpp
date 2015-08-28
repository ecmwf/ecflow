//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "PropertyDialog.hpp"

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

	//list_ = new QListWidget(this);
	//list_->setFlow(QListView::LeftToRight);
    //list_->setViewMode(QListView::IconMode);
	//list_->setIconSize(QSize(32, 32));
	//list_->setMovement(QListView::Static);
	//list_->setMaximumHeight(70);
	//list_->setSpacing(5);

	connect(list_,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(slotChangePage(QListWidgetItem*,QListWidgetItem*)));


	QFont f;
	QFontMetrics fm(f);
	list_->setMaximumWidth(fm.width("Server state ATWWWWW"));

	build();
}

//Build the property tree from the the definitions
void PropertyDialog::build()
{
	if(prop_)
	{
		Q_FOREACH(VProperty* vPage,prop_->children())
		{
			  PropertyEditor* ed=new PropertyEditor(this);
			  ed->edit(vPage);
			  addPage(ed,QIcon(),vPage->name());
			  editors_ << ed;
		}
	}
}

void PropertyDialog::accept()
{
	manageChange();

    QDialog::accept();
}    

void PropertyDialog::addPage(QWidget *w,QIcon icon,QString txt)
{
	QListWidgetItem *item = new QListWidgetItem(list_);
     	item->setIcon(icon);
     	item->setText(txt);
     	item->setTextAlignment(Qt::AlignHCenter);
     	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

	page_->addWidget(w);
}

void PropertyDialog::slotChangePage(QListWidgetItem *current, QListWidgetItem *previous)
{
     if (!current)
         current = previous;

     page_->setCurrentIndex(list_->row(current));
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

static SimpleLoader<PropertyDialog> loader("gui");
