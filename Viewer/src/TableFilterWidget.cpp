//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TableFilterWidget.hpp"

#include <QDebug>
#include <QMenu>
#include <QToolButton>

#include "FilterWidget.hpp"
#include "NodeFilterDialog.hpp"
#include "NodeQuery.hpp"
#include "ServerFilter.hpp"
#include "VFilter.hpp"

#include <assert.h>

TableFilterWidget::TableFilterWidget(QWidget *parent) :
   QWidget(parent),
   filterDef_(0),
   serverFilter_(0)
{
	setupUi(this);

	//queryTe_->setFixedHeight(18);
}

void TableFilterWidget::slotEdit()
{
	assert(filterDef_);
	assert(serverFilter_);

	NodeFilterDialog d(this);
	d.setServerFilter(serverFilter_);
	d.setQuery(filterDef_->query());
	if(d.exec() == QDialog::Accepted)
	{
		filterDef_->setQuery(d.query());
		qDebug() << "query" << d.query()->queryString();
		qDebug() << "query" << filterDef_->query()->queryString();
		qDebug() << "table query" << filterDef_->query()->extQueryString(false);
		//queryTe_->setPlainText(filterDef_->query()->extQueryString());
	}
}

void TableFilterWidget::build(NodeFilterDef* def,ServerFilter *sf)
{
	filterDef_=def;
	serverFilter_=sf;
	qDebug() << filterDef_->query()->stateSelection();

	connect(filterDef_,SIGNAL(changed()),
			this,SLOT(slotDefChanged()));

	/*exprLabel_->hide();

	//
    QToolButton *tb=new QToolButton(this);
    tb->setText("Status");
    tb->setAutoRaise(true);
    tb->setPopupMode(QToolButton::InstantPopup);

	buttonLayout_->addWidget(tb);

	QMenu *menu=new QMenu(this);

	//stateFilterMenu_=new StateFilterMenu(menuState,filter_->menu());
	VParamFilterMenu* sfm= new VParamFilterMenu(menu,filterDef_->nodeState(),
			VParamFilterMenu::ColourDecor);

	tb->setMenu(menu);*/

	slotDefChanged();



}

void TableFilterWidget::slotDefChanged()
{
	/*QStringList lst=filterDef_->query()->extQueryString();
	QString q;
	if(lst.count() > 1)
		q=lst.join(" and ");
	else if(lst.count()==1)
		q=lst.front();*/

	QColor bg(240,240,240);
	queryTe_->setHtml(filterDef_->query()->extQueryHtml(false,bg,0));
}

void TableFilterWidget::slotHeaderFilter(QString column,QPoint globalPos)
{
	NodeQuery *q=filterDef_->query()->clone();
	if(column == "status")
	{
		QMenu *menu=new QMenu(this);
		//stateFilterMenu_=new StateFilterMenu(menuState,filter_->menu());

		NodeStateFilter sf;
		sf.current(q->stateSelection());
		VParamFilterMenu* sfm= new VParamFilterMenu(menu,&sf,"Status filter",
				          VParamFilterMenu::FilterMode,VParamFilterMenu::ColourDecor);

		if(menu->exec(globalPos) != NULL)
		{
			q->setStateSelection(sf.currentAsList());
			filterDef_->setQuery(q);
		}

		delete menu;
	}

	delete q;
}
