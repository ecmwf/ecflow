//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//=======================================================================

#include "LogView.hpp"

#include "LogModel.hpp"

LogView::LogView(QWidget *parent) :
    QTreeView(parent)
{
    setProperty("log","1");
    setRootIsDecorated(false);
    setUniformRowHeights(true);
    setAlternatingRowColors(false);
    setItemDelegate(new LogDelegate(this));
    setContextMenuPolicy(Qt::ActionsContextMenu);
}

void LogView::setLogModel(LogModel* logModel)
{
    logModel_=logModel;
    QTreeView::setModel(logModel_);

    connect(logModel_,SIGNAL(rerender()),
            this,SLOT(rerender()));

    connect(logModel_,SIGNAL(scrollToHighlightedPeriod()),
            this,SLOT(scrolltToHighlightPeriod()));
}

void LogView::setModel(QAbstractItemModel*)
{
    Q_ASSERT(0);
}

#if 0
void LogView::setHighlightPeriod(qint64 start,qint64 end,qint64 tolerance)
{
    Q_ASSERT(logModel_);
    logModel_->setHighlightPeriod(start,end,tolerance);
    viewport()->update();
    QModelIndex idx=logModel_->highlightPeriodIndex();
    if(idx.isValid())
        scrollTo(idx,QAbstractItemView::PositionAtCenter);
}
#endif

void LogView::rerender()
{
     viewport()->update();
}

void LogView::scrolltToHighlightPeriod()
{
    QModelIndex idx=logModel_->highlightPeriodIndex();
    if(idx.isValid())
        scrollTo(idx,QAbstractItemView::PositionAtCenter);
}

