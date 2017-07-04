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

#include "ExpandState.hpp"
#include "TreeNodeModel.hpp"
#include "TreeNodeViewDelegate.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"

#include <QtAlgorithms>
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QMouseEvent>
#include <QScrollBar>
#include <QStyledItemDelegate>
#include <QTimerEvent>

//#define _UI_COMPACTVIEW_DEBUG

CompactView::CompactView(TreeNodeModel* model,QWidget* parent) :
    AbstractNodeView(model,parent)
{
    //This is needed for making the context menu work
    setProperty("view","tree");

    //we cannot call it from the constructor of the base class
    //because it calls a pure virtual method
    reset();
}

CompactView::~CompactView()
{

}

//Creates and initialize the viewItem structure of the children of the element
// parentId: the items whose children are to be expanded
// recursiveExpanding: all the children will be expanded
// afterIsUninitialized: when we recurse from layout(-1) it indicates
// the items after 'i' are not yet initialized and need not to be moved

void CompactView::layout(int parentId, bool recursiveExpanding,bool afterIsUninitialized,bool preAllocated)
{
    //This is the root item.
    if(parentId == -1)
    {
        rowCount_=0;
        maxRowWidth_=0;
    }

    QModelIndex parentIndex = (parentId < 0) ? root_ : modelIndex(parentId);

    if(parentId >=0 && !parentIndex.isValid())
    {
        //modelIndex() should never return something invalid for the real items.
        //This can happen if columncount has been set to 0.
        //To avoid infinite loop we stop here.
        return;
    }

    int count=model_->rowCount(parentIndex);
    bool expanding=true;

    //This is the root item. viewItems must be empty at this point.
    if(parentId == -1)
    {
        Q_ASSERT(viewItems_.empty());
        Q_ASSERT(preAllocated == false);
        viewItems_.resize(count);
        afterIsUninitialized = true; //It can only be true when we expand from the root!
    }
    //The count of the stored children does not match the actual count
    else if(viewItems_[parentId].total != (uint)count)
    {
        //Expand
        if(!afterIsUninitialized)
        {
            //We called expandall for a non-root item. All the new items need must be
            //already instered at this point. This is the duty of the caller routine.
            //const int itemsCount = viewItems_.size();
            //if(recursiveExpanding)
            if(preAllocated)
            {
                //We called expandAll() for a non-root item. All the needed items need must already be
                //inserted at this point. This is the duty of the caller routine!
                //When layout() is finished we need to adjust the parent of all the items
                //after the insertion position. This is the duty of the caller routine. We
                //have chosen this solution for performance reasons!
            }
            else
            {
                insertViewItems(parentId + 1, count, TreeNodeViewItem());
            }
        }
        //ExpandAll from the root
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
    int level=(parentId >=0?viewItems_[parentId].level+1:0);
    TreeNodeViewItem *item=0;

    std::vector<int> itemWidthVec;
    std::vector<int> itemHeightVec;
    int widest=0;
    for(int i=first; i < first+count; i++)
    {
        int w,h;
        QModelIndex currentIndex=model_->index(i-first,0,parentIndex);
        delegate_->sizeHint(currentIndex,w,h);
        itemWidthVec.push_back(w);
        itemHeightVec.push_back(h);

        if(parentId >=0 && !model_->isAttribute(currentIndex))
            if(w > widest) widest=w;
#ifdef _UI_COMPACTVIEW_DEBUG
        UiLog().dbg() << "  item=" << currentIndex.data().toString() << " w=" << w;
#endif
    }

#ifdef _UI_COMPACTVIEW_DEBUG
    if(parentId >=0)
        UiLog().dbg() << "layout parent=" << viewItems_[parentId].index.data().toString() <<
                         " widest child=" << widest;
#endif

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
        item->widestInSiblings=widest;

        //We compute the size of the item. For attributes we delay the width computation until we
        //actually paint them and we set their width to 300.
        item->width=itemWidthVec[i-first];
        item->height=itemHeightVec[i-first];

        int xp=leftMargin_;
        if(parentId >=0)
        {
            item->widestInSiblings=widest;
            xp=viewItems_[parentId].alignedRight()+itemGap_;
        }
        else
        {
            item->widestInSiblings=item->width;
        }

        item->x=xp;

        if(item->alignedRight() > maxRowWidth_)
            maxRowWidth_=item->alignedRight();

        //We need to expand the item
        if(recursiveExpanding || isIndexExpanded(currentIndex))
        {
            if(recursiveExpanding)
                expandedIndexes.insert(currentIndex);

            item->expanded = true;

#ifdef _UI_COMPACTVIEW_DEBUG
            UiLog().dbg() <<  "  before " <<  item->index.data().toString() <<  " total=" << item->total;
#endif
            //Add the children to the layout
            layout(last,recursiveExpanding,afterIsUninitialized,preAllocated);

            item = &viewItems_[last];

#ifdef _UI_COMPACTVIEW_DEBUG
            UiLog().dbg() <<  "  after " <<  item->index.data().toString() <<  " total=" << item->total;
#endif
            children+=item->total;
            item->hasChildren = item->total > 0;
        }
        else
        {
            item->hasChildren = model_->hasChildren(currentIndex);
        }
    }

    if(!expanding)
        return; // nothing changed

#ifdef _UI_COMPACTVIEW_DEBUG
    UiLog().dbg() << " update parent total";
#endif

    int pp=parentId;
    while (pp > -1)
    {
        viewItems_[pp].total += count;

#ifdef _UI_COMPACTVIEW_DEBUG
        UiLog().dbg() <<  "  parent=" << viewItems_[pp].index.data().toString() <<
                          "  total=" << viewItems_[pp].total;
#endif

        pp = viewItems_[pp].parentItem;
    }
}

//Paint the rows intersecting with the given region
void CompactView::paint(QPainter *painter,const QRegion& region)
{
    //Even though the viewport palette is set correctly at the
    //beginning something sets it to another value. Here we try
    //to detect it and correct the palette with the right colour.
    if(expectedBg_.isValid())
    {
        QPalette p=viewport()->palette();
        if(p.color(QPalette::Window) != expectedBg_)
        {
            p.setColor(QPalette::Window,expectedBg_);
            viewport()->setPalette(p);
            viewport()->update();
            expectedBg_=QColor();
            return;
        }
    }

#ifdef _UI_COMPACTVIEW_DEBUG
    UiLog().dbg() << "CompactView::paint -->";
    //UiLog().dbg() << "sizeof(TreeNodeViewItem)=" << sizeof(TreeNodeViewItem);
    //UiLog().dbg() << "region=" << region;
#endif

    int firstVisibleOffset=0;

    //The first visible item at the top of the viewport
    int firstVisible=firstVisibleItem(firstVisibleOffset);
#ifdef _UI_COMPACTVIEW_DEBUG
    UiLog().dbg() << "firstVisible " << firstVisible;
#endif

    if(firstVisible<0)
        return;

#ifdef _UI_COMPACTVIEW_DEBUG
    UiLog().dbg() << "scrollX" << horizontalScrollBar()->value() << " " << viewport()->width();
#endif

    int xOffset=0;
    if(horizontalScrollBar()->value() > 0)
    {
        xOffset=horizontalScrollBar()->value();
        painter->translate(-xOffset,0);
    }

    const int itemsCount = viewItems_.size();
    const int viewportWidth = viewport()->width();
    QVector<QRect> rects = region.rects();
    QVector<int> drawn;
    bool multipleRects = (rects.size() > 1);

    //Iterate through the rectangles in the region
    for(int a = 0; a < rects.size(); ++a)
    {
        const QRect area = (multipleRects
                            ? QRect(0, rects.at(a).y(), viewportWidth, rects.at(a).height())
                            : rects.at(a));
#ifdef _UI_COMPACTVIEW_DEBUG
        UiLog().dbg() << " area=" << area;
#endif

        //Initialise indentVec. For each indentation level it tells us if
        //a connector line is to be drawn. Here we scan up to the
        //toplevel item in the firstVisible item's branch.
        std::vector<int> indentVec(1000,0);
        if(firstVisible >0)
        {
            TreeNodeViewItem* item=&viewItems_[firstVisible];
            int level=item->level;
            while(item->parentItem >= 0 && level >0)
            {
                TreeNodeViewItem* pt=&viewItems_[item->parentItem];
                if(item->hasMoreSiblings)
                {
                    indentVec[item->level]=connectorPos(item,pt);
                }
                UI_ASSERT(pt->level == level-1, "item->parentItem=" << item->parentItem <<
                          " pt->level=" << pt->level << " level=" << level);
                item=pt;
                level--;
            }
        }

        int i = firstVisible; // the first item at the top of the viewport
        int y = firstVisibleOffset; // we may only see part of the first item

        //start at the top of the viewport  and iterate down through the update area
        int itemsInRow=1;
        for (; i < itemsCount; i+=itemsInRow)
        {
            int itemHeight;
            rowProperties(i,itemHeight,itemsInRow,indentVec);

#ifdef _UI_COMPACTVIEW_DEBUG
            UiLog().dbg() << "row: " << i << " " << itemHeight << " " << itemsInRow;
#endif
            //Try to find the first item int the current rect
            if(y + itemHeight > area.top())
                break;
            y += itemHeight;
        }

#ifdef _UI_COMPACTVIEW_DEBUG
        UiLog().dbg() << "y: " << y << " " << area.bottom();
#endif

        //Paint the visible rows in the current rect
        for (; i < itemsCount && y <= area.bottom(); i+=itemsInRow)
        {                     
            if(!multipleRects || !drawn.contains(i))
            {
                //Draw a whole row. It will update y,itemsInRow and indentVec!!
                drawRow(painter,i,xOffset,y,itemsInRow,indentVec);

#ifdef _UI_COMPACTVIEW_DEBUG
                UiLog().dbg() << " row rendered - item=" << i << " y=" << y << " itemsInRow=" << itemsInRow;
#endif
            }
            else
            {
                int rh=rowHeight(i,1,itemsInRow);
                y+=rh;
#ifdef _UI_COMPACTVIEW_DEBUG
                UiLog().dbg() << " row skipped  - item=" << i << " y=" << y << " itemsInRow=" << itemsInRow;
#endif
            }

            if(multipleRects)
               drawn.append(i);
        }
    }
}

//Draw a whole row starting at item "start".
void CompactView::drawRow(QPainter* painter,int start,int xOffset,int& yp,int& itemsInRow,std::vector<int>& indentVec)
{
    itemsInRow=0;
    bool leaf=false;
    const int itemsCount = static_cast<int>(viewItems_.size());

    //Get the rowheight
    int iir=0;
    int rh=rowHeight(start,1,iir);

    //See if there are no multiline items in this row
    bool singleRow=delegate_->isSingleHeight(rh);

    int firstLevel=0;
    const int viewportWidth = viewport()->width();

    //We iterate through the items in the row
    for(int i=start; i < itemsCount && !leaf; ++i )
    {
        TreeNodeViewItem* item=&(viewItems_[i]);
#ifdef _UI_COMPACTVIEW_DEBUG
        UiLog().dbg() << "  item=" << i << " " << item->index.data().toString();
#endif
        leaf=(item->total == 0);

        //Find out the first indentation level in the row
        if(firstLevel==0)
            firstLevel=item->level;

        //Init style option
        QStyleOptionViewItem opt;
        if(selectionModel_->isSelected(item->index))
            opt.state |= QStyle::State_Selected;

        int optWidth=2000;
        if(item->width > optWidth)
            optWidth=item->width;
        opt.rect=QRect(item->x,yp,optWidth,item->height);

        //We do not render the item if it is outisde the viewport and
        //its parent's right is also outside the viewport. Here we considered that
        //the connector line is always drawn from the child to the parent.
        bool needToDraw=true;
        if(item->parentItem >=0)
        {
            if(viewItems_[item->parentItem].right() >= translation() + viewportWidth)
                needToDraw=false;
        }

        if(needToDraw)
        {
            //For single rows we center items halfway through the rowHeight
            if(singleRow)
            {
                if(item->height < rh)
                {
                    opt.rect.moveTop(yp+(rh-item->height)/2);
                }
            }

            //QRect vr=visualRect(item->index);
            //painter->fillRect(vr,QColor(120,120,120,120));

//#ifdef _UI_COMPACTVIEW_DEBUG
//          UiLog().dbg() << "  optRect=" << opt.rect << " visRect=" << vr;
//#endif

            //Draw the item with the delegate
            QSize paintedSize;
            delegate_->paint(painter,opt,item->index,paintedSize);

            //we have to know if the item width/height is the same that we expected.
            //This can happen when:
            // -we set a fixed initial width for the item (e.g. for an attribute)
            //  and now we got the real width
            // -the number of icons or additional extra information
            //  changed for a node (so the width changed)
            // -the number of lines changed in a multiline label (so the height changed)
            bool wChanged=paintedSize.width() != item->width;
            bool hChanged=paintedSize.height() != item->height;

            if(wChanged || hChanged)
            {
                //set new size
                item->width=paintedSize.width();
                item->height=paintedSize.height();

                if(item->right() > maxRowWidth_)
                {
                    maxRowWidth_=item->right();
                    doDelayedWidthAdjustment();
                }
                else if(hChanged)
                {
                    doDelayedWidthAdjustment();
                }
            }

            //The width changed

            if(wChanged)
            {
                bool sameAsWidest=(item->width == item->widestInSiblings);
                item->width=paintedSize.width();

                //servers
                if(item->parentItem ==-1)
                {
                    adjustWidthInParent(i);
                    doDelayedWidthAdjustment();
                }
                //Nodes
                else if(model_->isNode(item->index))
                {
                    //widestInSiblings has to be adjusted
                    if(sameAsWidest || paintedSize.width()  > item->widestInSiblings)
                    {
                        adjustWidthInParent(i);
                        doDelayedWidthAdjustment();
                    }
                    //we just need to update the item
                    else if( paintedSize.width() < item->widestInSiblings)
                    {
                        doDelayedWidthAdjustment();
                    }
                }
                //Attributes
                else
                {
                    if(item->right() > maxRowWidth_)
                    {
                        maxRowWidth_=item->right();
                        doDelayedWidthAdjustment();
                    }
                }
            }
            //the height changed (can only be a multiline label)
            if(hChanged)
            {
                //set new size
                item->height=paintedSize.height();
                doDelayedWidthAdjustment();
            }

            //QRect rr=opt.rect;
            //rr.setWidth(item->width);
            //painter->drawRect(rr);

            //UiLog().dbg() << i << " " << viewItems_[i]->index << " " << viewItems_[i]->index.data().toString() << " "
            //              << viewItems_[i]->x << " " << viewItems_[i]->height << " " << leaf;

            painter->setPen(connectorColour_);

            //If not a top level item (e.i. not a server)
            if(item->parentItem >=0)
            {
                //The parent item. It is always a node.
                TreeNodeViewItem* pt=&(viewItems_[item->parentItem]);

                //The horizontal line connecting the item to its parent
                int lineX1=pt->right()+connectorGap_;
                int lineX2=item->x-connectorGap_;
                int lineX=(pt->right()+item->x)/2;

#ifdef _UI_COMPACTVIEW_DEBUG
                UiLog().dbg() << "  lineX=" << lineX << " " << item->x << " " << connectorPos(item,pt);
#endif
                UI_ASSERT(lineX==connectorPos(item,pt),"lineX=" << lineX << " i=" << i <<
                          " item->x=" << item->x << " connectorPos=" << connectorPos(item,pt));

                //First child - in the same row as its parent
                if(item->index.row() == 0)
                {
                    int lineY=yp+pt->height/2;

                    //horizontal line to the parent
                    painter->drawLine(lineX1,lineY,lineX2,lineY);

                    //line towards the siblings  - downwards
                    if(item->hasMoreSiblings)
                    {
                        //painter->drawLine(lineX,lineY,lineX,lineY+rh/2);
                        painter->drawLine(lineX,lineY,lineX,yp+rh);
                        indentVec[item->level]=lineX;
                    }
                    else
                        indentVec[item->level]=0;
                }
                //Child in the middle - has sibling both upwards and downwards
                else if(item->hasMoreSiblings)
                {
                    int lineY=yp+item->height/2;

                    painter->drawLine(lineX,lineY,lineX2,lineY);
                    //painter->drawLine(lineX,lineY+rh/2,lineX,lineY-rh/2);
                    painter->drawLine(lineX,yp,lineX,yp+rh);
                    indentVec[item->level]=lineX;
                }

                //The last child - has sibling only upwards
                else
                {
                    int lineY=yp+item->height/2;
                    painter->drawLine(lineX,lineY,lineX2,lineY);
                    //painter->drawLine(lineX,lineY,lineX,lineY-rh/2);
                    painter->drawLine(lineX,lineY,lineX,yp);
                    indentVec[item->level]=0;
                }
            }

            //indicate if a node is exandable
            if(item->hasChildren && !item->expanded)
            {
                int lineY=yp+item->height/2;
                int lineX=item->right()+connectorGap_;
                QPen oriPen=painter->pen();
                painter->setPen(QPen(connectorColour_,1,Qt::DashLine));
                painter->drawLine(lineX,lineY,lineX+expandConnectorLenght_,lineY);
                painter->setPen(oriPen);
            }
        }

        //When we reach a leaf item we move one row down.
        if(leaf)
        {            
            //Draw the vertical connector lines for all the levels
            //preceding the first level in the row!           
            painter->setPen(connectorColour_);
            for(size_t j=0; j < firstLevel; j++)
            {
                int xp=indentVec[j];
                if(xp != 0)    
                    painter->drawLine(xp,yp,xp,yp+rh);
            }

            yp+=rh;
            rh=0;
            firstLevel=0;
        }
        itemsInRow++;
    }

    if(itemsInRow == 0)
       itemsInRow=1;
}

void CompactView::adjustWidthInParent(int start)
{
    //The parent index of the start item
    int parentItem=viewItems_[start].parentItem;

    //The current max width in the start item's siblings
    int prevWidest=viewItems_[start].widestInSiblings;

    //If the parent is not the root ie the start item is not a server
    if(parentItem >=0)
    {
        int w=0, h=0, widest=0;
        QModelIndex parentIndex=viewItems_[parentItem].index;

        //Determine the max width in the siblings of the start
        //item, ie in the children of the parent item
        int rowCount=model_->rowCount(parentIndex);
        for(int i=0; i < rowCount; i++)
        {
            QModelIndex idx=model_->index(i,0,parentIndex);
            if(model_->isNode(idx))
            {
                delegate_->sizeHint(idx,w,h);
                if(w >widest) widest=w;
            }
        }

        //If there is a new max width we need to adjust all the children of
        //the parent item
        int delta=widest-prevWidest;
        if(delta != 0)
        {
            int n=parentItem+viewItems_[parentItem].total;
            for(int i=parentItem+1; i <= n; i++)
            {
                //For a direct child of the parent item we just
                //set the max width to its new value
                if(viewItems_[i].parentItem == parentItem)
                {
                    viewItems_[i].widestInSiblings = widest;
                }
                //The other items are shifted
                else
                {
                    viewItems_[i].x+=delta;
                }

                //Check if the total width changed
                if(viewItems_[i].right() > maxRowWidth_)
                    maxRowWidth_=viewItems_[i].right();
            }
        }
    }

    //If the parent is the root ie the start item is a server
    else
    {
        //Determine the diff between the current and the previous width
        int delta=viewItems_[start].width-prevWidest;

        //for server widestInSiblings is set to the width
        viewItems_[start].widestInSiblings=viewItems_[start].width;

        //Shift all the children with the diff
        if(delta != 0)
        {
            int n=start+viewItems_[start].total;
            for(int i=start+1; i <= n; i++)
            {
                //shifted
                viewItems_[i].x+=delta;

                //Check if the total width changed
                if(viewItems_[i].right() > maxRowWidth_)
                    maxRowWidth_=viewItems_[i].right();
            }
        }
    }

}

int CompactView::connectorPos(TreeNodeViewItem* item, TreeNodeViewItem* parent) const
{
    return (parent->right()+item->x)/2;
}


//Get the rowheight. There are three kinds of row heights.
// 1. nodes (fixed height)
// 2. attributes (fixed height)
// 3. multiline label attributes (variable height!!!)
void CompactView::rowProperties(int start,int& rowHeight,int &itemsInRow,std::vector<int>& indentVec) const
{
    rowHeight=0;
    itemsInRow=0;
    const int itemsCount = static_cast<int>(viewItems_.size());

    for(int i=start; i < itemsCount; i++)
    {
        TreeNodeViewItem* item=&(viewItems_[i]);
        rowHeight=qMax(rowHeight,static_cast<int>(item->height));
        itemsInRow++;
        if(item->total == 0)
        {
            indentVec[item->level]=0;
            break;
        }

        if(item->parentItem >=0)
        {
            //The parent item. It is always a node.
            TreeNodeViewItem* pt=&(viewItems_[item->parentItem]);

            if(item->hasMoreSiblings)
            {
                int lineX1=pt->right()+2;
                int lineX2=item->x-2;
                int lineX=(lineX1+lineX2)/2;
                indentVec[item->level]=lineX;
            }
            else
            {
                indentVec[item->level]=0;
            }
        }
    }

    UI_ASSERT(itemsInRow > 0,"itemsInRow=" << itemsInRow);
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
        UI_ASSERT(start >= 0,"start=" << start << " total=" << viewItems_.size());
        UI_ASSERT(start < viewItems_.size(),"start=" << start << " total=" << viewItems_.size());
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

    UI_ASSERT(itemsInRow > 0,"itemsInRow=" << itemsInRow);
    return rh;
}

int CompactView::itemCountInRow(int start) const
{
    const std::size_t itemsCount = viewItems_.size();
    int itemsInRow=0;
    for(int i=start; i < itemsCount; i++)
    {
        itemsInRow++;
        if(viewItems_[i].total == 0)
            return itemsInRow;
    }

    UI_ASSERT(itemsInRow > 0,"itemsInRow=" << itemsInRow);
    return itemsInRow;
}

int CompactView::itemRow(int item) const
{
    if(item < 0 || item >= viewItems_.size())
       return -1;

    int row=-1;
    int itemsInRow=0;
    for(int i=0; i <= item; i+=itemsInRow)
    {
        row++;
        itemsInRow=itemCountInRow(i);
    }

    return row;
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

    return -1;
}


//This has to be very quick. Called after each collapse/expand.
void CompactView::updateRowCount()
{
    rowCount_=0;
    const int itemsCount = static_cast<int>(viewItems_.size());
    for(int i=0; i < itemsCount; i++)
    {
        if(viewItems_[i].total == 0)
            rowCount_++;
    }

#ifdef _UI_COMPACTVIEW_DEBUG
    UiLog().dbg() << "CompactNodeView::updateRowCount --> " << rowCount_;
#endif
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

    const int itemsCount = viewItems_.size();
    if(itemsCount ==0)
        return;

    const int viewportHeight = viewportSize.height();
    int itemsInRow=1;
    for(int height = 0, item = itemsCount - 1; item >= 0; item-=itemsInRow)
    {
        //UiLog().dbg() << "item=" << item;
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
        // scroll per pixel       
    }

    //Horizontal scrollbar
    if(viewportSize.width() < maxRowWidth_)
    {
        horizontalScrollBar()->setRange(0,maxRowWidth_+10-viewportSize.width());
        horizontalScrollBar()->setPageStep(viewportSize.width());
        horizontalScrollBar()->setSingleStep(1);
    }
    else
    {
        horizontalScrollBar()->setRange(0,0);
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

    int y = -1;
    int rh=0;
    coordinateForItem(vi,y,rh);
    if(y >=0)
    {
        //return QRect(viewItems_[vi].x, y, viewItems_[vi].width,rh); //TODO: optimise it
        return QRect(viewItems_[vi].x-1-translation(), y, viewItems_[vi].width+2,rh);
    }
    return QRect();
}

//Returns the viewport y coordinate for  item.
void CompactView::coordinateForItem(int item,int& itemY,int& itemRowHeight) const
{
    itemY=-1;
    if(verticalScrollMode_ == ScrollPerItem)
    {
        int offset = 0;
        //firstVisibleItem must always start a row!!!!
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
                int h=rowHeight(viewItemIndex,1,itemsInRow);
                if(viewItemIndex <=item && item < viewItemIndex+itemsInRow)
                {
                    itemY=height;
                    itemRowHeight=h;
                    return;
                }
                height +=h;
            }
        }
    }
}

//coordinate is in viewport coordinates
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
                    viewItemIndex=itemAtRowCoordinate(viewItemIndex,itemsInRow,coordinate.x()+translation());
                    return (viewItemIndex >= itemCount ? -1 : viewItemIndex);
                }
            }
        }
    }

    return -1;
}

//return the item index at the absolute x coordinate (i.e. not viewport x coordinate)
int CompactView::itemAtRowCoordinate(int start,int count,int logicalXPos) const
{
    for(int i=start; i < start+count; i++)
    {
        int left=viewItems_[i].x-1;
        int right=viewItems_[i].right()+2;
        if(!viewItems_[i].expanded && viewItems_[i].hasChildren)
            right=viewItems_[i].right()+connectorGap_+expandConnectorLenght_+3;

        if(left <= logicalXPos && right >= logicalXPos)
        {
            return i;
        }
    }
    return -1;
}
