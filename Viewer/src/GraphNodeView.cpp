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
#include "TreeNodeViewDelegate.hpp"
#include "UiLog.hpp"
#include "VFilter.hpp"

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QMouseEvent>

#include <QScrollBar>

static int XPOS=0;

CompactViewItem::CompactViewItem(const QModelIndex idx,int parentId,int xp, bool leaf) :
    index(idx), parentItem(parentId), expanded(true), x(xp)
{
    width=20;
    height=20;

    //if(!leaf)
    //{
    //    width=computeWidth();
    //}
}

void CompactViewItem::paintItem(QPainter *p,int yp,TreeNodeViewDelegate* delegate)
{
    QStyleOptionViewItem option;
    option.rect=QRect(x,yp,width+10,height);

    delegate->paint(p,option,index);

   // p->setPen(Qt::black);
  //  QRect r(x,yp,width,height);
   // p->drawRect(r);
   // p->drawText(r,index.data().toString());
}

int CompactViewItem::computeWidth()
{
    QFont f;
    QFontMetrics fm(f);
    int w=fm.width(index.data().toString())+10;
    return qMin(qMax(50,w),100);
}


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


#if 0
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
        UiLog().dbg() << "GraphNodeView::setCurrentSelection --> " << info->path();
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



#endif


CompactNodeView::CompactNodeView(TreeNodeModel* model,NodeFilterDef* filterDef,QWidget* parent) :
    QAbstractScrollArea(parent),
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

    //setAlignment(Qt::AlignLeft | Qt::AlignTop);
    //setDragMode(QGraphicsView::RubberBandDrag);

    setContextMenuPolicy(Qt::CustomContextMenu);

    //Context menu
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(slotContextMenu(const QPoint &)));

    //UiLog().dbg() << maximumViewportSize();
    //UiLog().dbg()  << "scenerect" << sceneRect();

    //expandState_=new ExpandState(this,model_);
    actionHandler_=new ActionHandler(this);

    delegate_=new TreeNodeViewDelegate(this);

    //We attach the model because by default the view is enabled. At this point the model is empty so
    //it is a cheap operation!!
    attachModel();

    //Properties
    std::vector<std::string> propVec;
    propVec.push_back("view.tree.indentation");
    propVec.push_back("view.tree.background");
    //propVec.push_back("view.tree.drawBranchLine");
    propVec.push_back("view.tree.serverToolTip");
    propVec.push_back("view.tree.nodeToolTip");
    propVec.push_back("view.tree.attributeToolTip");
    prop_=new PropertyMapper(propVec,this);

    //Initialise indentation
    Q_ASSERT(prop_->find("view.tree.indentation"));
    adjustIndentation(prop_->find("view.tree.indentation")->value().toInt());

    //Init stylesheet related properties
    Q_ASSERT(prop_->find("view.tree.background"));
    adjustBackground(prop_->find("view.tree.background")->value().value<QColor>(),false);
    //Q_ASSERT(prop_->find("view.tree.drawBranchLine"));
    //adjustBranchLines(prop_->find("view.tree.drawBranchLine")->value().toBool(),false);
    //adjustStyleSheet();

    //Adjust tooltip
    Q_ASSERT(prop_->find("view.tree.serverToolTip"));
    adjustServerToolTip(prop_->find("view.tree.serverToolTip")->value().toBool());

    Q_ASSERT(prop_->find("view.tree.nodeToolTip"));
    adjustNodeToolTip(prop_->find("view.tree.nodeToolTip")->value().toBool());

    Q_ASSERT(prop_->find("view.tree.attributeToolTip"));
    adjustAttributeToolTip(prop_->find("view.tree.attributeToolTip")->value().toBool());

}

CompactNodeView::~CompactNodeView()
{
    //delete expandState_;
    delete actionHandler_;
    //delete prop_;
}

QWidget* CompactNodeView::realWidget()
{
    return this;
}

VInfo_ptr CompactNodeView::currentSelection()
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

void CompactNodeView::setCurrentSelection(VInfo_ptr info)
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
        UiLog().dbg() << "GraphNodeView::setCurrentSelection --> " << info->path();
#endif
        //setCurrentIndex(idx);
    }
    setCurrentIsRunning_=false;
#endif
}

void CompactNodeView::reload()
{
    //model_->reload();
    //expandAll();
}

void CompactNodeView::rerender()
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
void CompactNodeView::attachModel()
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

void CompactNodeView::reset()
{
    viewItems_.clear();
    insertItems(QModelIndex(),-1);
}

void CompactNodeView::rowsInserted(QModelIndex,int,int)
{
    reset();
}

void CompactNodeView::insertItems(const QModelIndex& parent,int parentPos)
{
    UiLog().dbg() << "insert --> parent=" << parent.data().toString() << " rows=" << model_->rowCount(parent);

    int dx=10;
    int yp=20;

    CompactViewItem* pt=0;
    int rowNum=model_->rowCount(parent);
    if(parentPos != -1)
    {
        pt=viewItems_[parentPos];
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
        CompactViewItem* item=new CompactViewItem(idx,parentPos,xp,leaf);
        item->hasMoreSiblings=(i < rowNum-1);

        int w,h;
        delegate_->sizeHint(idx,w,h);
        item->width=w;
        item->height=h;

        if(leaf || i > 0)
            yp+=item->height+5;

        viewItems_ << item;

        insertItems(idx,viewItems_.count()-1);
    }

    verticalScrollBar()->setRange(0, yp+20);

}


void CompactNodeView::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    paint(&painter,event->region());
}


void CompactNodeView::paint(QPainter *painter,const QRegion& region)
{
    UiLog().dbg() << "size=" << sizeof(CompactViewItem);

    int firstVisible_=0;

    int parent=-1;
    int yp=5;
    int xp;
    std::vector<int> indentVec;
    int indentNum=0;
    for(int i=firstVisible_; i < viewItems_.count(); i++)
    {
        if(viewItems_[i]->expanded == 1)
        {
            viewItems_[i]->paintItem(painter,yp,delegate_);
            bool leaf=!model_->hasChildren(viewItems_[i]->index);

            //UiLog().dbg() << i << " " << viewItems_[i]->index << " " << viewItems_[i]->index.data().toString() << " "
            //              << viewItems_[i]->x << " " << viewItems_[i]->height << " " << leaf;

            if( viewItems_[i]->parentItem >=0)
            {
                CompactViewItem* pt=viewItems_[viewItems_[i]->parentItem];

                int lineY=yp+pt->height/2;
                int lineX1=pt->right()+2;
                int lineX2=viewItems_[i]->x-2;
                int lineX=(lineX1+lineX2)/2;

                //First child
                if(viewItems_[i]->index.row() == 0)
                {
                    painter->drawLine(lineX1,lineY,lineX2,lineY);
                    indentVec.push_back(lineX);

                    if(viewItems_[i]->hasMoreSiblings)
                    {
                        painter->drawLine(lineX,lineY,lineX,lineY+20);
                    }


                    //if(model_->rowCount(pt->index) > 1)
                    //{
                    //    painter->drawLine(pt->right()+2,yp+pt->height/2,viewItems_[i]->x-2,yp+pt->height/2);
                    //}
                }

                else if(viewItems_[i]->hasMoreSiblings)
                {
                    painter->drawLine(lineX,lineY,lineX2,lineY);
                    painter->drawLine(lineX,lineY+20,lineX,lineY-20);
                }

                else
                {
                    painter->drawLine(lineX,lineY,lineX2,lineY);
                    painter->drawLine(lineX,lineY,lineX,lineY-20);
                }

            }


            //if(leaf || viewItems_[i]->index.row() > 0)
            if(leaf)
            {
                yp+=viewItems_[i]->height+5;
            }

            parent=i;
        }

    }
}

void CompactNodeView::adjustStyleSheet()
{
    QString sh;
    if(styleSheet_.contains("bg"))
       sh+=styleSheet_["bg"];
    if(styleSheet_.contains("branch"))
       sh+=styleSheet_["branch"];

   UiLog().dbg() << "stylesheet" << sh;

    setStyleSheet(sh);
}

void CompactNodeView::adjustIndentation(int offset)
{
    if(offset >=0)
    {
        //setIndentation(defaultIndentation_+offset);
        //delegate_->setIndentation(indentation());
    }
}

void CompactNodeView::adjustBackground(QColor col,bool adjust)
{
    if(col.isValid())
    {
        styleSheet_["bg"]="QTreeView { background : " + col.name() + ";}";

        if(adjust)
            adjustStyleSheet();
    }
}

#if 0
void GraphNodeView::adjustBranchLines(bool st,bool adjust)
{
    if(styleSheet_.contains("branch"))
    {
        bool oriSt=styleSheet_["branch"].contains("url(:");
        if(oriSt == st)
            return;
    }

    QString vline((st)?"url(:/viewer/tree_vline.png) 0":"none");
    QString bmore((st)?"url(:/viewer/tree_branch_more.png) 0":"none");
    QString bend((st)?"url(:/viewer/tree_branch_end.png) 0":"none");

    styleSheet_["branch"]="QTreeView::branch:has-siblings:!adjoins-item { border-image: " + vline + ";}" \
     "QTreeView::branch:!has-children:has-siblings:adjoins-item {border-image: " +  bmore + ";}" \
     "QTreeView::branch:!has-children:!has-siblings:adjoins-item {border-image: " + bend + ";}";

    if(adjust)
        adjustStyleSheet();
}
#endif

void CompactNodeView::adjustServerToolTip(bool st)
{
    model_->setEnableServerToolTip(st);
}

void CompactNodeView::adjustNodeToolTip(bool st)
{
    model_->setEnableNodeToolTip(st);
}

void CompactNodeView::adjustAttributeToolTip(bool st)
{
    model_->setEnableAttributeToolTip(st);
}

void CompactNodeView::notifyChange(VProperty* p)
{
    if(p->path() == "view.tree.indentation")
    {
        adjustIndentation(p->value().toInt());
    }
    else if(p->path() == "view.tree.background")
    {
        adjustBackground(p->value().value<QColor>());
    }
    else if(p->path() == "view.tree.drawBranchLine")
    {
        //adjustBranchLines(p->value().toBool());
    }
    else if(p->path() == "view.tree.serverToolTip")
    {
        adjustServerToolTip(p->value().toBool());
    }
    else if(p->path() == "view.tree.nodeToolTip")
    {
        adjustNodeToolTip(p->value().toBool());
    }
    else if(p->path() == "view.tree.attributeToolTip")
    {
        adjustAttributeToolTip(p->value().toBool());
    }
}
