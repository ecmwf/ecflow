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
#include "NodeQueryOption.hpp"
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

void NodeQueryOptionEdit::init(NodeQuery* query)
{
    init(query->option(optionId_));
}


NodeQueryStringOptionEdit::NodeQueryStringOptionEdit(NodeQueryOption* option,QGridLayout* grid,
                                                     QWidget* parent,bool sameRow) :
    NodeQueryOptionEdit(option->name(),grid,parent),
    label_(0),
    matchCb_(0),
    le_(0),
    option_(0)
{
    label_=new QLabel(option->label() + ":",parent_);
    matchCb_=new StringMatchCombo(parent_);
    le_=new QLineEdit(parent_);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    le_->setClearButtonEnabled(true);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    le_->setPlaceholderText(tr("ANY"));
#endif

    int row=grid_->rowCount();
    if(!sameRow)
    {
        grid_->addWidget(label_,row,0);
        grid_->addWidget(matchCb_,row,1);
        grid_->addWidget(le_,row,2,1,1);
    }
    else
    {
        row=row-1;
        Q_ASSERT(row >= 0);
        grid_->addWidget(label_,row,3);
        grid_->addWidget(matchCb_,row,4);
        grid_->addWidget(le_,row,5,1,-1);
    }

    connect(le_,SIGNAL(textChanged(QString)),
           this,SLOT(slotEdited(QString)));

    connect(matchCb_,SIGNAL(currentIndexChanged(int)),
           this,SLOT(slotMatchChanged(int)));

    init(option);
    Q_ASSERT(option_);
}

void NodeQueryStringOptionEdit::init(NodeQueryOption* option)
{
    initIsOn_=true;
    option_=static_cast<NodeQueryStringOption*>(option);
    Q_ASSERT(option_);
    Q_ASSERT(optionId_ == option_->name());
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

//===================================================================
//
//
//==================================================================

NodeQueryListOptionEdit::NodeQueryListOptionEdit(NodeQueryOption *option,CustomListWidget* list,
                                                 QToolButton* tb,QWidget* parent) :
     NodeQueryOptionEdit(option->name(),0,parent),
     list_(list),
     resetTb_(tb),
     option_(0)
{
    option_=static_cast<NodeQueryListOption*>(option);
    Q_ASSERT(option_);

    connect(list_,SIGNAL(selectionChanged()),
            this,SLOT(slotListChanged()));

    connect(resetTb_,SIGNAL(clicked()),
            list_,SLOT(clearSelection()));

    list_->addItems(option_->values(),false);
    resetTb_->setEnabled(list_->hasSelection());

    init(option_);
}

void NodeQueryListOptionEdit::init(NodeQueryOption* option)
{
    initIsOn_=true;
    option_=static_cast<NodeQueryListOption*>(option);
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

//===================================================================
//
//
//==================================================================

NodeQueryComboOptionEdit::NodeQueryComboOptionEdit(NodeQueryOption *option,QGridLayout* grid, QWidget* parent) :
     NodeQueryOptionEdit(option->name(),grid,parent),
     cb_(0),
     option_(0)
{
    option_=static_cast<NodeQueryComboOption*>(option);
    Q_ASSERT(option_);

    label_=new QLabel(option->label() + ":",parent_);
    cb_=new QComboBox(parent_);

    int row=grid_->rowCount();
    grid_->addWidget(label_,row,0);
    grid_->addWidget(cb_,row,1,1,-1);

    connect(cb_,SIGNAL(currentIndexChanged(int)),
            this,SLOT(slotCbChanged(int)));

    QStringList vals=option_->values();
    QStringList labels=option_->valueLabels();
    for(int i=0; i < vals.count(); i++)
    {
        cb_->addItem(labels[i],vals[i]);
    }

    init(option_);
}

void NodeQueryComboOptionEdit::init(NodeQueryOption* option)
{
    initIsOn_=true;
    option_=static_cast<NodeQueryComboOption*>(option);
    Q_ASSERT(option_);
    Q_ASSERT(option_->name() == optionId_);

    for(int i=0; i < cb_->count(); i++)
    {
        if(cb_->itemData(i).toString() == option_->value())
        {
            cb_->setCurrentIndex(i);
            break;
        }
    }
    initIsOn_=false;
}

void NodeQueryComboOptionEdit::slotCbChanged(int idx)
{
    if(initIsOn_)
        return;

    if(option_)
    {
        option_->setValue(cb_->itemData(idx).toString());
        Q_EMIT changed();
    }
}

void NodeQueryComboOptionEdit::setVisible(bool st)
{
    label_->setVisible(st);
    cb_->setVisible(st);
}

