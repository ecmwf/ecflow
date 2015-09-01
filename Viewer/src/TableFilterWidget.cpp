//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TableFilterWidget.hpp"

#include <QMenu>
#include <QToolButton>

#include "FilterWidget.hpp"
#include "VFilter.hpp"

TableFilterWidget::TableFilterWidget(QWidget *parent) :
   QWidget(parent),
   filterDef_(0)
{
	setupUi(this);

}

void TableFilterWidget::build(NodeFilterDef* def)
{
	filterDef_=def;

    exprLabel_->hide();

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

	tb->setMenu(menu);
}


