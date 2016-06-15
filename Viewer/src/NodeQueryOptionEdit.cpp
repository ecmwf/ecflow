//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "NodeQueryOptionEdit.hpp"

#include "ComboMulti.hpp"
#include "CustomListWidget.hpp"
#include "NodeQuery.hpp"
#include "StringMatchCombo.hpp"

#include <QtGlobal>
#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>

NodeQueryOptionEdit::NodeQueryOptionEdit(QString optionId,QGridLayout* grid,QWidget* parent) :
    QObject(parent),
    optionId_(optionId),
    initIsOn_(false),
    parent_(parent),
    grid_(grid)
{
    connect(this,SIGNAL(changed()),
            parent_,SLOT(slotOptionEditChanged()));
}

NodeQueryStringOptionEdit::NodeQueryStringOptionEdit(QString optionId,QGridLayout* grid,QWidget* parent) :
    NodeQueryOptionEdit(optionId,grid,parent),
    label_(0),
    matchCb_(0),
    le_(0),
    option_(0)
{
    label_=new QLabel(optionId,parent_);
    matchCb_=new StringMatchCombo(parent_);
    le_=new QLineEdit(parent_);
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    le_->setClearButtonEnabled(true);
#endif

    int row=grid_->rowCount();
    grid_->addWidget(label_,row,0);
    grid_->addWidget(matchCb_,row,1);
    grid_->addWidget(le_,row,1,1,-1);

    connect(le_,SIGNAL(textChanged(QString)),
           this,SLOT(slotEdited(QString)));

    connect(matchCb_,SIGNAL(currentIndexChanged(int)),
           this,SLOT(slotMatchChanged(int)));
}

void NodeQueryStringOptionEdit::init(NodeQuery* query)
{
    initIsOn_=true;
    option_=static_cast<NodeQueryStringOption*>(query->option(optionId_));
    Q_ASSERT(option_);
    Q_ASSERT(option_->name() == optionId_);
    le_->setText(option_->value());
    matchCb_->setMatchMode(option_->matchMode());
    initIsOn_=false;
}

void NodeQueryStringOptionEdit::slotEdited(QString val)
{
    if(initIsOn_)
        return;

    if(option_)
    {
        option_->setValue(val);
        Q_EMIT changed();
    }
}

void NodeQueryStringOptionEdit::slotMatchChanged(int val)
{
    if(initIsOn_)
        return;

    if(option_)
    {
        option_->setMatchMode(matchCb_->matchMode(val));
        Q_EMIT changed();
    }
}

void NodeQueryStringOptionEdit::setVisible(bool st)
{
    label_->setVisible(st);
    matchCb_->setVisible(st);
    le_->setVisible(st);
}

NodeQueryListOptionEdit::NodeQueryListOptionEdit(QString optionId,CustomListWidget* list,
                                                 QToolButton* tb,QWidget* parent) :
     NodeQueryOptionEdit(optionId,0,parent),
     list_(list),
     resetTb_(tb),
     option_(0)
{
    connect(list_,SIGNAL(selectionChanged()),
            this,SLOT(slotListChanged()));

    connect(resetTb_,SIGNAL(clicked()),
            list_,SLOT(clearSelection()));

    list_->addItems(NodeQuery::def(optionId_)->values(),false);
    resetTb_->setEnabled(list_->hasSelection());
}

void NodeQueryListOptionEdit::init(NodeQuery* query)
{
    initIsOn_=true;
    option_=static_cast<NodeQueryListOption*>(query->option(optionId_));
    Q_ASSERT(option_);
    Q_ASSERT(option_->name() == optionId_);
    list_->setSelection(option_->selection());
    initIsOn_=false;
}

void NodeQueryListOptionEdit::slotListChanged()
{
    resetTb_->setEnabled(list_->hasSelection());
    if(initIsOn_)
        return;

    if(option_)
    {
        option_->setSelection(list_->selection());
        Q_EMIT changed();
    }
}

