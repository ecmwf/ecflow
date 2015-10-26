//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "AttributeSearchPanel.hpp"

#include "ComboMulti.hpp"

#include <QtGlobal>
#include <QDebug>
#include <QStandardItemModel>


AttributeSearchWidget::AttributeSearchWidget(QString labelName,QWidget* parent) :
    QWidget(parent),
	labelName_(labelName)
{
}


//======================================================
//
// EventSearchWidget
//
//======================================================

EventSearchWidget::EventSearchWidget(QWidget *parent) :
	AttributeSearchWidget("event",parent)
{
	setupUi(this);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)

    nameLe_->setClearButtonEnabled(true);
#endif

    connect(nameLe_,SIGNAL(textEdited(QString)),
    		this,SLOT(slotTextChanged(QString)));
}

void EventSearchWidget::slotTextChanged(QString)
{
	Q_EMIT queryChanged();
}

QString EventSearchWidget::query()
{
	QString t;

	QString name=nameLe_->text().simplified();
	qDebug() << "event text" << name;
	if(!name.isEmpty())
	{
		t="event_name = \'" + name + "\'";
	}

	if(t.isEmpty())
	{
		t="event";
	}

	return t;
}

//======================================================
//
// LabelSearchWidget
//
//======================================================

LabelSearchWidget::LabelSearchWidget(QWidget *parent) :
	AttributeSearchWidget("label",parent)
{
	setupUi(this);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    nameLe_->setClearButtonEnabled(true);
    valueLe_->setClearButtonEnabled(true);
#endif

    connect(nameLe_,SIGNAL(textEdited(QString)),
    	this,SLOT(slotTextChanged(QString)));

    connect(valueLe_,SIGNAL(textEdited(QString)),
    	 this,SLOT(slotTextChanged(QString)));
}

void LabelSearchWidget::slotTextChanged(QString)
{
	Q_EMIT queryChanged();
}

QString LabelSearchWidget::query()
{
	QString t;

	QString name=nameLe_->text().simplified();
	qDebug() << "label text" << name;
	if(!name.isEmpty())
	{
		t="label_name = \'" + name + "\'";
	}

	if(t.isEmpty())
	{
		t="label";
	}

	return t;
}

//======================================================
//
// AttributeSearchPanel
//
//======================================================

AttributeSearchPanel::AttributeSearchPanel(QWidget* parent) : QWidget(parent)
{
	setupUi(this);

	model_=new QStandardItemModel(this);

	QStringList header;
	header << "" << "Type";
	model_->setHorizontalHeaderLabels(header);


	addPage(new EventSearchWidget(this));
	addPage(new LabelSearchWidget(this));

	tree_->setModel(model_);

	QFont f;
	QFontMetrics fm(f);
	int w=fm.width("Generated AAAAA");
	int h=fm.height()*10;
	tree_->setFixedWidth(w);
	tree_->setFixedHeight(h);

	tree_->setRootIsDecorated(false);
	tree_->resizeColumnToContents(0);

	connect(tree_,SIGNAL(clicked(QModelIndex)),
			this,SLOT(slotChangePage(QModelIndex)));

}

void AttributeSearchPanel::addPage(AttributeSearchWidget* page)
{
	QString name=page->labelName();

	QStandardItem *item1=new QStandardItem();
	item1->setCheckable(true);item1->setCheckable(true);
	item1->setCheckState(Qt::Unchecked);

	QStandardItem *item2=new QStandardItem(name);
	item2->setEditable(false);

	QList<QStandardItem*> lst;
	lst << item1 << item2;
	model_->appendRow(lst);

	page->setEnabled(false);

	stacked_->addWidget(page);
	pages_[name]=page;

	connect(page,SIGNAL(queryChanged()),
			this,SLOT(buildQuery()));
}

void AttributeSearchPanel::slotChangePage(const QModelIndex& idx)
{
	if(!idx.isValid())
		return;

	QModelIndex idxN=model_->index(idx.row(),1);
	QString name=model_->data(idxN).toString();

	if(QWidget *p=pages_.value(name,NULL))
	{
		if(idx.column() == 0)
		{
			bool b=model_->data(idx,Qt::CheckStateRole).toBool();
			p->setEnabled(b);
			buildQuery();
		}
		stacked_->setCurrentWidget(p);
	}
}

void AttributeSearchPanel::buildQuery()
{
	query_.clear();

	QMap<QString,AttributeSearchWidget*>::const_iterator it = pages_.constBegin();
	while (it != pages_.constEnd())
	 {
		 QString name=it.key();
		 AttributeSearchWidget*p=it.value();

		 if(p->isEnabled())
		 {
			 QString res=p->query();
			 if(!query_.isEmpty())
				 query_+=" or ";
			 query_+=res;
		 }
	     ++it;
	 }

	 Q_EMIT queryChanged();
}






