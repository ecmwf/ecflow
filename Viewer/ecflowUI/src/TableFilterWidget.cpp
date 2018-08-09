//============================================================================
// Copyright 2009-2017 ECMWF.
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
#include "NodeQueryOption.hpp"
#include "ServerFilter.hpp"
#include "UiLog.hpp"
#include "VFilter.hpp"

#include <cassert>

TableFilterWidget::TableFilterWidget(QWidget *parent) :
   QWidget(parent)
{
	setupUi(this);

	connect(queryTe_,SIGNAL(clicked()),
			this,SLOT(slotEdit()));

	numLabel_->hide();

	slotTotalNumChanged(0);

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
        UiLog().dbg() << "new table query: " << filterDef_->query()->query();
	}
}

void TableFilterWidget::build(NodeFilterDef* def,ServerFilter *sf)
{
	filterDef_=def;
	serverFilter_=sf;

	connect(filterDef_,SIGNAL(changed()),
			this,SLOT(slotDefChanged()));

	slotDefChanged();
}

void TableFilterWidget::slotDefChanged()
{
	QColor bg(240,240,240);
    queryTe_->setHtml(filterDef_->query()->sqlQuery());
}

void TableFilterWidget::slotHeaderFilter(QString column,QPoint globalPos)
{
	NodeQuery *q=filterDef_->query()->clone();
	if(column == "status")
	{
		auto *menu=new QMenu(this);
		//stateFilterMenu_=new StateFilterMenu(menuState,filter_->menu());

		NodeStateFilter sf;
        NodeQueryListOption* op=q->stateOption();
        Q_ASSERT(op);
        //if(!op->selection().isEmpty())
        sf.setCurrent(op->selection());

        //The menu takes ownership of it
        new VParamFilterMenu(menu,&sf,"Status filter",
                          VParamFilterMenu::FilterMode,VParamFilterMenu::ColourDecor);

		if(menu->exec(globalPos) != nullptr)
		{
            //if(sf.isComplete())
            //   op->setSelection(QStringList());
            //else
                op->setSelection(sf.currentAsList());

            //this will create deep copy so we can delet q in the end
            filterDef_->setQuery(q);
		}

		delete menu;
	}

	delete q;
}

void TableFilterWidget::slotTotalNumChanged(int n)
{
	numLabel_->setText(tr("Total: ") + QString::number(n));
}
