//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "GraphNodeView.hpp"

#include "ActionHandler.hpp"
#include "ExpandState.hpp"
#include "PropertyMapper.hpp"
#include "TreeNodeModel.hpp"
#include "UiLog.hpp"
#include "VFilter.hpp"

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QMouseEvent>

static int XPOS=0;

#if 0
struct GraphNodeViewItem
{
    GraphNodeViewItem() : parentItem(-1), expanded(false), spanning(false), hasChildren(false),
                      hasMoreSiblings(false), total(0), level(0), height(0) {}
    QModelIndex index; // we remove items whenever the indexes are invalidated
    int parentItem; // parent item index in viewItems
    uint expanded : 1;
    uint spanning : 1;
    uint hasChildren : 1; // if the item has visible children (even if collapsed)
    uint hasMoreSiblings : 1;
    uint total : 28; // total number of children visible
    uint level : 16; // indentation
    int height : 16; // row height
};
#endif

class GraphNodeViewItem : public QGraphicsItem
{
public:
    GraphNodeViewItem(const QModelIndex& index,QGraphicsItem* parent=0);

    QRectF boundingRect() const;
    void paint(QPainter*, const QStyleOptionGraphicsItem *,QWidget*);
    const QModelIndex& index();
    //QRect pixmapRect();
    //QString text();
    //QFont font();
    //void adjust();
    //bool textContains(QPointF);
    //void setDropTarget(bool);
    //enum BlinkState {NoBlink,BlinkOn,BlinkOff};
    //void setBlinkState(BlinkState);
    //QPointF textPos();

protected:
    //void hoverEnterEvent(QGraphicsSceneHoverEvent *);
    //void hoverLeaveEvent(QGraphicsSceneHoverEvent *);

    //MvQIconFolderView* view_;
    //MvQFolderItemProperty* itemProp_;
    QModelIndex index_;
    //QRectF bRect_;

    uint expanded : 1;
    uint spanning : 1;
    uint hasChildren : 1; // if the item has visible children (even if collapsed)
    uint hasMoreSiblings : 1;
    uint total : 28; // total number of children visible
    uint level : 16; // indentation
    int height : 16; // row height

    //We make these static members because they are the same for all instances
    //static QPen editPen_;
    //static QBrush editBrush_;
    //static QPen hoverPen_;
    //static QBrush hoverBrush_;
    //static QPen selectPen_;
    //static QBrush selectBrush_;
    //static int cornerRad_;
};

GraphNodeViewItem::GraphNodeViewItem(const QModelIndex& index,QGraphicsItem* parent) :
    index_(index)
    //QGraphicsItem(parent)
{
    if(parent)
    {
        int chNum=parent->childItems().count();
        //Q_ASSERT(chNum>0);
        //first child
        //if(chNum == 1)
        //{
        //    setPos(QPoint(parent->boundingRect().width()+10,0));
        //}
       //else
        {
            QRectF cb=parent->scene()->sceneRect();
            //parent->childrenBoundingRect();

            XPOS+=20;
            //QGraphicsItem* s=parent->childItems()[chNum-2];
            //Q_ASSERT(s);
            //QRectF pb=s->boundingRect();

            QRectF brParent=parent->mapToScene(parent->boundingRect()).boundingRect();

            setPos(QPoint(brParent.width()+10,
                          brParent.top()+cb.height()+10));
        }
    }
    else
    {
        setPos(20,10);
    }
}

QRectF GraphNodeViewItem::boundingRect() const
{
    QFont f;
    QFontMetrics fm(f);
    int w=fm.width(index_.data(Qt::DisplayRole).toString())+5;
    int h=fm.height();
    return QRectF(0,0,w,h);
}

void GraphNodeViewItem::paint(QPainter* p, const QStyleOptionGraphicsItem *,QWidget*)
{
    QRectF r=boundingRect();
    p->drawRect(r);
    p->drawText(r,index_.data(Qt::DisplayRole).toString());
}

GraphNodeView::GraphNodeView(TreeNodeModel* model,NodeFilterDef* filterDef,QWidget* parent) :
    QGraphicsView(parent),
    NodeViewBase(filterDef),
    model_(model),
    needItemsLayout_(false),
    //defaultIndentation_(indentation()),
    prop_(NULL),
    setCurrentIsRunning_(false),
    setCurrentFromExpand_(false)
{
    setObjectName("view");
    setProperty("style","nodeView");
    setProperty("view","tree");

    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setDragMode(QGraphicsView::RubberBandDrag);

    QGraphicsScene* scene_=new QGraphicsScene(this);
    setScene(scene_);

    setContextMenuPolicy(Qt::CustomContextMenu);

    //Context menu
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(slotContextMenu(const QPoint &)));

    UiLog().dbg() << maximumViewportSize();
    UiLog().dbg()  << "scenerect" << sceneRect();

    //expandState_=new ExpandState(this,model_);
    actionHandler_=new ActionHandler(this);

    //We attach the model because by default the view is enabled. At this point the model is empty so
    //it is a cheap operation!!
    attachModel();
}

GraphNodeView::~GraphNodeView()
{
    //delete expandState_;
    delete actionHandler_;
    //delete prop_;
}

QWidget* GraphNodeView::realWidget()
{
    return this;
}

VInfo_ptr GraphNodeView::currentSelection()
{
#if 0
    QModelIndexList lst=selectedIndexes();
    if(lst.count() > 0)
    {
        return model_->nodeInfo(lst.front());
    }
#endif
    return VInfo_ptr();
}

void GraphNodeView::setCurrentSelection(VInfo_ptr info)
{
#if 0

    //While the current is being selected we do not allow
    //another setCurrent call go through
    if(!info || setCurrentIsRunning_)
        return;

    setCurrentIsRunning_=true;
    QModelIndex idx=model_->infoToIndex(info);
    if(idx.isValid())
    {
#ifdef _UI_TREENODEVIEW_DEBUG
        UiLog().dbg() << "TreeNodeView::setCurrentSelection --> " << info->path();
#endif
        //setCurrentIndex(idx);
    }
    setCurrentIsRunning_=false;
#endif
}

void GraphNodeView::reload()
{
    //model_->reload();
    //expandAll();
}

void GraphNodeView::rerender()
{
    if(needItemsLayout_)
    {
        //doItemsLayout();
        needItemsLayout_=false;
    }
    else
    {
        //viewport()->update();
    }
}

//Connect the models signal to the view
void GraphNodeView::attachModel()
{
    //Standard signals from the model
    connect(model_,SIGNAL(modelReset()),
        this,SLOT(reset()));

    connect(model_, SIGNAL(rowsInserted(QModelIndex,int,int)),
              this, SLOT(rowsInserted(QModelIndex,int,int)));


    connect(model_,SIGNAL(dataChanged(const QModelIndex&,const QModelIndex&)),
        this,SLOT(slotDataChanged(const QModelIndex&,const QModelIndex&)));

    //We need to call it to be sure that the view show the actual state of the model!!!
    //doReset();
}

void GraphNodeView::reset()
{

    //removeEditor();
    //removeDropTarget();
    scene()->clear();
    //items_.clear();

    //initImportedHalfSizeItems();

    XPOS=20;
    insertItems(QModelIndex());

#if 0
    for(int i=0; i < model_->rowCount(); i++)
    {
        QModelIndex idx=model_->index(i,0);

        UiLog().dbg() << "reset" << idx;

       // MvQIconItem* item=new MvQIconItem(idx,itemProp_,this);
        //scene()->addItem(item);

        //QPersistentModelIndex idxP(idx);
        //items_[idxP]=item;
    }
#endif

    //Checks if there are icons with (0,0) positions and layout them
    //nicely
    //checkPositions();

   // adjustSceneRect();

}


void GraphNodeView::rowsInserted(QModelIndex,int,int)
{
    //for(int i=0; i < 1000000; i++)
    // {
    //    new GraphNodeViewItem(QModelIndex(),0);
    //}
    reset();
}

void GraphNodeView::insertItems(const QModelIndex& parent,GraphNodeViewItem *p)
{
    UiLog().dbg() << "insert --> parent=" << parent.data().toString() << " rows=" << model_->rowCount(parent) << " " << sizeof(GraphNodeViewItem);
    for(int i=0; i < model_->rowCount(parent); i++)
    {
        QModelIndex idx=model_->index(i,0,parent);
        UiLog().dbg() << " " << i << " " << idx.data().toString();

        if(p)
        {
            if(i==0)
            {
                insertItems(idx,p);
            }
            else
            {
                GraphNodeViewItem* item=new GraphNodeViewItem(idx,p);
                scene()->addItem(item);
                insertItems(idx,item);
            }
        }
#if 0
        //Root
        if(p==0)
           item->setPos(20,XPOS);
        else
        {
            if(i==0)
                item->setPos(p->pos()+QPoint(p->boundingRect().width()+10,0));
            else
            {
                XPOS+=20;
                item->setPos(p->pos()+QPoint(p->boundingRect().width()+10,XPOS));
            }
        }
#endif
        else
        {
            GraphNodeViewItem* item=new GraphNodeViewItem(idx,p);
            scene()->addItem(item);
            insertItems(idx,item);
        }
    }
}

void GraphNodeView::mousePressEvent(QMouseEvent* event)
{
    QGraphicsItem* item=itemAt(event->pos());
    if(item)
    {
        QList<QGraphicsItem*> ch=item->childItems();
        if(ch.count() > 0)
        {
            bool h=ch[0]->isVisible();
            Q_FOREACH(QGraphicsItem* it,ch)
            {
                it->setVisible(!h);
            }
        }
    }
}













