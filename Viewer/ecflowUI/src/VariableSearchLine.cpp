//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <assert.h>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeView>

#include "VariableSearchLine.hpp"

VariableSearchLine::VariableSearchLine(QWidget *parent) :
    AbstractSearchLine(parent), 
    view_(0),
    currentResultItem_(-1)
{
	label_->hide();
	closeTb_->hide();
}

VariableSearchLine::~VariableSearchLine()
{

}

//This should only be called once!!
void VariableSearchLine::setView(QTreeView* view)
{
    //We should disconnect from the previous view ...
	view_=view;

    //We detect when the sorting changes in the view
    connect(view_->header(),SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)),
    		this,SLOT(slotSortHappened(int,Qt::SortOrder)));

    //At this point the model has to be set on the view!!!
    assert(view_->model() != 0);

    //We detect when the model is reset
    connect(view_->model(),SIGNAL(modelReset()),
    		this,SLOT(slotUpdate()));

    //We should detect when the model changes!!!!
    connect(view_->model(),SIGNAL(dataChanged(QModelIndex,QModelIndex)),
       		this,SLOT(slotUpdate(QModelIndex,QModelIndex)));
}    

void VariableSearchLine::slotFind(QString txt)
{
    if(!view_)
        return;
    
    if(txt.simplified().isEmpty() && resultItems_.isEmpty())
    {
        return;
    }   

    QModelIndex current=view_->currentIndex();

    //The (sort) model redoes the match.
    resultItems_=view_->model()->match(current,Qt::DisplayRole,txt,-1,
    		            Qt::MatchStartsWith | Qt::MatchRecursive | Qt::MatchWrap);

    //The view needs to be rerendered!
    view_->dataChanged(QModelIndex(),QModelIndex());
    
    if(resultItems_.count() > 0)
    {
        selectIndex(resultItems_[0]);
        currentResultItem_=0;
        updateButtons(true);
    }  
    else
    {  
        currentResultItem_=0;
        updateButtons(false);
    }   
    
}

void VariableSearchLine::slotFindNext()
{
    if(!view_)
        return;
    
    
    if(status_==true && resultItems_.count() > 0)
    {
        currentResultItem_++;
        if(currentResultItem_ >= resultItems_.count())  
        {
            currentResultItem_=0;
        }
        selectIndex(resultItems_[currentResultItem_]);
    }
}

void VariableSearchLine::slotFindPrev()
{
    if(!view_)
            return;
    
    if(status_==true && resultItems_.count() > 0)
    {
        currentResultItem_--;
        if(currentResultItem_ <0)   
        {
            currentResultItem_=resultItems_.count()-1;
        }
        selectIndex(resultItems_[currentResultItem_]);
    }
}

void VariableSearchLine::selectIndex(const QModelIndex& index)
{
    if(!view_)
        return;
    
    view_->setCurrentIndex(index);
    //emit indexSelected(index);
}

//Called when sorting changed in the view
void VariableSearchLine::slotSortHappened(int,Qt::SortOrder)
{
	slotUpdate();
}

void VariableSearchLine::slotUpdate()
{
	slotFind(searchLine_->text());
}

void VariableSearchLine::slotUpdate(const QModelIndex&,const QModelIndex&)
{
	slotUpdate();
}



