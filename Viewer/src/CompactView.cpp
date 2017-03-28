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

//#define _UI_COMPACTVIEW_DEBUG




CompactView::CompactView(TreeNodeModel* model,QWidget* parent) :
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
    expandButtonSize_(8),
    expandButtonMode_(Classic),
    connectorColour_(Qt::black)
{
    delegate_=new CompactNodeViewDelegate(model_,this);

    itemDelegate_=new QStyledItemDelegate(this);

    //setAutoFillBackground(true);

    viewport()->setBackgroundRole(QPalette::Window);

    //We attach the model. At this point the model is empty so
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
        this,SLOT(dataChanged(const QModelIndex&,const QModelIndex&)));


    selectionModel_ = new QItemSelectionModel(model_, this);
    connect(model_, SIGNAL(destroyed()), selectionModel_, SLOT(deleteLater()));

    connect(selectionModel_, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged(QItemSelection,QItemSelection)));

    connect(selectionModel_, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentChanged(QModelIndex,QModelIndex)));

    //connect(d->selectionModel, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
    //                d->model, SLOT(submit()));


    //We need to call it to be sure that the view show the actual state of the model!!!
    reset();
}

void CompactView::mousePressEvent(QMouseEvent* event)
{
    QPoint pos = event->pos();
    QPersistentModelIndex index = indexAt(pos);
    pressedIndex_ = index;
    //d->pressedAlreadySelected = d->selectionModel->isSelected(index);
    //pressedIndex_ = index;
    //    d->pressedModifiers = event->modifiers();

    QItemSelectionModel::SelectionFlags command = selectionCommand(index, event);

    //d->noSelectionOnMousePress = command == QItemSelectionModel::NoUpdate || !index.isValid();
    QPoint offset;

    if((command & QItemSelectionModel::Current) == 0)
        pressedPosition_ = pos + offset;
    else if(!indexAt(pressedPosition_).isValid())
        pressedPosition_ = visualRect(currentIndex()).center() + offset;

    if(index.isValid())
    {
        selectionModel_->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
        QRect rect(pressedPosition_ - offset, pos);
        setSelection(rect, command);
    }
    else
    {
        // Forces a finalize() even if mouse is pressed, but not on a item
        selectionModel_->select(QModelIndex(), QItemSelectionModel::Select);
    }


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
            updateRowCount();
            updateScrollBars();
            viewport()->update();
        }
    }
}

bool CompactView::viewportEvent(QEvent *event)
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

void CompactView::timerEvent(QTimerEvent *event)
{
    if(event->timerId() == delayedWidth_.timerId())
    {
        updateScrollBars();
        viewport()->update();
        delayedWidth_.stop();
    }
}

QStyleOptionViewItem CompactView::viewOptions() const
{
    QStyleOptionViewItem option;
    option.init(this);
    option.state &= ~QStyle::State_MouseOver;
    option.font = font();

#ifndef Q_WS_MAC
    // On mac the focus appearance follows window activation
    // not widget activation
    if (!hasFocus())
        option.state &= ~QStyle::State_Active;
#endif

    option.state &= ~QStyle::State_HasFocus;
    //if (d->iconSize.isValid()) {
    //    option.decorationSize = d->iconSize;
    //} else {

        int pm = style()->pixelMetric(QStyle::PM_SmallIconSize, 0, this);
        option.decorationSize = QSize(pm, pm);
   // }
    option.decorationPosition = QStyleOptionViewItem::Left;
    option.decorationAlignment = Qt::AlignCenter;
    option.displayAlignment = Qt::AlignLeft|Qt::AlignVCenter;
    //option.textElideMode = d->textElideMode;
    option.rect = QRect();
    option.showDecorationSelected = style()->styleHint(QStyle::SH_ItemView_ShowDecorationSelected, 0, this);
    //if (d->wrapItemText)
    //    option.features = QStyleOptionViewItem::WrapText;
    option.locale = locale();
    option.locale.setNumberOptions(QLocale::OmitGroupSeparator);
    option.widget = this;
    return option;
}

void CompactView::reset()
{
    viewItems_.clear();
    rowCount_=0;
    maxRowWidth_=0;
    expandedIndexes.clear();
    //currentIndexSet_ = false;
    if (selectionModel_)
            selectionModel_->reset();

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
        viewItems_.resize(count);
        afterIsUninitialized = true;
    }
    //The count of the
    else if(viewItems_[parentId].total != (uint)count)
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
    int level=(parentId >=0?viewItems_[parentId].level+1:0);
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

        //We compute the size of the item. For attributes we delay the width computation until we
        //actually paint them and we set their width to 300.
        int w,h;
        delegate_->sizeHint(currentIndex,w,h);
        item->width=w;
        item->height=h;

        if(item->right() > maxRowWidth_)
            maxRowWidth_=item->right();

        int xp=leftMargin_;
        if(parentId >=0)
        {
            xp=viewItems_[parentId].right()+itemGap_;
        }

        item->x=xp;
        //UiLog().dbg() << "layout --> " << item->index.data().toString() << " x=" << item->x;

        if(item->right() > maxRowWidth_)
            maxRowWidth_=item->right();

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

        //When we reach a leaf node we reach the end of a row as well.
        if((i != first || parentId == -1) && item->total == 0)
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

void CompactView::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    paint(&painter,event->region());
}

//Paint the rows intersecting with the given region
void CompactView::paint(QPainter *painter,const QRegion& region)
{
    //Even though the viewport palette is set correctly at the
    //beginning something sets it to another value. Here we try
    //to detect it and correct the palette with right colour.
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
    UiLog().dbg() << "CompactNodeView::paint -->";
    UiLog().dbg() << "sizeof(CompactViewItem)=" << sizeof(CompactViewItem);
    UiLog().dbg() << "region=" << region;
    qDebug() << painter->pen() << painter->brush();
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

        //Initialise indentVec. For each indentation level it tells us if
        //a connector line is to be drawn. Here we scan up to the
        //toplevel item in the firstVisible item's branch.
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
                    indentVec[item->level]=connectorPos(item,pt);
                }
                Q_ASSERT(pt->level == level-1);
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
            //Draw a whole row. It will update y,itemsInRow and indentVec!!
            drawRow(painter,i,xOffset,y,itemsInRow,indentVec);
#ifdef _UI_COMPACTVIEW_DEBUG
//            UiLog().dbg() << " drawRow " << i << " " << y << " " << itemsInRow;
#endif
        }
    }
}

//Draw a whole row staring at item start.
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

    QVector<QPoint> expandButtons;
    QVector<QPoint> collapseButtons;

    //We iterate through the items in the row
    for(int i=start; i < itemsCount && !leaf; ++i )
    {
        CompactViewItem* item=&(viewItems_[i]);
#ifdef _UI_COMPACTVIEW_DEBUG
        UiLog().dbg() << "item=" << i << " " << item->index.data().toString();
#endif
        leaf=(item->total == 0);

        //Init style option
        QStyleOptionViewItem opt;
        if(selectionModel_->isSelected(item->index))
            opt.state |= QStyle::State_Selected;

        int optWidth=2000;
        if(item->width > optWidth)
            optWidth=item->width;
        opt.rect=QRect(item->x,yp,optWidth,item->height);

        //For single rows we center items halfway through the rowHeight
        if(singleRow)
        {
            if(item->height < rh)
            {
                opt.rect.moveTop(yp+(rh-item->height)/2);
            }
        }

        //Draw the item with the delegate
        int paintedWidth=delegate_->paintItem(painter,opt,item->index);

        //we have to know if the item width is the same that we exepcted
        if(paintedWidth != item->width)
        {
            item->width=paintedWidth;
            //For non-leaf nodes we have to shift the children
            if(item->parentItem >=0 && item->total > 0)
            {
                shiftItems(i);
                doDelayedWidthAdjustment();
            }
            else if(item->total == 0)
            {
                if(item->right() > maxRowWidth_)
                {
                    maxRowWidth_=item->right();
                    doDelayedWidthAdjustment();
                }
            }
        }

        //QRect rr=opt.rect;
        //rr.setWidth(item->width);
        //painter->drawRect(rr);

        //Collect expand/collapse button positions
        if(item->hasChildren)
        {

                QPoint p(item->right()+itemGap_/2,yp+item->height/2);
                if(item->expanded)
                    collapseButtons << QPoint(item->right()+itemGap_/2,yp+item->height/2);
                else
                    expandButtons << QPoint(item->right()+expandButtonSize_/2+1,yp+item->height/2);
           }

        //UiLog().dbg() << i << " " << viewItems_[i]->index << " " << viewItems_[i]->index.data().toString() << " "
        //              << viewItems_[i]->x << " " << viewItems_[i]->height << " " << leaf;

        //Find out the first indentation level in the row
        if(firstLevel==0)
            firstLevel=item->level;

        painter->setPen(connectorColour_);

        //If not a top level item (e.i. not a server)
        if(item->parentItem >=0)
        {          
            //The parent item. It is always a node.
            CompactViewItem* pt=&(viewItems_[item->parentItem]);

            //The horizontal line connecting the item to its parent           
            int lineX1=pt->right()+connectorGap_;
            int lineX2=item->x-connectorGap_;
            int lineX=(pt->right()+item->x)/2;

#ifdef _UI_COMPACTVIEW_DEBUG
            UiLog().dbg() << "  lineX=" << lineX << " " << item->x << " " << connectorPos(item,pt);
#endif
            Q_ASSERT(lineX==connectorPos(item,pt));

            //First child - in the same row as its parent
            if(item->index.row() == 0)
            {
                int lineY=yp+pt->height/2;

                //horizontal line to the parent
                painter->drawLine(lineX1,lineY,lineX2,lineY);

                //line towards the siblings  - downwards
                if(item->hasMoreSiblings)
                {
                    painter->drawLine(lineX,lineY,lineX,lineY+rh/2);
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
                painter->drawLine(lineX,lineY+rh/2,lineX,lineY-rh/2);
                indentVec[item->level]=lineX;
            }

            //The last child - has sibling only upwards
            else
            {
                int lineY=yp+item->height/2;
                painter->drawLine(lineX,lineY,lineX2,lineY);
                painter->drawLine(lineX,lineY,lineX,lineY-rh/2);
                indentVec[item->level]=0;
            }
        }

        if(expandButtonMode_ == Classic)
        {
            if(item->hasChildren && !item->expanded)
            {
                int lineY=yp+item->height/2;
                int lineX=item->right()+connectorGap_;
                QPen oriPen=painter->pen();
                painter->setPen(QPen(connectorColour_,1,Qt::DashLine));
                painter->drawLine(lineX,lineY,lineX+itemGap_-2*connectorGap_,lineY);
                painter->setPen(oriPen);
            }
        }

        //When we reach a leaf item we move one row down.
        if(leaf)
        {            
            //Draw the vertical connector lines for all the levels
            //preceding the first level in the row!
            int level=item->level;
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

    if(expandButtonMode_ == Modern)
    {
        if(expandButtons.isEmpty() == false)
        {
            QBrush brushOri=painter->brush();
            QPen penOri=painter->pen();
            painter->setBrush(QColor(250,250,250));
            Q_FOREACH(QPoint p,expandButtons)
            {
                painter->setPen(connectorColour_);
                QRect r(p.x()-expandButtonSize_/2,p.y()-expandButtonSize_/2,expandButtonSize_,expandButtonSize_);
                painter->drawRect(r);
                int xc=r.left()+4;
                int yc=r.top()+4;
                //painter->setPen(QColor(20,20,20));
                painter->drawLine(r.left()+2,yc,r.right()-1,yc);
                painter->drawLine(xc,r.top()+2,xc,r.bottom()-1);
            }

            painter->setBrush(brushOri);
            painter->setPen(penOri);
        }

        if(collapseButtons.isEmpty() == false)
        {
            QBrush brushOri=painter->brush();
            QPen penOri=painter->pen();
            painter->setBrush(QColor(250,250,250));
            Q_FOREACH(QPoint p,collapseButtons)
            {
                painter->setPen(connectorColour_);
                QRect r(p.x()-expandButtonSize_/2,p.y()-expandButtonSize_/2,expandButtonSize_,expandButtonSize_);
                painter->drawRect(r);
                int xc=r.left()+4;
                int yc=r.top()+4;
                //painter->setPen(QColor(20,20,20));
                painter->drawLine(r.left()+2,yc,r.right()-1,yc);
            }

            painter->setBrush(brushOri);
            painter->setPen(penOri);
        }
    }

    if(itemsInRow == 0)
       itemsInRow=1;
}

void CompactView::shiftItems(int start)
{
    int n=start+viewItems_[start].total+1;
    int w=0, h=0;
    for(int i=start+1; i < n; i++)
    {
        delegate_->sizeHint(viewItems_[i].index,w,h);
        viewItems_[i].width=w;
        int pt=viewItems_[i].parentItem;
        viewItems_[i].x=viewItems_[pt].right()+itemGap_;
        if(viewItems_[i].right() > maxRowWidth_)
            maxRowWidth_=viewItems_[i].right();
    }
}

int CompactView::connectorPos(CompactViewItem* item, CompactViewItem* parent) const
{
    return (parent->right()+item->x)/2;
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

/*
    This slot is called when items are changed in the model. The
    changed items are those from topLeft to bottomRight
    inclusive. If just one item is changed topLeft ==
    bottomRight.
*/
void CompactView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    // Single item changed
    if (topLeft == bottomRight && topLeft.isValid())
    {
        update(topLeft);
        return;
    }

    viewport()->update();
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
        CompactViewItem* item=&(viewItems_[i]);
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
            CompactViewItem* pt=&(viewItems_[item->parentItem]);

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

    Q_ASSERT(itemsInRow > 0);
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
        Q_ASSERT(start >= 0);
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
        if(viewItems_[i].total == 0)
            return itemsInRow;
    }

    Q_ASSERT(itemsInRow > 0);
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

void CompactView::resizeEvent(QResizeEvent *event)
{
    QAbstractScrollArea::resizeEvent(event);
    updateScrollBars();
    viewport()->update();
}

void CompactView::updateRowCount()
{
    rowCount_=0;
    const int itemsCount = static_cast<int>(viewItems_.size());
    int itemsInRow=0;
    for (int i=0; i < itemsCount; i+=itemsInRow)
    {
        itemsInRow=itemCountInRow(i);
        rowCount_++;
    }
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

void CompactView::doDelayedWidthAdjustment()
{
    if(!delayedWidth_.isActive())
    {
        delayedWidth_.start(0,this);
    }
}

int CompactView::translation() const
{
    return horizontalScrollBar()->value();
}

void CompactView::scrollTo(const QModelIndex &index)
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

    Q_FOREACH(QModelIndex parent,parentLst)
    {
        if(!isExpanded(parent))
            expand(parent);
    }

#if 0
    while(parent.isValid())
    {
        if(!isExpanded(parent))
            expand(parent);
        parent = model_->parent(parent);
    }
#endif

    int item = viewIndex(index);
    if (item < 0)
        return;

    //QRect area = viewport()->rect();

    //vertical
    //hint == EnsureVisible

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
            verticalScrollBar()->setValue(item);
        }
        else
        {
            verticalScrollBar()->setValue(item);
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
        return QRect(viewItems_[vi].x, y, viewItems_[vi].width,rh); //TODO: optimise it
    }
    return QRect();
}

QModelIndex CompactView::indexAt(const QPoint &point) const
{
    int item=itemAtCoordinate(point);
    return (item>=0)?viewItems_[item].index:QModelIndex();
}

/*
  Returns the viewport y coordinate for  item.
*/
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
                    viewItemIndex=itemAtRowCoordinate(viewItemIndex,itemsInRow,coordinate.x()+translation());
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

int CompactView::itemAtRowCoordinate(int start,int count,int logicalXPos) const
{
    for(int i=start; i < start+count; i++)
    {
        if(viewItems_[i].x <= logicalXPos && viewItems_[i].right() >= logicalXPos)
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
        //updateRowCount();
        updateScrollBars();
        viewport()->update();
    }
}

void CompactView::collapse(const QModelIndex &index)
{
    int i = viewIndex(index);
    if (i != -1) // is visible
    {
        //updateRowCount();
        collapse(i);
        updateRowCount();
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

//========================================================
//
// Selection
//
//========================================================

/*
    Sets the current item to be the item at a index.

    Unless the current selection mode is
    \l{QAbstractItemView::}{NoSelection}, the item is also be selected.
    Note that this function also updates the starting position for any
    new selections the user performs.

    To set an item as the current item without selecting it, call

    \c{selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);}

    \sa currentIndex(), currentChanged(), selectionMode
*/

void CompactView::setCurrentIndex(const QModelIndex &index)
{
    if(selectionModel_ && index.isValid())
    {
        QItemSelectionModel::SelectionFlags command = selectionCommand(index, 0);
        selectionModel_->setCurrentIndex(index, command);
        //currentIndexSet_ = true;
        QPoint offset;
        if((command & QItemSelectionModel::Current) == 0)
            pressedPosition_ = visualRect(currentIndex()).center() + offset;
    }
}

/*
    Returns the model index of the current item.
*/
QModelIndex CompactView::currentIndex() const
{
    return selectionModel_ ? selectionModel_->currentIndex() : QModelIndex();
}


QModelIndexList CompactView::selectedIndexes() const
{
    if(selectionModel_)
        return selectionModel_->selectedIndexes();

    return QModelIndexList();
}


/*
  Applies the selection command to the items in or touched by the
  rectangle rect.
*/
void CompactView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
    if (!selectionModel_ || rect.isNull())
        return;

    //d->executePostedLayout();
    QPoint tl(isRightToLeft() ? qMax(rect.left(), rect.right())
              : qMin(rect.left(), rect.right()), qMin(rect.top(), rect.bottom()));
    QPoint br(isRightToLeft() ? qMin(rect.left(), rect.right()) :
              qMax(rect.left(), rect.right()), qMax(rect.top(), rect.bottom()));

    QModelIndex topLeft = indexAt(tl);
    QModelIndex bottomRight = indexAt(br);
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

void CompactView::select(const QModelIndex &topIndex, const QModelIndex &bottomIndex,
                              QItemSelectionModel::SelectionFlags command)
{
    QItemSelection selection;
    const int top = viewIndex(topIndex),
    bottom = viewIndex(bottomIndex);

#if 0
    const QList< QPair<int, int> > colRanges = columnRanges(topIndex, bottomIndex);
    QList< QPair<int, int> >::const_iterator it;
    for (it = colRanges.begin(); it != colRanges.end(); ++it) {
        const int left = (*it).first,
            right = (*it).second;
#endif

    QModelIndex previous;
    QItemSelectionRange currentRange;
    QStack<QItemSelectionRange> rangeStack;
    for(int i = top; i <= bottom; ++i)
    {
        QModelIndex index = modelIndex(i);
        QModelIndex parent = index.parent();
        QModelIndex previousParent = previous.parent();
        if (previous.isValid() && parent == previousParent)
        {
            // same parent
            if (qAbs(previous.row() - index.row()) > 1)
            {
                //a hole (hidden index inside a range) has been detected
                if (currentRange.isValid())
                {
                    selection.append(currentRange);
                }
                //let's start a new range
                currentRange = QItemSelectionRange(index.sibling(index.row(),0), index.sibling(index.row(),0));
            }
            else
            {
                QModelIndex tl = model_->index(currentRange.top(), currentRange.left(),
                        currentRange.parent());
                currentRange = QItemSelectionRange(tl, index.sibling(index.row(),0));
            }
        }
        else if (previous.isValid() && parent == model_->index(previous.row(), 0, previousParent))
        {
            // item is child of previous
            rangeStack.push(currentRange);
            currentRange = QItemSelectionRange(index.sibling(index.row(), 0), index.sibling(index.row(),0));
        }
        else
        {
            if(currentRange.isValid())
                selection.append(currentRange);
            if(rangeStack.isEmpty())
            {
                currentRange = QItemSelectionRange(index.sibling(index.row(),0), index.sibling(index.row(),0));
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

QItemSelectionModel::SelectionFlags CompactView::selectionCommand(
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
QRegion CompactView::visualRegionForSelection(const QItemSelection &selection) const
{
    if(selection.isEmpty())
        return QRegion();

    QRegion selectionRegion;
    const QRect &viewportRect = viewport()->rect();
    for (int i = 0; i < selection.count(); ++i)
    {
        QItemSelectionRange range = selection.at(i);
        if (!range.isValid())
            continue;
        QModelIndex parent = range.parent();
        QModelIndex leftIndex = range.topLeft();
        int columnCount = model_->columnCount(parent);
        if (!leftIndex.isValid())
            continue;
        const QRect leftRect = visualRect(leftIndex);
        int top = leftRect.top();
        QModelIndex rightIndex = range.bottomRight();
        if (!rightIndex.isValid())
            continue;
        const QRect rightRect = visualRect(rightIndex);
        int bottom = rightRect.bottom();
        if (top > bottom)
            qSwap<int>(top, bottom);
        int height = bottom - top + 1;

        QRect combined = leftRect|rightRect;
        combined.setX(range.left());
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
void CompactView::selectionChanged(const QItemSelection &selected,
                                   const QItemSelection &deselected)
{
    if(isVisible()) // && updatesEnabled()) {
        viewport()->update(visualRegionForSelection(deselected) | visualRegionForSelection(selected));
}

/*
    This slot is called when a new item becomes the current item.
    The previous current item is specified by the previous index, and the new
    item by the current index.
*/
void CompactView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
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


