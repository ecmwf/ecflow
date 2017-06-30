//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "AbstractNodeView.hpp"

#include "Animation.hpp"
#include "ExpandState.hpp"
#include "TreeNodeModel.hpp"
#include "CompactNodeViewDelegate.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"

#include <QtAlgorithms>
#include <QApplication>
#include <QDebug>
#include <QItemSelectionRange>
#include <QPainter>
#include <QMouseEvent>
#include <QScrollBar>
#include <QStack>
#include <QStyledItemDelegate>
#include <QTimerEvent>
#include <QToolTip>

//#define _UI_QABSTRACTNODEVIEW_DEBUG

AbstractNodeView::AbstractNodeView(TreeNodeModel* model,QWidget* parent) :
    QAbstractScrollArea(parent),
    model_(model),
    verticalScrollMode_(ScrollPerItem),
    rowCount_(0),
    maxRowWidth_(0),
    lastViewedItem_(0),
    topMargin_(4),
    leftMargin_(4),
    itemGap_(12),
    connectorGap_(1),
    expandConnectorLenght_(20),
    noSelectionOnMousePress_(false),
    connectorColour_(Qt::black)
{  
    expandConnectorLenght_=itemGap_-2*connectorGap_;

    setContextMenuPolicy(Qt::CustomContextMenu);

    viewport()->setBackgroundRole(QPalette::Window);

    //We attach the model.
    attachModel();

    //We should call reset here but it has a pure virtual method,
    //so cannot be called from the constructor. We need to call it from
    //the constructor of the derived classes
}

AbstractNodeView::~AbstractNodeView()
{

}

//Connect the models signal to the view. Must only be called once!!
void AbstractNodeView::attachModel()
{
    //Standard signals from the model
    connect(model_,SIGNAL(modelReset()),
        this,SLOT(reset()));

    connect(model_,SIGNAL(rowsInserted(QModelIndex,int,int)),
              this, SLOT(rowsInserted(QModelIndex,int,int)));

    connect(model_,SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(rowsAboutToBeRemoved(QModelIndex,int,int)));

    connect(model_,SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(rowsRemoved(QModelIndex,int,int)));

    connect(model_,SIGNAL(dataChanged(const QModelIndex&,const QModelIndex&)),
        this,SLOT(dataChanged(const QModelIndex&,const QModelIndex&)));

    //The selection model
    selectionModel_ = new QItemSelectionModel(model_, this);
    connect(model_, SIGNAL(destroyed()), selectionModel_, SLOT(deleteLater()));

    connect(selectionModel_, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged(QItemSelection,QItemSelection)));

    //broadcast the selection change
    connect(selectionModel_, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SIGNAL(selectionChangedInView(QItemSelection,QItemSelection)));

    connect(selectionModel_, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentChanged(QModelIndex,QModelIndex)));

}

void AbstractNodeView::mousePressEvent(QMouseEvent* event)
{
    //When the expand indicataor is pressed
    if(event->button() == Qt::LeftButton)
    {
        int viewItemIndex=itemAtCoordinate(event->pos());
        if(viewItemIndex != -1 && viewItems_[viewItemIndex].hasChildren)
        {
            if(isPointInExpandIndicator(viewItemIndex,event->pos()))
            {
                if(viewItems_[viewItemIndex].expanded)
                {
                    collapse(viewItemIndex);
                    updateRowCount();
                    updateScrollBars();
                    viewport()->update();
                }
                else
                {
                    expand(viewItemIndex);
                }
            }
            return;
        }
    }

    QPoint pos = event->pos();
    QPersistentModelIndex index = indexAt(pos);

    pressedIndex_ = index;

    //Get the selection flags
    QItemSelectionModel::SelectionFlags command = selectionCommand(index, event);

    noSelectionOnMousePress_ = command == QItemSelectionModel::NoUpdate || !index.isValid();

#ifdef _UI_QABSTRACTNODEVIEW_DEBUG
    UiLog().dbg() << "TreeNodeViewBase::mousePressEvent --> current=" << currentIndex().data().toString() <<
                     " pressed=" << pressedIndex_.data().toString() <<
                     " pos=" << pos << " pressedRef=" << pressedRefIndex_.data().toString();
#endif

    if((command & QItemSelectionModel::Current) == 0)
        pressedRefIndex_ = index;
    else if(!pressedRefIndex_.isValid())
        pressedRefIndex_ = currentIndex();

    QPoint pressedRefPosition=visualRect(pressedRefIndex_).center();


#ifdef _UI_QABSTRACTNODEVIEW_DEBUG
    UiLog().dbg() << " pressedRefPosition=" << pressedRefPosition << " visrect=" << visualRect(currentIndex()) <<
                     " center=" << visualRect(currentIndex()).center() << " pressedRef=" << indexAt(pressedRefPosition).data().toString() <<
                     " pressedRef=" << pressedRefIndex_.data().toString();
#endif

    if(index.isValid())
    {
        selectionModel_->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
        QPoint p1=pressedRefPosition;
        QRect rect(p1,QSize(pos.x()-p1.x(),pos.y()-p1.y()));
#ifdef _UI_QABSTRACTNODEVIEW_DEBUG
        UiLog().dbg() << " rect=" << rect << " p1=" << p1 << " p2=" << pos ;
#endif
        setSelection(rect, command);
    }
    else
    {
        //Forces a finalize even if mouse is pressed, but not on a item
        selectionModel_->select(QModelIndex(), QItemSelectionModel::Select);
    }

    if(event->button() == Qt::MidButton)
    {
        int viewItemIndex=itemAtCoordinate(event->pos());
        if(viewItemIndex != -1 && viewItems_[viewItemIndex].hasChildren)
        {
#ifdef _UI_QABSTRACTNODEVIEW_DEBUG
            UiLog().dbg() << " midbutton index=" << viewItemIndex << " name=" <<
                               viewItems_[viewItemIndex].index.data().toString();
#endif
            if(viewItems_[viewItemIndex].expanded)
            {
                collapse(viewItemIndex);
                updateRowCount();
                updateScrollBars();
                viewport()->update();
            }
            else
            {
                expand(viewItemIndex);
            }
        }
    }
}

void AbstractNodeView::mouseReleaseEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    QPersistentModelIndex index = indexAt(pos);

    if(selectionModel_ && noSelectionOnMousePress_)
    {
        noSelectionOnMousePress_ = false;
        selectionModel_->select(index, selectionCommand(index, event));
    }
}

void AbstractNodeView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        int viewItemIndex=itemAtCoordinate(event->pos());
        if(viewItemIndex != -1)
        {
            if(viewItems_[viewItemIndex].hasChildren)
            {
#ifdef _UI_QABSTRACTNODEVIEW_DEBUG
                UiLog().dbg() << "CompactNodeView::mousePressEvent " << viewItemIndex << " name=" <<
                               viewItems_[viewItemIndex].index.data().toString();
#endif
                if(viewItems_[viewItemIndex].expanded)
                {
                    collapse(viewItemIndex);
                }
                else
                {
                    expand(viewItemIndex);
                }
                updateRowCount();
                updateScrollBars();
                viewport()->update();
            }
            else
            {
                Q_EMIT doubleClicked(viewItems_[viewItemIndex].index);
            }
       }
    }
}

void AbstractNodeView::keyPressEvent(QKeyEvent *event)
{
    QModelIndex current = currentIndex();

    if (current.isValid())
    {
        switch(event->key())
        {
        case Qt::Key_Plus:
            expand(current);
            break;
        case Qt::Key_Minus:
            collapse(current);
            break;
        }
    }

    QAbstractScrollArea::keyPressEvent(event);
}

bool AbstractNodeView::viewportEvent(QEvent *event)
{
    if(event->type() == QEvent::ToolTip)
    {
        QHelpEvent *he = static_cast<QHelpEvent*>(event);
        const QModelIndex index = indexAt(he->pos());

        //see qbatractitemdelegate::helpEvent()
        QVariant tooltip = index.data(Qt::ToolTipRole);
        if(tooltip.canConvert<QString>())
        {
              QToolTip::showText(he->globalPos(),tooltip.toString(),this);
              return true;
        }
        return false;
    }
    return QAbstractScrollArea::viewportEvent(event);
}

void AbstractNodeView::timerEvent(QTimerEvent *event)
{
    if(event->timerId() == delayedWidth_.timerId())
    {
        updateScrollBars();
        viewport()->update();
        delayedWidth_.stop();
    }
}

void AbstractNodeView::reset()
{
    viewItems_.clear();
    rowCount_=0;
    maxRowWidth_=0;
    expandedIndexes.clear();
    pressedRefIndex_=QPersistentModelIndex(QModelIndex());
    //currentIndexSet_ = false;
    if(selectionModel_)
        selectionModel_->reset();

    layout(-1,false,false,false);
    updateRowCount();
    updateScrollBars();
}

/*
 Informs the view that the rows from the start row to the end row
 inclusive have been inserted into the parent model item.
*/
void AbstractNodeView::rowsInserted(const QModelIndex& parent,int start,int end)
{
    const int parentItem = viewIndex(parent);

    //If the item is expanded we need to relayout the whole tree
    if(((parentItem != -1) && viewItems_[parentItem].expanded) || (parent == root_))
    {
        doItemsLayout();
    }

    //the parent just went from 0 children to more. update to re-paint the decoration
    else if(parentItem != -1 && (model_->rowCount(parent) == end - start + 1))
    {
        viewItems_[parentItem].hasChildren = true;
        viewport()->update();
    }
}

/*
  Informs the view that the rows from the start row to the  end row
  inclusive are about to removed from the given parent model item.
*/
void AbstractNodeView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    //TODO: the selection has to be adjusted!!!
    //QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);

    //A safety measure
    viewItems_.clear();
}

/*
    Informs the view that the rows from the start row to the  end row
    inclusive have been removed from the given  parent model item.
*/
void AbstractNodeView::rowsRemoved(const QModelIndex &parent, int start, int end)
{
    doItemsLayout(true);
}

void AbstractNodeView::doItemsLayout(bool hasRemovedItems)
{
    if(hasRemovedItems)
    {
        //clean the QSet that may contains old (and thus invalid) indexes
        QSet<QPersistentModelIndex>::iterator it = expandedIndexes.begin();
        while (it != expandedIndexes.constEnd())
        {
            if (!it->isValid())
                it = expandedIndexes.erase(it);
            else
                ++it;
        }
        //TODO: do we need to clear the selectionmodel?
    }

    viewItems_.clear(); // prepare for new layout
    rowCount_=0;
    maxRowWidth_=0;
    pressedRefIndex_=QPersistentModelIndex(QModelIndex());

    QModelIndex parent = root_;
    if(model_->hasChildren(parent))
    {
        layout(-1,false,false,false);
    }
    updateRowCount();
    updateScrollBars();
    viewport()->update();
}


void AbstractNodeView::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    paint(&painter,event->region());
}

void AbstractNodeView::slotRepaint(Animation* an)
{
    if(!an)
        return;

    Q_FOREACH(VNode* n,an->targets())
    {
        update(model_->nodeToIndex(n));
    }
}

//Updates the area occupied by the given index.
void AbstractNodeView::update(const QModelIndex &index)
{
    if (index.isValid())
    {
        const QRect rect = visualRect(index);
        //this test is important for peformance reasons
        //For example in dataChanged if we simply update all the cells without checking
        //it can be a major bottleneck to update rects that aren't even part of the viewport
        if(viewport()->rect().intersects(rect))
        {
#ifdef _UI_QABSTRACTNODEVIEW_DEBUG
            UiLog().dbg() << "update -->" << index.data().toString() << " rect=" << rect;
#endif
            viewport()->update(rect);
        }
    }
}

/*
    This slot is called when items are changed in the model. The
    changed items are those from topLeft to bottomRight
    inclusive. If just one item is changed topLeft ==
    bottomRight.
*/
void AbstractNodeView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    // Single item changed
    if (topLeft == bottomRight && topLeft.isValid())
    {
        update(topLeft);
        return;
    }

    viewport()->update();
}

void AbstractNodeView::resizeEvent(QResizeEvent *event)
{
    QAbstractScrollArea::resizeEvent(event);
    updateScrollBars();
    viewport()->update();
}

void AbstractNodeView::doDelayedWidthAdjustment()
{
    if(!delayedWidth_.isActive())
    {
        delayedWidth_.start(0,this);
    }
}

int AbstractNodeView::translation() const
{
    return horizontalScrollBar()->value();
}

void AbstractNodeView::scrollTo(const QModelIndex &index)
{
    if(!index.isValid())
        return;

    //d->executePostedLayout();
    updateScrollBars();

    // Expand all parents if the parent(s) of the node are not expanded.
    QList<QModelIndex> parentLst;
    QModelIndex parent = index.parent();
    while(parent.isValid())
    {
        parentLst.prepend(parent);
        parent = model_->parent(parent);
    }

    Q_FOREACH(QModelIndex pt,parentLst)
    {
        if(!isExpanded(pt))
            expand(pt);
    }

    int item = viewIndex(index);
    if (item < 0)
        return;

    if (verticalScrollMode_ == ScrollPerItem)
    {
        int row=itemRow(item);

        int top = verticalScrollBar()->value();
        int bottom = top + verticalScrollBar()->pageStep();

        if (row >= top && row < bottom)
        {
            // nothing to do
        }
        else if(row < top)
        {
            verticalScrollBar()->setValue(row);
        }
        else
        {
            verticalScrollBar()->setValue(row);
#if 0
            const int currentItemHeight = viewItem_[item].height;
            int y = area.height();
            if (y > currentItemHeight)
            {
                while (item >= 0)
                {
                    y -= viewItem_[item].height;
                    if (y < 0)
                    { //there is no more space left
                        item++;
                        break;
                    }
                    item--;
                }
            }
            verticalScrollBar()->setValue(item);
#endif
        }
    }
    else // ScrollPerPixel
    {

    }

    // horizontal
    int viewportWidth = viewport()->width();
    int xp=viewItems_[item].x;
    int horizontalPosition=xp-horizontalScrollBar()->value();

    if( horizontalPosition< 0)
    {
        xp-=10;
        if(xp < 0) xp=0;
        horizontalScrollBar()->setValue(xp);
    }
    else if(horizontalPosition > viewportWidth)
    {
        xp-=10;
        horizontalScrollBar()->setValue(xp);
    }
}


//point is in viewport coordinates
QModelIndex AbstractNodeView::indexAt(const QPoint &point) const
{
    int item=itemAtCoordinate(point);
    return (item>=0)?viewItems_[item].index:QModelIndex();
}

QModelIndex AbstractNodeView::modelIndex(int i) const
{
    if(i < 0 || i >= viewItems_.size())
        return QModelIndex();

    return viewItems_[i].index;
}

//Returns the index of the view item representing the given index
int AbstractNodeView::viewIndex(const QModelIndex& index) const
{
    if(!index.isValid() || viewItems_.empty())
        return -1;

    const int totalCount = static_cast<int>(viewItems_.size());
    const QModelIndex topIndex = index.sibling(index.row(), 0);
    const int row = topIndex.row();
    const qint64 internalId = topIndex.internalId();

    // We start nearest to the lastViewedItem
    int localCount = qMin(lastViewedItem_ - 1, totalCount - lastViewedItem_);
    for(int i = 0; i < localCount; ++i)
    {
        const QModelIndex &idx1 = viewItems_[lastViewedItem_ + i].index;
        if(idx1.row() == row && idx1.internalId() == internalId)
        {
            lastViewedItem_ = lastViewedItem_ + i;
            return lastViewedItem_;
        }
        const QModelIndex &idx2 = viewItems_[lastViewedItem_ - i - 1].index;
        if(idx2.row() == row && idx2.internalId() == internalId)
        {
            lastViewedItem_ = lastViewedItem_ - i - 1;
            return lastViewedItem_;
        }
    }

    for(int j = qMax(0, lastViewedItem_ + localCount); j < totalCount; ++j)
    {
        const QModelIndex &idx = viewItems_[j].index;
        if (idx.row() == row && idx.internalId() == internalId)
        {
            lastViewedItem_ = j;
            return j;
        }
    }
    for(int j = qMin(totalCount, lastViewedItem_ - localCount) - 1; j >= 0; --j)
    {
        const QModelIndex &idx = viewItems_[j].index;
        if (idx.row() == row && idx.internalId() == internalId)
        {
            lastViewedItem_ = j;
            return j;
        }
    }

    // nothing found
    return -1;
}

void AbstractNodeView::insertViewItems(int pos, int count, const TreeNodeViewItem &viewItem)
{
    ViewItemIterator it=viewItems_.begin();
    viewItems_.insert(it+pos,count,viewItem);

    //We need to update the parentItem in the items after the insertion
    const int itemsCount=static_cast<int>(viewItems_.size());
    for(int i = pos + count; i < itemsCount; i++)
        if (viewItems_[i].parentItem >= pos)
            viewItems_[i].parentItem += count;
}


void AbstractNodeView::removeViewItems(int pos, int count)
{
    ViewItemIterator it=viewItems_.begin();
    viewItems_.erase(it+pos,it+pos+count);

    //We need to update the parentItem in the items after the deletion
    const int itemsCount=static_cast<int>(viewItems_.size());
    for(int i=0; i < itemsCount; i++)
        if(viewItems_[i].parentItem >= pos)
           viewItems_[i].parentItem -= count;
}


//---------------------------------------
// Expand / collapse
//---------------------------------------

int AbstractNodeView::totalNumOfChildren(const QModelIndex& idx,int& num) const
{
    int count=model_->rowCount(idx);
    num+=count;
    for(int i=0; i < count; i++)
    {
        QModelIndex chIdx=model_->index(i,0,idx);
        totalNumOfChildren(chIdx,num);
    }
}

int AbstractNodeView::totalNumOfExpandedChildren(const QModelIndex& idx,int& num) const
{
    int count=model_->rowCount(idx);
    num+=count;
    for(int i=0; i < count; i++)
    {
        QModelIndex chIdx=model_->index(i,0,idx);
        if(isIndexExpanded(chIdx))
        {
            totalNumOfExpandedChildren(chIdx,num);
        }
    }
}

void AbstractNodeView::expand(int item)
{
    if(item != -1 && !viewItems_[item].expanded)
    {
        QModelIndex idx=viewItems_[item].index;

        //mark the item as expanded
        storeExpanded(idx);
        viewItems_[item].expanded = true;

        //The total number items to be inserted
        int total=0;
        totalNumOfExpandedChildren(idx,total);

        //Insert the required number items
        ViewItemIterator it=viewItems_.begin();
        viewItems_.insert(it+item+1,total,TreeNodeViewItem());

        //recursively relayout the item
        layout(item,false,false,true);

        UI_ASSERT(viewItems_[item].total==total,"viewItems_[" << item << "].total=" << viewItems_[item].total <<
                  " total=" << total);

        //We need to update the parentItem in the items after the insertion.
        //When layout() is called with the given arguments it is delayed to
        //this point to gain performance!
        const int itemsCount=static_cast<int>(viewItems_.size());
        int count=viewItems_[item].total;
        for(int i = item + count+1; i < itemsCount; i++)
            if (viewItems_[i].parentItem >= item)
                viewItems_[i].parentItem += count;

        //update the scrollbars and rerender the viewport
        updateRowCount();
        updateScrollBars();
        viewport()->update();
    }
}

void AbstractNodeView::expand(const QModelIndex &idx)
{
    int item=viewIndex(idx);
    expand(item);
}

void AbstractNodeView::expandAll(const QModelIndex& idx)
{
    int item = viewIndex(idx);
    if (item != -1) // is visible
    {
        //first we need to collapse all the children to start
        //with a managable state.
        collapseAllCore(idx);

        //mark the item as expanded
        storeExpanded(idx);
        viewItems_[item].expanded = true;

        //The total number items to be inserted
        int total=0;
        totalNumOfChildren(idx,total);

        //Insert the required number items
        ViewItemIterator it=viewItems_.begin();
        viewItems_.insert(it+item+1,total,TreeNodeViewItem());

        //recursively relayout the item
        layout(item,true,false,true);

        UI_ASSERT(viewItems_[item].total==total,"viewItems_[" << item << "].total=" << viewItems_[item].total <<
                  " total=" << total);

        //We need to update the parentItem in the items after the insertion.
        //When layout() is called with the given arguments it is delayed to
        //this point to gain performance!
        const int itemsCount=static_cast<int>(viewItems_.size());
        int count=viewItems_[item].total;
        for(int i = item + count+1; i < itemsCount; i++)
            if (viewItems_[i].parentItem >= item)
                viewItems_[i].parentItem += count;


        //update the scrollbars and rerender the viewport
        updateRowCount();
        updateScrollBars();
        viewport()->update();
    }
}

void AbstractNodeView::restoreExpand(const QModelIndex& idx)
{
    //expandedIndexed now contains all the indexes to expand
    expand(idx);
}

void AbstractNodeView::collapse(int item)
{
    if (item == -1 || expandedIndexes.isEmpty())
        return;

    const QModelIndex &modelIndex = viewItems_.at(item).index;
    //if(!isPersistent(modelIndex))
    //       return; // if the index is not persistent, no chances it is expanded

    QSet<QPersistentModelIndex>::iterator it = expandedIndexes.find(modelIndex);
    if (it == expandedIndexes.end() || viewItems_.at(item).expanded == false)
           return; // nothing to do

    expandedIndexes.erase(it);
    viewItems_[item].expanded = false;
    int total=viewItems_[item].total;
    int index = item;
    while (index > -1)
    {
        viewItems_[index].total-=total;
        index = viewItems_[index].parentItem;
    }
    removeViewItems(item + 1, total); // collapse
}

void AbstractNodeView::collapse(const QModelIndex &index)
{
    int i = viewIndex(index);
    if (i != -1) // is visible
    {
        collapse(i);
        updateRowCount();
        updateScrollBars();
        viewport()->update();
    }
}

bool AbstractNodeView::collapseAllCore(const QModelIndex &index)
{
    //identify item
    int item = viewIndex(index);

    //check if there is nothing to do
    if (item == -1 || expandedIndexes.isEmpty())
        return false;

    //check if the item is expanded
    QSet<QPersistentModelIndex>::iterator it = expandedIndexes.find(index);
    if (it == expandedIndexes.end() || viewItems_.at(item).expanded == false)
           return false;

    //remove all the children of the item
    viewItems_[item].expanded = false;
    int total=viewItems_[item].total;
    int parentItem = item;
    while (parentItem > -1)
    {
        viewItems_[parentItem].total-=total;
        parentItem = viewItems_[parentItem].parentItem;
    }
    removeViewItems(item + 1, total);

    //recursivel remove the indexes related to the deleted items from the expanded set
    removeAllFromExpanded(index);

    updateRowCount();
    return true;
}

void AbstractNodeView::collapseAll(const QModelIndex &index)
{
    if(collapseAllCore(index))
    {
        updateScrollBars();
        viewport()->update();
    }
}

void AbstractNodeView::removeAllFromExpanded(const QModelIndex &index)
{
    if(expandedIndexes.isEmpty())
        return;

    QSet<QPersistentModelIndex>::iterator it = expandedIndexes.find(index);
    if(it == expandedIndexes.end())
       return;

    expandedIndexes.erase(it);

    for(int i=0; i < model_->rowCount(index); i++)
    {
        QModelIndex chIdx=model_->index(i, 0, index);
        removeAllFromExpanded(chIdx);
    }
}


bool AbstractNodeView::isExpanded(const QModelIndex &index) const
{
    return isIndexExpanded(index);
}

void AbstractNodeView::setExpanded(const QModelIndex &index, bool expanded)
{
    if (expanded)
        expand(index);
    else
        collapse(index);
}

//Expands all expandable items.
void AbstractNodeView::expandAll()
{
    viewItems_.clear();
    expandedIndexes.clear();
    //d->interruptDelayedItemsLayout();
    layout(-1, true,false,false);
    updateRowCount();
    updateScrollBars();
    viewport()->update();
}

//Collapses all expanded items.
void AbstractNodeView::collapseAll()
{
    expandedIndexes.clear();
    doItemsLayout();
}

//========================================================
//
// Selection
//
//========================================================

void AbstractNodeView::setCurrentIndex(const QModelIndex &index)
{
    if(selectionModel_ && index.isValid())
    {
        QItemSelectionModel::SelectionFlags command = selectionCommand(index, 0);
        selectionModel_->setCurrentIndex(index, command);
        //currentIndexSet_ = true;
        QPoint offset;
        if((command & QItemSelectionModel::Current) == 0)
            //pressedPosition_ = visualRect(currentIndex()).center() + offset;
            pressedRefIndex_=currentIndex();
    }
}


QModelIndex AbstractNodeView::currentIndex() const
{
    return selectionModel_ ? selectionModel_->currentIndex() : QModelIndex();
}

QModelIndexList AbstractNodeView::selectedIndexes() const
{
    if(selectionModel_)
        return selectionModel_->selectedIndexes();

    return QModelIndexList();
}


/*
  Applies the selection command to the items in or touched by the
  rectangle rect.
*/
void AbstractNodeView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
    if (!selectionModel_ || rect.isNull())
        return;

#ifdef _UI_QABSTRACTNODEVIEW_DEBUG
    UiLog().dbg() << "TreeNodeViewBase::setSelection --> rect=" << rect;
#endif

    QPoint tl=QPoint(rect.x(),rect.y());
    QPoint br=QPoint(rect.x()+rect.width(),rect.y()+rect.height());

    if(tl.y() > br.y())
        qSwap(tl,br);

    QModelIndex topLeft = indexAt(tl);
    QModelIndex bottomRight = indexAt(br);

#ifdef _UI_QABSTRACTNODEVIEW_DEBUG
    UiLog().dbg() << " tl=" << tl  << " " << topLeft.data().toString() <<
                     " br=" << br  << " " << bottomRight.data().toString();
#endif

    if (!topLeft.isValid() && !bottomRight.isValid())
    {
        if(command & QItemSelectionModel::Clear)
            selectionModel_->clear();
        return;
    }

    if (!topLeft.isValid() && !viewItems_.empty())
        topLeft = viewItems_.front().index;

    if (!bottomRight.isValid() && !viewItems_.empty())
    {
        const QModelIndex index = viewItems_.back().index;
        bottomRight = index.sibling(index.row(),0);
    }

    select(topLeft, bottomRight, command);
}

void AbstractNodeView::select(const QModelIndex &topIndex, const QModelIndex &bottomIndex,
                              QItemSelectionModel::SelectionFlags command)
{
    QItemSelection selection;
    const int top = viewIndex(topIndex),
    bottom = viewIndex(bottomIndex);

#ifdef _UI_QABSTRACTNODEVIEW_DEBUG
    UiLog().dbg() << "TreeNodeViewBase::select --> command="  << command;
    UiLog().dbg() << "top=" << top << " " << topIndex.data().toString() <<
                     " bottom=" << bottom << " " << bottomIndex.data().toString();
#endif

    QModelIndex previous;
    QItemSelectionRange currentRange;
    QStack<QItemSelectionRange> rangeStack;
    for(int i = top; i <= bottom; ++i)
    {
        QModelIndex index = modelIndex(i);
        QModelIndex parent = index.parent();
        QModelIndex previousParent = previous.parent();

        //same parent as previous
        if (previous.isValid() && parent == previousParent)
        {
            //same parent
            if (qAbs(previous.row() - index.row()) > 1)
            {
                //a hole (hidden index inside a range) has been detected
                if (currentRange.isValid())
                {
                    selection.append(currentRange);
                }
                //let's start a new range
                currentRange = QItemSelectionRange(index, index);
            }

            else
            {
                QModelIndex tl = model_->index(currentRange.top(),0,
                        currentRange.parent());
                currentRange = QItemSelectionRange(tl, index);
            }
        }

        //The current parent is the previous item
        else if(previous.isValid() &&
                parent == model_->index(previous.row(), 0, previousParent))
        {
            rangeStack.push(currentRange);
            currentRange = QItemSelectionRange(index, index);
        }

        else
        {
            if(currentRange.isValid())
                selection.append(currentRange);
            if(rangeStack.isEmpty())
            {
                currentRange = QItemSelectionRange(index, index);
            }
            else
            {
                currentRange = rangeStack.pop();
                index = currentRange.bottomRight(); //let's resume the range
                --i; //we process again the current item
            }
        }

        previous = index;
    }

    if (currentRange.isValid())
        selection.append(currentRange);

    for (int i = 0; i < rangeStack.count(); ++i)
        selection.append(rangeStack.at(i));

    selectionModel_->select(selection, command);
}


/*
    Returns the SelectionFlags to be used when updating a selection with
    to include the index specified. The  event is a user input event,
    such as a mouse or keyboard event.
*/

QItemSelectionModel::SelectionFlags AbstractNodeView::selectionCommand(
    const QModelIndex &index, const QEvent *event) const
{
    Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
    if (event) {

        switch (event->type()) {
        case QEvent::MouseMove: {
            // Toggle on MouseMove
            modifiers = static_cast<const QMouseEvent*>(event)->modifiers();
            if (modifiers & Qt::ControlModifier)
                return QItemSelectionModel::ToggleCurrent|selectionBehaviorFlags();
            break;
        }
        case QEvent::MouseButtonPress: {
            modifiers = static_cast<const QMouseEvent*>(event)->modifiers();
            const Qt::MouseButton button = static_cast<const QMouseEvent*>(event)->button();
            const bool rightButtonPressed = button & Qt::RightButton;
            const bool shiftKeyPressed = modifiers & Qt::ShiftModifier;
            const bool controlKeyPressed = modifiers & Qt::ControlModifier;
            const bool indexIsSelected = selectionModel_->isSelected(index);
            if ((shiftKeyPressed || controlKeyPressed) && rightButtonPressed)
                return QItemSelectionModel::NoUpdate;
            if (!shiftKeyPressed && !controlKeyPressed && indexIsSelected)
                return QItemSelectionModel::NoUpdate;
            if (!index.isValid() && !rightButtonPressed && !shiftKeyPressed && !controlKeyPressed)
                return QItemSelectionModel::Clear;
            if (!index.isValid())
                return QItemSelectionModel::NoUpdate;
            break;
        }
        case QEvent::MouseButtonRelease: {
            // ClearAndSelect on MouseButtonRelease if MouseButtonPress on selected item or empty area
            modifiers = static_cast<const QMouseEvent*>(event)->modifiers();
            const Qt::MouseButton button = static_cast<const QMouseEvent*>(event)->button();
            const bool rightButtonPressed = button & Qt::RightButton;
            const bool shiftKeyPressed = modifiers & Qt::ShiftModifier;
            const bool controlKeyPressed = modifiers & Qt::ControlModifier;
            if (((index == pressedIndex_ && selectionModel_->isSelected(index))
                || !index.isValid()) //&& state != QAbstractItemView::DragSelectingState
                && !shiftKeyPressed && !controlKeyPressed && (!rightButtonPressed || !index.isValid()))
                return QItemSelectionModel::ClearAndSelect|selectionBehaviorFlags();
            return QItemSelectionModel::NoUpdate;
        }
        case QEvent::KeyPress: {
            // NoUpdate on Key movement and Ctrl
            modifiers = static_cast<const QKeyEvent*>(event)->modifiers();
            switch (static_cast<const QKeyEvent*>(event)->key()) {
            case Qt::Key_Backtab:
                modifiers = modifiers & ~Qt::ShiftModifier; // special case for backtab
            case Qt::Key_Down:
            case Qt::Key_Up:
            case Qt::Key_Left:
            case Qt::Key_Right:
            case Qt::Key_Home:
            case Qt::Key_End:
            case Qt::Key_PageUp:
            case Qt::Key_PageDown:
            case Qt::Key_Tab:
                if (modifiers & Qt::ControlModifier
#ifdef QT_KEYPAD_NAVIGATION
                    // Preserve historical tab order navigation behavior
                    || QApplication::navigationMode() == Qt::NavigationModeKeypadTabOrder
#endif
                    )
                    return QItemSelectionModel::NoUpdate;
                break;
            case Qt::Key_Select:
                return QItemSelectionModel::Toggle|selectionBehaviorFlags();
            case Qt::Key_Space:// Toggle on Ctrl-Qt::Key_Space, Select on Space
                if (modifiers & Qt::ControlModifier)
                    return QItemSelectionModel::Toggle|selectionBehaviorFlags();
                return QItemSelectionModel::Select|selectionBehaviorFlags();
            default:
                break;
            }
        }
        default:
            break;
        }
    }

    if (modifiers & Qt::ShiftModifier)
        return QItemSelectionModel::SelectCurrent|selectionBehaviorFlags();
    if (modifiers & Qt::ControlModifier)
        return QItemSelectionModel::Toggle|selectionBehaviorFlags();
    //if (state == QAbstractItemView::DragSelectingState) {
    //    //when drag-selecting we need to clear any previous selection and select the current one
    //    return QItemSelectionModel::Clear|QItemSelectionModel::SelectCurrent|selectionBehaviorFlags();
    //}

    return QItemSelectionModel::ClearAndSelect|selectionBehaviorFlags();
}

/*
  Returns the rectangle from the viewport of the items in the given
  selection.

  The returned region only contains rectangles intersecting
  (or included in) the viewport.
*/
QRegion AbstractNodeView::visualRegionForSelection(const QItemSelection &selection) const
{
    if(selection.isEmpty())
        return QRegion();

    QRegion selectionRegion;
    const QRect &viewportRect = viewport()->rect();
    for(int i = 0; i < selection.count(); ++i)
    {
        QItemSelectionRange range = selection.at(i);
        if (!range.isValid())
            continue;

        QModelIndex leftIndex = range.topLeft();
        if (!leftIndex.isValid())
            continue;

        QModelIndex rightIndex = range.bottomRight();
        if (!rightIndex.isValid())
            continue;

        int left=100000000,right=0,top=1000000000, bottom=0;
        Q_FOREACH(QModelIndex idx,range.indexes())
        {
            const QRect r = visualRect(idx);
            //UiLog().dbg() << r << " " << idx << " " << idx.data().toString();
            if(r.x() < left) left=r.x();
            if(r.right()+1 > right) right=r.right()+1;
            if(r.y() < top) top=r.y();
            if(r.bottom()+1 > bottom ) bottom=r.bottom()+1;
        }

        top-=1;
        bottom+=1;

        QRect combined(left,top,right-left+1,bottom-top+1);
        if (viewportRect.intersects(combined))
                selectionRegion += combined;
    }
    return selectionRegion;
}

/*
    This slot is called when the selection is changed. The previous
    selection (which may be empty), is specified by  deselected, and the
    new selection by selected.
*/
void AbstractNodeView::selectionChanged(const QItemSelection &selected,
                                   const QItemSelection &deselected)
{
    if(isVisible()) // && updatesEnabled()) {
    {
        QRegion des=visualRegionForSelection(deselected);
        QRegion sel=visualRegionForSelection(selected);

#ifdef _UI_QABSTRACTNODEVIEW_DEBUG
        UiLog().dbg() << "TreeNodeViewBase::selectionChanged -->";
        UiLog().dbg() << "  deselect=" << des.boundingRect() << " select=" << sel.boundingRect();
        QRegion un=des | sel;
        UiLog().dbg() << "  union=" << un.boundingRect();
#endif
        viewport()->update(des | sel);
    }
}

/*
    This slot is called when a new item becomes the current item.
    The previous current item is specified by the previous index, and the new
    item by the current index.
*/
void AbstractNodeView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    if(!isVisible())
        return;

    if(previous.isValid())
    {
        update(previous);
    }

    if(current.isValid())
    {
        scrollTo(current);
        update(current);
    }
}


