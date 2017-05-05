//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VARIABLESEARCHLINE_HPP_
#define VARIABLESEARCHLINE_HPP_

#include "AbstractSearchLine.hpp"

#include <QModelIndex>

class QTreeView;

class  VariableSearchLine : public AbstractSearchLine
{
    Q_OBJECT

public:
     explicit VariableSearchLine(QWidget *parent);
    ~VariableSearchLine();
    void setView(QTreeView* view);

public Q_SLOTS:
    void slotFind(QString);
    void slotFindNext();
    void slotFindPrev();
    void slotFindNext(bool) { slotFindNext();}
    void slotFindPrev(bool) {slotFindPrev();}
    void slotSortHappened(int,Qt::SortOrder);
    void slotUpdate();
    void slotUpdate(const QModelIndex&,const QModelIndex&);

protected:   
    void selectIndex(const QModelIndex& index);
    void clearRequested() {}
    
    QTreeView* view_;
    QModelIndexList resultItems_;
    int currentResultItem_;
};

#endif
