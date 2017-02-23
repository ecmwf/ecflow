//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "CompactView.hpp"

#include "TreeNodeModel.hpp"
#include "TreeNodeViewDelegate.hpp"
#include "UiLog.hpp"

#include <QtAlgorithms>
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QMouseEvent>
#include <QScrollBar>

#define _UI_COMPACTVIEW_DEBUG


CompactView::CompactView(TreeNodeModel* model,QWidget* parent) :
    QAbstractScrollArea(parent),
    model_(model),
    verticalScrollMode_(ScrollPerItem),
    rowCount_(0),
    lastViewedItem_(0)
{
    delegate_=new TreeNodeViewDelegate(this);

    //We attach the model because by default the view is enabled. At this point the model is empty so
    //it is a cheap operation!!
    attachModel();
}

CompactView::~CompactView()
{

}

//Connect the models signal to the view
void CompactView::attachModel()
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
        this,SLOT(slotDataChanged(const QModelIndex&,const QModelIndex&)));

    //We need to call it to be sure that the view show the actual state of the model!!!
    //doReset();
}

void CompactView::mousePressEvent(QMouseEvent* event)
{
    if(event->button() == Qt::MidButton)
    {
        int viewItemIndex=itemAtCoordinate(event->pos());
        if(viewItemIndex != -1)
        {
#ifdef _UI_COMPACTVIEW_DEBUG
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
            updateScrollBars();
            viewport()->update();
        }
    }

}

void CompactView::reset()
{
    viewItems_.clear();
    rowCount_=0;
    expandedIndexes.clear();
    layout(-1);
    updateScrollBars();
}

/*
 Informs the view that the rows from the start row to the end row
 inclusive have been inserted into the parent model item.
*/
void CompactView::rowsInserted(const QModelIndex& parent,int start,int end)
{
#if 0
    const int parentRowCount = model_->rowCount(parent);
    const int delta = end - start + 1;
    if(parent != root_ && !isIndexExpanded(parent) && parentRowCount > delta)
    {
        //QAbstractItemView::rowsInserted(parent, start, end);
        return;
    }
#endif

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
void CompactView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
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
void CompactView::rowsRemoved(const QModelIndex &parent, int start, int end)
{
    doItemsLayout(true);
}

void CompactView::doItemsLayout(bool hasRemovedItems)
{
    if(hasRemovedItems)
    {
        //clean the QSet that may contains old (and this invalid) indexes
        QSet<QPersistentModelIndex>::iterator it = expandedIndexes.begin();
        while (it != expandedIndexes.constEnd())
        {
            if (!it->isValid())
                it = expandedIndexes.erase(it);
            else
                ++it;
        }
    }

    viewItems_.clear(); // prepare for new layout
    QModelIndex parent = root_;
    if(model_->hasChildren(parent))
    {
        layout(-1);
    }
    updateScrollBars();
    viewport()->update();
}


/*
    creates and initialize the viewItem structure of the children of the element

    set recursiveExpanding if the function has to expand all the children (called from expandAll)
    afterIsUninitialized is when we recurse from layout(-1), it means all the items after 'i' are
    not yet initialized and need not to be moved
 */

void CompactView::layout(int parentId, bool recursiveExpanding,bool afterIsUninitialized)
{
    int dx=10;
    QModelIndex parentIndex = (parentId < 0) ? root_ : modelIndex(parentId);

    if(parentId >=0 && !parentIndex.isValid())
    {
           //modelIndex() should never return something invalid for the real items.
           //This can happen if columncount has been set to 0.
           //To avoid infinite loop we stop here.
        return;
    }

    CompactViewItem* parentItem=0;
    int count=model_->rowCount(parentIndex);

    //If there is a parent (e.i. non-root)
    if(parentId > -1)
    {
        parentItem=&(viewItems_[parentId]);
    }

    bool expanding=true;

    //This is the root item. viewItems must be empty at this point.
    if(parentId == -1)
    {
        Q_ASSERT(viewItems_.empty());
        viewItems_.resize(count);
        afterIsUninitialized = true;
    }
    //The count of the
    else if(parentItem->total != (uint)count)
    {
        //Expand
        if(!afterIsUninitialized)
            insertViewItems(parentId + 1, count, CompactViewItem());
        //ExpandAll
        else if(count > 0)
            viewItems_.resize(viewItems_.size() + count);
    }
    else
    {
        expanding=false;
    }

    int first = parentId + 1;
    int last = 0;
    int children = 0;
    int level=(parentItem)?parentItem->level+1:0;
    CompactViewItem *item=0;

    //Iterate through the direct children of parent item. At this point all the items
    //needed in the loop below are pre-allocated but not yet initialised.
    for(int i=first; i < first+count; i++)
    {
        QModelIndex currentIndex=model_->index(i-first,0,parentIndex);

        last = i + children;
        item = &viewItems_[last];
        item->parentItem = parentId;
        item->index=currentIndex;
        item->hasMoreSiblings=(i < first+count-1);
        item->level=level;
        item->expanded = false;
        item->total = 0;

        int w,h;
        delegate_->sizeHint(currentIndex,w,h);
        item->width=w;
        item->height=h;

        int xp=dx;
        if(parentItem)
        {
            xp=parentItem->right()+dx;
            item->x=xp;
            UiLog().dbg() <<  "right " << parentItem->right();
        }

        //We need to expand the item
        if(recursiveExpanding || isIndexExpanded(currentIndex))
        {
            if(recursiveExpanding)
                expandedIndexes.insert(currentIndex);

            item->expanded = true;

            //Add the children to the layout
            layout(last,recursiveExpanding,afterIsUninitialized);

            //item = &viewItems_[last];
            children+=item->total;
            item->hasChildren = item->total > 0;
            last = i + children;
        }
        else
        {
            item->hasChildren = model_->hasChildren(currentIndex);
        }

        //When we reach a leaf node we reache the end of a row as well.
        if(item->total == 0)
            rowCount_++;
    }

    if(!expanding)
        return; // nothing changed

    int pp=parentId;
    while (pp > -1)
    {
        viewItems_[pp].total += count;
        pp = viewItems_[pp].parentItem;
    }
}

#if 0
void CompactView::insertItems(const QModelIndex& parent,int parentPos)
{
    UiLog().dbg() << "insert --> parent=" << parent.data().toString() << " rows=" << model_->rowCount(parent);

    int dx=10;

    CompactViewItem* pt=0;
    int rowNum=model_->rowCount(parent);
    if(parentPos != -1)
    {
        pt=&(viewItems_[parentPos]);
    }

    for(int i=0; i < rowNum; i++)
    {
        QModelIndex idx=model_->index(i,0,parent);

        int xp=dx;
        if(pt)
        {
            xp=pt->right()+dx;
            UiLog().dbg() <<  "right " << pt->right();
        }

        bool leaf=!model_->hasChildren(idx);
        CompactViewItem item(idx,parentPos,xp,leaf);
        item.hasMoreSiblings=(i < rowNum-1);

        if(pt)
            item.level=pt->level+1;

        int w,h;
        delegate_->sizeHint(idx,w,h);
        item.width=w;
        item.height=h;

        if(leaf) // || i > 0)
        {
            rowCount_++;
        }

        viewItems_.push_back(item);

        insertItems(idx,viewItems_.size()-1);
    }

    int pp=parentPos;
    while (pp > -1)
    {
        viewItems_[pp].total += rowNum;
        pp = viewItems_[pp].parentItem;
    }



    //verticalScrollBar()->setRange(0, yp+20);

}
#endif

void CompactView::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    paint(&painter,event->region());
}


void CompactView::paint(QPainter *painter,const QRegion& region)
{
#ifdef _UI_COMPACTVIEW_DEBUG
    UiLog().dbg() << "CompactNodeView::paint -->";
#endif

    UiLog().dbg() << "size=" << sizeof(CompactViewItem);

    UiLog().dbg() << "region=" << region;

    int firstVisibleOffset=0;
    int firstVisible=firstVisibleItem(firstVisibleOffset);

#ifdef _UI_COMPACTVIEW_DEBUG
    UiLog().dbg() << "firstVisible " << firstVisible;
#endif

    if(firstVisible <0)
        return;


    std::vector<int> indentVec(1000,0);
    if(firstVisible >0)
    {
        CompactViewItem* item=&viewItems_[firstVisible];
        int level=item->level;
        while(item->parentItem >= 0 && level >0)
        {
            CompactViewItem* pt=&viewItems_[item->parentItem];
            if(item->hasMoreSiblings)
            {
                indentVec[item->level]=(pt->right()+2+item->x-2)/2;
            }
            Q_ASSERT(pt->level == level-1);
            item=pt;
            level--;
        }
    }

    const std::size_t itemsCount = viewItems_.size();
    const int viewportWidth = viewport()->width();
    QVector<QRect> rects = region.rects();
    QVector<int> drawn;
    bool multipleRects = (rects.size() > 1);
    for (int a = 0; a < rects.size(); ++a)
    {
        const QRect area = (multipleRects
                            ? QRect(0, rects.at(a).y(), viewportWidth, rects.at(a).height())
                            : rects.at(a));

        //d->leftAndRight = d->startAndEndColumns(area);

        std::size_t i = firstVisible; // the first item at the top of the viewport
        int y = firstVisibleOffset; // we may only see part of the first item

        //start at the top of the viewport  and iterate down to the update area
        int itemsInRow=1;
        for (; i < itemsCount; i+=itemsInRow)
        {
            const int itemHeight=rowHeight(i,1,itemsInRow);

#ifdef _UI_COMPACTVIEW_DEBUG
            UiLog().dbg() << "row: " << i << " " << itemHeight << " " << itemsInRow;
#endif

            //const int itemHeight = viewItems_[i]->height;
            if(y + itemHeight > area.top())
                break;
            y += itemHeight;
        }

#ifdef _UI_COMPACTVIEW_DEBUG
        UiLog().dbg() << "y: " << y << " " << area.bottom();
#endif

        //paint the visible rows
        for (; i < itemsCount && y <= area.bottom(); i+=itemsInRow)
        {
            drawRow(painter,i,y,itemsInRow,indentVec);
#ifdef _UI_COMPACTVIEW_DEBUG
            UiLog().dbg() << " drawRow " << i << " " << y << " " << itemsInRow;
#endif
        }
    }
}

void CompactView::drawRow(QPainter* painter,int start,int &yp,int &itemsInRow,std::vector<int>& indentVec)
{
    itemsInRow=0;
    bool leaf=false;
    const int itemsCount = static_cast<int>(viewItems_.size());

   // std::vector<int> indentVec(1000,0);
    int rh=0;
    int firstLevel=0;

#if 0
    if(start >0)
    {
        CompactViewItem* item=viewItems_[start];
        int level=item->level;
        while(item->parentItem >= 0 && level >0)
        {
            CompactViewItem* pt=viewItems_[item->parentItem];
            indentVec[item->level]=(pt->right()+2+item->x-2)/2;
            Q_ASSERT(pt->level == level-1);
            item=pt;
            level--;
        }
    }
#endif

    for(int i=start; i < itemsCount && !leaf; ++i )
    {
        CompactViewItem* item=&(viewItems_[i]);
        //if(item->expanded == 1)
        {
            QStyleOptionViewItem option;
            option.rect=QRect(item->x,yp,item->width+10,item->height);
            delegate_->paint(painter,option,item->index);

            leaf=(item->total == 0);

            //UiLog().dbg() << i << " " << viewItems_[i]->index << " " << viewItems_[i]->index.data().toString() << " "
            //              << viewItems_[i]->x << " " << viewItems_[i]->height << " " << leaf;

            //Get the rowheight. There are three kinds of row heights.
            // 1. nodes (fixed height)
            // 2. attributes (fixed height)
            // 3. multiline label attributes (variable height!!!)
            if(rh == 0)
            {
                int iir=0;
                rh=rowHeight(i,1,iir);
            }

            //Finds out the first indentation level in the row
            if(firstLevel==0)
                firstLevel=item->level;

            if(item->parentItem >=0)
            {
                //The parent item. It is always a node.
                CompactViewItem* pt=&(viewItems_[item->parentItem]);

                //The horizontal line connecting the item to its parent
                int lineY=yp+pt->height/2;
                int lineX1=pt->right()+2;
                int lineX2=item->x-2;
                int lineX=(lineX1+lineX2)/2;

                //First child - in the same row as its parent
                if(item->index.row() == 0)
                {
                    //Line to the parent
                    painter->drawLine(lineX1,lineY,lineX2,lineY);

                    //line towards the siblings  - downwards
                    if(item->hasMoreSiblings)
                    {
                        painter->drawLine(lineX,lineY,lineX,lineY+rh/2);
                    }

                    indentVec[item->level]=lineX;
                }

                //Child in the middle - has sibling both upwards and downwards
                else if(item->hasMoreSiblings)
                {
                    painter->drawLine(lineX,lineY,lineX2,lineY);
                    painter->drawLine(lineX,lineY+rh/2,lineX,lineY-rh/2);
                    indentVec[item->level]=lineX;
                }

                //The last child - has sibling only upwards
                else
                {
                    painter->drawLine(lineX,lineY,lineX2,lineY);
                    painter->drawLine(lineX,lineY,lineX,lineY-rh/2);

                    indentVec[item->level]=0;
                }

            }

            //When we reach a leaf item we move one row down.
            if(leaf)
            {
                //Draw the vertical connector lines for all the levels
                //preceding the first level in the row where it is needed!
                int level=item->level;
                for(size_t j=0; j < firstLevel; j++)
                {
                    int xp=indentVec[j];
                    if(xp != 0)
                        painter->drawLine(xp,yp,xp,yp+rh);
                }

                yp+=rh; //+5;

                rh=0;
                firstLevel=0;
            }

            itemsInRow++;
       }
    }

    if(itemsInRow == 0)
       itemsInRow=1;
}


//Updates the area occupied by the given index.
void CompactView::update(const QModelIndex &index)
{
    if (index.isValid())
    {
        const QRect rect = visualRect(index);
        //this test is important for peformance reason
        //For example in dataChanged if we simply update all the cells without checking
        //it can be a major bottleneck to update rects that aren't even part of the viewport
        if(viewport()->rect().intersects(rect))
            viewport()->update(rect);
    }
}

int CompactView::rowHeight(int start,int forward, int &itemsInRow) const
{
    uint rh=0;
    itemsInRow=0;
    const int itemsCount = static_cast<int>(viewItems_.size());

    if(forward == 1)
    {
        for(int i=start; i < itemsCount; i++)
        {
           rh=qMax(rh,viewItems_[i].height);
           itemsInRow++;
           if(viewItems_[i].total == 0)
                break;
        }
    }
    else
    {
        rh=qMax(rh,viewItems_[start].height);
        itemsInRow++;
        for(int i=start-1; i >= 0; i--)
        {
           if(viewItems_[i].total == 0)
                break;
           rh=qMax(rh,viewItems_[i].height);
           itemsInRow++;
        }
    }

    Q_ASSERT(itemsInRow > 0);
    return rh;
}

int CompactView::itemCountInRow(int start) const
{
    const std::size_t itemsCount = viewItems_.size();
    int itemsInRow=0;
    for(int i=start; i < itemsCount; i++)
    {
        itemsInRow++;
        if(!model_->hasChildren(viewItems_[i].index))
            return itemsInRow;
    }

    Q_ASSERT(itemsInRow > 0);
    return itemsInRow;
}


int CompactView::firstVisibleItem(int &offset) const
{
    const int value = verticalScrollBar()->value();
#ifdef _UI_COMPACTVIEW_DEBUG
    UiLog().dbg() << "CompactNodeView::firstVisibleItem --> value=" << value;
#endif

    if (verticalScrollMode_ == ScrollPerItem)
    {
        offset = 0;
        //value is the row number

        if(value <0 || value >= rowCount_)
            return -1;

        int cnt=0;
        int itemsInRow=0;
        const std::size_t itemsCount=viewItems_.size();
        for (std::size_t i=0; i < itemsCount; i+=itemsInRow)
        {
            if(cnt == value)
            {
#ifdef _UI_COMPACTVIEW_DEBUG
    UiLog().dbg() << " i=" << i << " itemsInRow=" << itemsInRow;
#endif
                return i;
            }
            itemsInRow=itemCountInRow(i);
            cnt++;
        }
        //return (value < 0 || value >= viewItems_.count()) ? -1 : value;
    }

#if 0
    int y = 0;
    for(int i = 0; i < viewItems_.count(); ++i)
    {
        y += itemHeight(i); // the height value is cached
        if (y > value) {
            if (offset)
                *offset = y - value - itemHeight(i);
            return i;
        }
    }
#endif
    return -1;
}

void CompactView::updateScrollBars()
{
#ifdef _UI_COMPACTVIEW_DEBUG
    UiLog().dbg() << "CompactNodeView::updateScrollBars -->";
#endif

    QSize viewportSize = viewport()->size();
    if(!viewportSize.isValid())
        viewportSize = QSize(0, 0);

    if(viewItems_.empty())
    {
        //doItemsLayout();
    }

    int itemsInViewport = 0;

    const std::size_t itemsCount = viewItems_.size();
    const int viewportHeight = viewportSize.height();
    int itemsInRow=1;
    for(std::size_t height = 0, item = itemsCount - 1; item >= 0; item-=itemsInRow)
    {
        height +=rowHeight(item,-1,itemsInRow);
        if(height > viewportHeight)
            break;
        itemsInViewport++;
    }
#ifdef _UI_COMPACTVIEW_DEBUG
    UiLog().dbg() << "  itemsCount=" << itemsCount << " rowCount=" << rowCount_;
    UiLog().dbg() << "  itemsInViewport " << itemsInViewport;
#endif

    if(verticalScrollMode_ == ScrollPerItem)
    {
        if(!viewItems_.empty())
            itemsInViewport = qMax(1, itemsInViewport);

        //verticalScrollBar()->setRange(0, itemsCount - itemsInViewport);
        verticalScrollBar()->setRange(0, rowCount_ - itemsInViewport);
        verticalScrollBar()->setPageStep(itemsInViewport);
        verticalScrollBar()->setSingleStep(1);
    }
    else
    {
#if 0
        // scroll per pixel
        int contentsHeight = 0;
        if (uniformRowHeights) {
            contentsHeight = defaultItemHeight * viewItems.count();
        } else { // ### optimize (spans or caching)
            for (int i = 0; i < viewItems.count(); ++i)
                contentsHeight += itemHeight(i);
        }
        vbar->setRange(0, contentsHeight - viewportSize.height());
        vbar->setPageStep(viewportSize.height());
        vbar->setSingleStep(qMax(viewportSize.height() / (itemsInViewport + 1), 2));
#endif
    }
}







/*
  Returns the rectangle on the viewport occupied by the item at \a index.
  If the index is not visible or explicitly hidden, the returned rectangle is invalid.
*/
QRect CompactView::visualRect(const QModelIndex &index) const
{
    //if (!d->isIndexValid(index) || isIndexHidden(index))
    //    return QRect();

    //d->executePostedLayout();

    int vi = viewIndex(index);
    if (vi < 0)
        return QRect();

    int y = coordinateForItem(vi);
    if(y >=0)
    {
        return QRect(viewItems_[vi].x, y, viewItems_[vi].width, viewItems_[vi].height);
    }
    return QRect();
}

QModelIndex CompactView::indexAt(const QPoint &point) const
{
    int item=itemAtCoordinate(point);
    return (item>=0)?viewItems_[item].index:QModelIndex();
}

/*!
  Returns the viewport y coordinate for  item.
*/
int CompactView::coordinateForItem(int item) const
{
    if(verticalScrollMode_ == ScrollPerItem)
    {
        int offset = 0;
        int topViewItemIndex=firstVisibleItem(offset);
        if (item >= topViewItemIndex)
        {
            // search in the visible area first and continue down
            // ### slow if the item is not visible

            const int itemsCount = viewItems_.size();
            const int viewportHeight = viewport()->size().height();
            int itemsInRow=1;

            for(int height = 0, viewItemIndex = topViewItemIndex;
                height <= viewportHeight && viewItemIndex < itemsCount; viewItemIndex+=itemsInRow)
            {
                if(viewItemIndex == item)
                    return height;
                height +=rowHeight(item,1,itemsInRow);
            }
        }
    }
#if 0
    else if(verticalScrollMode_ == ScrollPerPixel)
    {
        // ### optimize (spans or caching)
        int y = 0;
        const int itemsSize= viewItems_.size();
        for(int i = 0; i < itemsSize; ++i)
        {
            if (i == item)
                return y - verticalScrollBar()->value();
            y += itemHeight(i);
        }
    }
#endif

    return -1;
}

int CompactView::itemAtCoordinate(const QPoint& coordinate) const
{
    const std::size_t itemCount = viewItems_.size();
    if(itemCount == 0)
        return -1;

    if(verticalScrollMode_ == ScrollPerItem)
    {
        //int topRow = verticalScrollBar()->value();

        int offset = 0;
        int topViewItemIndex=firstVisibleItem(offset);

        if(coordinate.y() >= 0)
        {
            // the coordinate is in or below the viewport
            int viewItemCoordinate = 0;
            int itemsInRow=0;
            for(std::size_t viewItemIndex = topViewItemIndex; viewItemIndex < itemCount; viewItemIndex+=itemsInRow)
            {
                viewItemCoordinate += rowHeight(viewItemIndex,1,itemsInRow);
                if (viewItemCoordinate > coordinate.y())
                {
                    viewItemIndex=itemAtRowCoordinate(viewItemIndex,itemsInRow,coordinate.x());
                    return (viewItemIndex >= itemCount ? -1 : viewItemIndex);
                }
            }
        }
#if 0
        else
        {
            // the coordinate is above the viewport
            int viewItemCoordinate = 0;
            int itemsInRow=0;
            for (int viewItemIndex = topViewItemIndex; viewItemIndex >= 0; viewItemIndex-=itemsInRow)
            {
                if (viewItemCoordinate <= coordinate.y())
                {
                    viewItemIndex=itemAtRowCoordinate(viewItemIndex,itemsInRow,coordinate.x());
                    return (viewItemIndex >= itemCount ? -1 : viewItemIndex);
                }
                viewItemCoordinate -= rowHeight(viewItemIndex,1,itemsInRow);
            }
        }
#endif

    }

    return -1;
}

int CompactView::itemAtRowCoordinate(int start,int count,int xPos) const
{
    for(int i=start; i < start+count; i++)
    {
        if(viewItems_[i].x <= xPos && viewItems_[i].right() >= xPos)
            return i;
    }
    return -1;
}

QModelIndex CompactView::modelIndex(int i) const
{
    if(i < 0 || i >= viewItems_.size())
        return QModelIndex();

    return viewItems_[i].index;
}

//Returns the index of the view item representing the given index
int CompactView::viewIndex(const QModelIndex& index) const
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

void CompactView::insertViewItems(int pos, int count, const CompactViewItem &viewItem)
{
    ViewItemIterator it=viewItems_.begin();
    viewItems_.insert(it+pos,count,viewItem);

    //We need to update the parentItem in the items after the insertion
    const int itemsCount=static_cast<int>(viewItems_.size());
    for(int i = pos + count; i < itemsCount; i++)
        if (viewItems_[i].parentItem >= pos)
            viewItems_[i].parentItem += count;
}


void CompactView::removeViewItems(int pos, int count)
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

void CompactView::expand(int item)
{
    if (item == -1 || viewItems_[item].expanded)
        return;

    //q->setState(QAbstractItemView::ExpandingState);
    const QModelIndex index = viewItems_[item].index;
    storeExpanded(index);
    viewItems_[item].expanded = true;
    layout(item);
    //q->setState(stateBeforeAnimation);
}

void CompactView::collapse(int item)
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

void CompactView::expand(const QModelIndex &index)
{
    int i = viewIndex(index);
    if (i != -1) // is visible
    {
        expand(i);
        updateScrollBars();
        viewport()->update();
    }
}

void CompactView::collapse(const QModelIndex &index)
{
    int i = viewIndex(index);
    if (i != -1) // is visible
    {
        collapse(i);
        updateScrollBars();
        viewport()->update();
    }
}

bool CompactView::isExpanded(const QModelIndex &index) const
{
    return isIndexExpanded(index);
}

void CompactView::setExpanded(const QModelIndex &index, bool expanded)
{
    if (expanded)
        expand(index);
    else
        collapse(index);
}

//Expands all expandable items.
void CompactView::expandAll()
{
    viewItems_.clear();
    expandedIndexes.clear();
    //d->interruptDelayedItemsLayout();
    layout(-1, true);
    updateScrollBars();
    viewport()->update();
}

//Collapses all expanded items.
void CompactView::collapseAll()
{
    expandedIndexes.clear();
    doItemsLayout();
}
