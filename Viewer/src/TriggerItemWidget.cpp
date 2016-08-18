//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerItemWidget.hpp"

#include "VNode.hpp"
#include "TriggerCollector.hpp"
#include "TriggerView.hpp"


//========================================================
//
// TriggerItemWidget
//
//========================================================

TriggerItemWidget::TriggerItemWidget(QWidget *parent) : QWidget(parent)
{
    setupUi(this);
}

QWidget* TriggerItemWidget::realWidget()
{
	return this;
}

void TriggerItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();

    info_=info;
    //messageLabel_->hide();

    //Info must be a node
    if(info_ && info_->isNode() && info_->node())
    {
        load();
    }
}


void TriggerItemWidget::load()
{
    if(info_ && info_->isNode() && info_->node())
    {
        VNode *n=info_->node();
        TriggerListCollector c(0,"Nodes/attributes triggering this node",dependency());
        n->triggers(&c);

        TriggerListCollector c1(0,"Nodes triggered by this node",dependency());
        n->triggered(&c1);

        triggerBrowser_->setHtml(c.text()+c1.text());
    }
}

void TriggerItemWidget::clearContents()
{
	InfoPanelItem::clear();
}


void TriggerItemWidget::on_dependTb__toggled(bool)
{
    load();
}

bool TriggerItemWidget::dependency() const
{
    return dependTb_->isChecked();
}

static InfoPanelItemMaker<TriggerItemWidget> maker1("triggers");
