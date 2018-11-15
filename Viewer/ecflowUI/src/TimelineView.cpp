//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TimelineView.hpp"

#include <QDebug>
#include <QtGlobal>
#include <QApplication>
#include <QComboBox>
#include <QHBoxLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStyle>
#include <QToolButton>

#include "ActionHandler.hpp"
#include "IconProvider.hpp"
#include "PropertyMapper.hpp"
#include "TimelineData.hpp"
#include "TimelineModel.hpp"
#include "TimelineInfoWidget.hpp"
#include "UiLog.hpp"
#include "ViewerUtil.hpp"
#include "VFilter.hpp"
#include "VNState.hpp"
#include "VSettings.hpp"

#define _UI_TimelineView_DEBUG

static std::vector<std::string> propVec;

//======================================================================
//
// TimelineView
//
//======================================================================

TimelineDelegate::TimelineDelegate(TimelineModel *model,QWidget *parent) :
    model_(model),
    borderPen_(QPen(QColor(216,216,216)))
{
    Q_ASSERT(model_);

    //columns_=ModelColumn::def("table_columns");

   // nodeBox_=new TableNodeDelegateBox;
   // attrBox_=new TableAttrDelegateBox;

   //nodeBox_->adjust(font_);
    //attrFont_=font_;
    //attrBox_->adjust(attrFont_);

    //Property
    if(propVec.empty())
    {
        propVec.push_back("view.table.font");

        //Base settings
        //addBaseSettings(propVec);
    }

    prop_=new PropertyMapper(propVec,this);

    updateSettings();
}

TimelineDelegate::~TimelineDelegate()
{
}

void TimelineDelegate::notifyChange(VProperty* p)
{
    updateSettings();
}

void TimelineDelegate::updateSettings()
{
    if(VProperty* p=prop_->find("view.table.font"))
    {
        QFont newFont=p->value().value<QFont>();

        if(font_ != newFont)
        {
            font_=newFont;
            //attrFont_=newFont;
            //nodeBox_->adjust(font_);
            //attrBox_->adjust(attrFont_);
            Q_EMIT sizeHintChangedGlobal();
        }
    }

    //Update the settings handled by the base class
    //updateBaseSettings();
}

QSize TimelineDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QSize size=QStyledItemDelegate::sizeHint(option,index);
    //return QSize(size.width(),nodeBox_->sizeHintCache.height());
    return size;
}


void TimelineDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const
{
    //Background
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QStyleOptionViewItem vopt(option);
#else
    QStyleOptionViewItemV4 vopt(option);
#endif

    initStyleOption(&vopt, index);

    const QStyle *style = vopt.widget ? vopt.widget->style() : QApplication::style();
    const QWidget* widget = vopt.widget;

    //Save painter state
    painter->save();

    //QString id=columns_->id(index.column());

    if(index.column() == 1)
    {
        renderTimeline(painter,option,index.data().toInt());
    }
    //rest of the columns
    else
    {
        QRect bgRect=option.rect;
        painter->fillRect(bgRect,QColor(244,244,245));
        painter->setPen(borderPen_);
        painter->drawLine(bgRect.x()+bgRect.width()-1,bgRect.top(),
                          bgRect.x()+bgRect.width()-1,bgRect.bottom());

        QString text=index.data(Qt::DisplayRole).toString();
        QFontMetrics fm(font_);
        //QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt,widget);
        QRect textRect = bgRect.adjusted(2,0,-2,0);
        text=fm.elidedText(text,Qt::ElideMiddle,textRect.width());
        painter->setFont(font_);
        painter->setPen(Qt::black);        
        painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);
    }

    //Render the horizontal border for rows. We only render the top border line.
    //With this technique we miss the bottom border line of the last row!!!
    //QRect fullRect=QRect(0,option.rect.y(),painter->device()->width(),option.rect.height());
    QRect bgRect=option.rect;
    painter->setPen(borderPen_);
    painter->drawLine(bgRect.topLeft(),bgRect.topRight());

    painter->restore();
}


void TimelineDelegate::renderTimeline(QPainter *painter,const QStyleOptionViewItem& option,int row) const
{   
    if(row < 0)
        return;

    TimelineData *data=model_->data();
    if(!data)
        return;

    bool selected=option.state & QStyle::State_Selected;

    int leftEdge=option.rect.x();
    int rightEdge=option.rect.x()+option.rect.width();
    int xpPrev=leftEdge-2;
    int xpNext=rightEdge+2;

    for(size_t i=0; i < data->items()[row].size(); i++)
    {                              
        bool hasRect=false;
        bool hasGrad=false;
        int xpLeft=0,xpRight=0;
        QColor fillCol;

        int xp=(i==0)?(timeToPos(option.rect,data->items()[row].start_[i])):xpNext;

        if(i < data->items()[row].size()-1)
        {
            xpNext=timeToPos(option.rect,data->items()[row].start_[i+1]);
        }
        else
        {
            xpNext=rightEdge+2;
        }

        if(xp >= leftEdge && xp < rightEdge)
        {           
            if(i > 0 && xpPrev < leftEdge)
            {
                if(VNState* vn=VNState::find(data->items()[row].status_[i-1]))
                {
                    hasRect=true;
                    hasGrad=true;
                    xpLeft=leftEdge;
                    xpRight=xp;
                    fillCol=vn->colour();
                }
            }

            if(VNState* vn=VNState::find(data->items()[row].status_[i]))
            {
                hasRect=true;
                hasGrad=true;
                xpLeft=xp;
                xpRight=(xpNext <= rightEdge)?xpNext:rightEdge;
                fillCol=vn->colour();
            }
        }
        else if(i > 0 && xp >= rightEdge && xpPrev < leftEdge)
        {
            if(VNState* vn=VNState::find(data->items()[row].status_[i-1]))
            {
                hasRect=true;
                xpLeft=leftEdge;
                xpRight=rightEdge;
                fillCol=vn->colour();
            }
        }
        else if(xp <= leftEdge && i == data->items()[row].size()-1)
        {
            if(VNState* vn=VNState::find(data->items()[row].status_[i]))
            {
                hasRect=true;
                xpLeft=leftEdge;
                xpRight=rightEdge;
                fillCol=vn->colour();
            }
        }

        if(hasRect)
        {
            QBrush fillBrush(fillCol);
            if(hasGrad)
            {
                QLinearGradient gr;
                gr.setCoordinateMode(QGradient::ObjectBoundingMode);
                gr.setStart(0,0);
                gr.setFinalStop(1,0);
                fillCol.dark(110);
                gr.setColorAt(0,fillCol);
                fillCol.setAlpha(128);
                gr.setColorAt(1,fillCol);
                fillBrush=QBrush(gr);
            }
            painter->fillRect(QRect(xpLeft,option.rect.y(),
                   xpRight-xpLeft+1,option.rect.height()-1),fillBrush);
        }


        if(xp >= rightEdge)
            break;

        xpPrev=xp;

    }

    //The initial filled rect (we will adjust its  width)
    //QRect itemRect=option.rect.adjusted(nodeBox_->leftMargin,nodeBox_->topMargin,0,-nodeBox_->bottomMargin);
}

int TimelineDelegate::timeToPos(QRect r,unsigned int time) const
{
    unsigned int start=startDate_.toMSecsSinceEpoch()/1000;
    unsigned int end=endDate_.toMSecsSinceEpoch()/1000;

    if(time < start)
        return r.x()-2;

    if(time >= end)
        return r.x()+r.width()+2;

    if(start >= end)
        return r.x()-2;

    return r.x()+static_cast<float>(time-start)*static_cast<float>(r.width())/static_cast<float>((end-start));

}

void TimelineDelegate::setStartDate(QDateTime t)
{
    startDate_=t;
}

void TimelineDelegate::setEndDate(QDateTime t)
{
    endDate_=t;
}

void TimelineDelegate::setPeriod(QDateTime t1,QDateTime t2)
{
    startDate_=t1;
    endDate_=t2;
}

//======================================================================
//
// TimelineView
//
//======================================================================

TimelineView::TimelineView(TimelineSortModel* model,QWidget* parent) :
     QTreeView(parent),
     model_(model),
     needItemsLayout_(false),
     prop_(NULL),
     setCurrentIsRunning_(false)
{
    setObjectName("view");
    //setProperty("style","nodeView");
    //setProperty("view","table");
    setProperty("log","1");
    setRootIsDecorated(false);

    //We enable sorting but do not want to perform it immediately
    //setSortingEnabledNoExec(true);

    setSortingEnabled(false);
    //sortByColumn(-1,Qt::AscendingOrder);

    setAllColumnsShowFocus(true);
    setUniformRowHeights(true);
    setAlternatingRowColors(false);
    setMouseTracking(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    //!!!!We need to do it because:
    //The background colour between the views left border and the nodes cannot be
    //controlled by delegates or stylesheets. It always takes the QPalette::Highlight
    //colour from the palette. Here we set this to transparent so that Qt could leave
    //this area empty and we fill it appropriately in our delegate.
    QPalette pal=palette();
    pal.setColor(QPalette::Highlight,QColor(128,128,128,0));
    setPalette(pal);

    //Context menu
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this,SIGNAL(customContextMenuRequested(const QPoint&)),
            this,SLOT(slotContextMenu(const QPoint&)));

    //Selection
    connect(this,SIGNAL(doubleClicked(const QModelIndex&)),
            this,SLOT(slotDoubleClickItem(const QModelIndex)));

   // actionHandler_=new ActionHandler(this,this);

    //expandAll();

    //Header
    header_=new TimelineHeader(this);

    setHeader(header_);

    //Set header ContextMenuPolicy
    //header_->setContextMenuPolicy(Qt::CustomContextMenu);

    //connect(header_,SIGNAL(customContextMenuRequested(const QPoint &)),
    //            this, SLOT(slotHeaderContextMenu(const QPoint &)));

    //connect(header_,SIGNAL(customButtonClicked(QString,QPoint)),
    //        this,SIGNAL(headerButtonClicked(QString,QPoint)));

    connect(header_,SIGNAL(periodSelected(QDateTime,QDateTime)),
            this,SLOT(periodSelectedInHeader(QDateTime,QDateTime)));

    connect(header_,SIGNAL(periodBeingZoomed(QDateTime,QDateTime)),
            this, SIGNAL(periodBeingZoomed(QDateTime,QDateTime)));

    //for(int i=0; i < model_->columnCount(QModelIndex())-1; i++)
    //  	resizeColumnToContents(i);

    /*connect(header(),SIGNAL(sectionMoved(int,int,int)),
                this, SLOT(slotMessageTreeColumnMoved(int,int,int)));*/

    QTreeView::setModel(model_);

    //Create delegate to the view
    delegate_=new TimelineDelegate(model_->tlModel(),this);
    setItemDelegate(delegate_);

    connect(delegate_,SIGNAL(sizeHintChangedGlobal()),
            this,SLOT(slotSizeHintChangedGlobal()));

    //Properties
    std::vector<std::string> propVec;
    propVec.push_back("view.table.background");
    prop_=new PropertyMapper(propVec,this);

    //Initialise bg
    adjustBackground(prop_->find("view.table.background")->value().value<QColor>());

    //header_->setSortIndicatorShown(true);
}

TimelineView::~TimelineView()
{
    delete prop_;
}

//Enable sorting without actually performing it!!!
void TimelineView::setSortingEnabledNoExec(bool b)
{
#if 0
    if(b)
    {
        model_->setSkipSort(true);
        setSortingEnabled(true);
        model_->setSkipSort(false);
    }
    else
    {
        setSortingEnabled(false);
    }
#endif
}

//Collects the selected list of indexes
QModelIndexList TimelineView::selectedList()
{
    QModelIndexList lst;
    Q_FOREACH(QModelIndex idx,selectedIndexes())
        if(idx.column() == 0)
            lst << idx;
    return lst;
}


// reimplement virtual function from QTreeView - called when the selection is changed
void TimelineView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QTreeView::selectionChanged(selected, deselected);


#if 0
    QModelIndexList lst=selectedIndexes();
    if(lst.count() > 0)
    {
        VInfo_ptr info=model_->nodeInfo(lst.front());
        if(info && !info->isEmpty())
        {
#ifdef _UI_TimelineView_DEBUG
            UiLog().dbg() << "TimelineView::selectionChanged --> emit=" << info->path();
#endif
            Q_EMIT selectionChanged(info);
        }
    }
    QTreeView::selectionChanged(selected, deselected);
#endif
}

VInfo_ptr TimelineView::currentSelection()
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

void TimelineView::setCurrentSelection(VInfo_ptr info)
{
#if 0
    //While the current is being selected we do not allow
    //another setCurrent call go through
    if(setCurrentIsRunning_)
        return;

    setCurrentIsRunning_=true;
    QModelIndex idx=model_->infoToIndex(info);
    if(idx.isValid())
    {
#ifdef _UI_TimelineView_DEBUG
        if(info)
            UiLog().dbg() << "TimelineView::setCurrentSelection --> " <<  info->path();
#endif
        setCurrentIndex(idx);
    }
    setCurrentIsRunning_=false;
#endif
}

void TimelineView::slotDoubleClickItem(const QModelIndex& idx)
{
    showDetails(idx);
}

void TimelineView::slotContextMenu(const QPoint &position)
{
    QModelIndexList lst=selectedList();
    //QModelIndex index=indexAt(position);
    QPoint scrollOffset(horizontalScrollBar()->value(),verticalScrollBar()->value());
    handleContextMenu(indexAt(position),lst,mapToGlobal(position),position+scrollOffset,this);
}


void TimelineView::handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget)
{
    if(!indexClicked.isValid())
        return;

    QMenu *menu=new QMenu(this);

    QAction *ac=new QAction(this);
    ac->setText(tr("Show details"));
    QFont fBold;
    fBold.setBold(true);
    ac->setFont(fBold); //default action with double click
    ac->setData("details");
    menu->addAction(ac);

    ac=new QAction(this);
    ac->setSeparator(true);
    menu->addAction(ac);

    ac=new QAction(this);
    ac->setText(tr("Lookup in tree"));
    ac->setData("lookup");
    menu->addAction(ac);

    ac=new QAction(this);
    ac->setText(tr("Copy node path"));
    ac->setData("copy");
    menu->addAction(ac);

    //Show the context menu and check selected action
    ac=menu->exec(globalPos);
    if(ac && ac->isEnabled() && !ac->isSeparator())
    {
        if(ac->data().toString() == "details")
        {
            showDetails(indexClicked);
        }
        else if(ac->data().toString() == "lookup")
        {
            lookup(indexClicked);
        }
        else if(ac->data().toString() == "copy")
        {
            copyPath(indexClicked);
        }
    }

    delete menu;
}

void TimelineView::showDetails(const QModelIndex& indexClicked)
{
    QModelIndex idx=model_->mapToSource(indexClicked);
    if(!idx.isValid())
        return;

    TimelineInfoDialog diag(this);
    diag.infoW_->load("host","port",model_->tlModel()->data(),idx.row(),
                      header_->startDate(),header_->endDate());

    diag.exec();
}

void TimelineView::lookup(const QModelIndex &indexClicked)
{
    QModelIndex idx=model_->mapToSource(indexClicked);
    if(!idx.isValid())
        return;

    QString nodePath=QString::fromStdString(model_->tlModel()->data()->items()[idx.row()].path());
    Q_EMIT(lookupRequested(nodePath));
}

void TimelineView::copyPath(const QModelIndex &indexClicked)
{
    QModelIndex idx=model_->mapToSource(indexClicked);
    if(!idx.isValid())
        return;

    QString nodePath=QString::fromStdString(model_->tlModel()->data()->items()[idx.row()].path());
    Q_EMIT(copyPathRequested(nodePath));
}

void TimelineView::slotViewCommand(VInfo_ptr info,QString cmd)
{
}

void TimelineView::rerender()
{
    if(needItemsLayout_)
    {
        doItemsLayout();
        needItemsLayout_=false;
    }
    else
    {
        viewport()->update();
    }
}

void TimelineView::slotRerender()
{
    rerender();
}

void TimelineView::slotSizeHintChangedGlobal()
{
    needItemsLayout_=true;
}

void TimelineView::adjustBackground(QColor col)
{
    if(col.isValid())
    {
        QString sh="QTreeView { background : " + col.name() + ";}";
        setStyleSheet(sh);
    }
}

void TimelineView::notifyChange(VProperty* p)
{
    if(p->path() == "view.table.background")
    {
        adjustBackground(p->value().value<QColor>());
    }
}

void TimelineView::setZoomActions(QAction* zoomInAction,QAction* zoomOutAction)
{
    header_->setZoomActions(zoomInAction,zoomOutAction);
}

void TimelineView::periodSelectedInHeader(QDateTime t1,QDateTime t2)
{
    delegate_->setPeriod(t1,t2);
    rerender();
    Q_EMIT periodSelected(t1,t2);
}

void TimelineView::setStartDate(QDateTime t)
{
    delegate_->setStartDate(t);
    header_->setStartDate(t);
    rerender();
    UiLog().dbg() << "startdate" << t;
}

void TimelineView::setEndDate(QDateTime t)
{
    delegate_->setEndDate(t);
    header_->setEndDate(t);
    rerender();
}

void TimelineView::setPeriod(QDateTime t1,QDateTime t2)
{
    delegate_->setPeriod(t1,t2);
    header_->setPeriod(t1,t2);
    rerender();
}

//=========================================
// Header
//=========================================

void TimelineView::slotHeaderContextMenu(const QPoint &position)
{
}

void TimelineView::readSettings(VSettings* vs)
{
    vs->beginGroup("view");
    std::vector<int> wVec;
    vs->get("width",wVec);
    vs->endGroup();

    for(size_t i=0; i < wVec.size() && i < static_cast<size_t>(model_->columnCount()); i++)
    {
        if(wVec[i] > 0)
            setColumnWidth(i,wVec[i]);
    }
}

void TimelineView::writeSettings(VSettings* vs)
{   
    vs->beginGroup("view");

    std::vector<int> wVec;
    for(int i=0; i < model_->columnCount(QModelIndex()); i++)
    {      
        wVec.push_back(columnWidth(i));
    }

    vs->put("width",wVec);

    vs->endGroup();

}

//=========================================
// TimelineHeader
//=========================================

TimelineHeader::TimelineHeader(QWidget *parent) :
    QHeaderView(Qt::Horizontal, parent),
    fm_(QFont()),  
    timelineCol_(50,50,50),
    dateTextCol_(33,95,161),
    timeTextCol_(30,30,30),
    timelineFrameBorderCol_(150,150,150),
    zoomCol_(224,236,248,190),
    inZoom_(false),
    timelineSection_(1),
    timelineFrameSize_(4),
    majorTickSize_(5),
    zoomInAction_(0),
    zoomOutAction_(0)
{
    setMouseTracking(true);

    setStretchLastSection(true);
    connect(this, SIGNAL(sectionResized(int, int, int)),
             this, SLOT(slotSectionResized(int)));

    QColor bg0(214,214,214);
    QColor bg1(194,194,194);
    QLinearGradient gr;
    gr.setCoordinateMode(QGradient::ObjectBoundingMode);
    gr.setStart(0,0);
    gr.setFinalStop(0,1);
    gr.setColorAt(0,bg0);
    gr.setColorAt(1,bg1);
    timelineBrush_=QBrush(gr);

    int pixId=IconProvider::add(":viewer/filter_decor.svg","filter_decor");

    customPix_=IconProvider::pixmap(pixId,10);

    font_= QFont();
    font_.setPointSize(font_.pointSize()-2);
    fm_=QFontMetrics(font_);

    zoomCursor_=QCursor(QPixmap(":/viewer/cursor_zoom.svg"));
}


QSize TimelineHeader::sizeHint() const
{
    //return QHeaderView::sizeHint();

    QSize s = QHeaderView::sizeHint(); //size();
    //s.setHeight(headerSections[0]->minimumSizeHint().height() + 35);
    //s.setHeight(2*35);
    s.setHeight(timelineFrameSize_ + fm_.height() + 6 + majorTickSize_ + fm_.height() + 6 + timelineFrameSize_);
    return s;
}

void TimelineHeader::showEvent(QShowEvent *e)
{
  /*  for(int i=0;i<count();i++)
    {


       if(1)
       {
           widgets_[i]->setGeometry(sectionViewportPosition(i),0,
                                sectionSize(i),height());
           widgets_[i]->shosetMouseTracking(true);w();
       }
    }
    */

    QHeaderView::showEvent(e);
}

void TimelineHeader::mousePressEvent(QMouseEvent *event)
{
    //Start new zoom
    if(isZoomEnabled() &&
       !inZoom_ && event->button() == Qt::LeftButton &&
       logicalIndexAt(event->pos()) == timelineSection_ &&
       canBeZoomed())
    {
        zoomStartPos_=event->pos();
    }
#if 0
    //Unzoom
    else if(!inZoom_ && event->button() == Qt::RightButton &&
            logicalIndexAt(event->pos()) == timelineSection_)
    {
        if(zoomHistory_.count() >= 2)
        {
            zoomHistory_.pop();
            QDateTime sDt=zoomHistory_.top().first;
            QDateTime eDt=zoomHistory_.top().second;
            if(sDt.isValid() && eDt.isValid())
            {
                setPeriodCore(sDt,eDt,false);
                Q_EMIT periodSelected(sDt,eDt);
            }
        }
    }
 #endif
    else
        QHeaderView::mousePressEvent(event);
}

void TimelineHeader::mouseMoveEvent(QMouseEvent *event)
{           
    //When we enter the timeline section we show a zoom cursor
    bool hasCursor=testAttribute(Qt::WA_SetCursor);

    if(!isZoomEnabled())
    {
        QHeaderView::mouseMoveEvent(event);
        return;
    }

    //When enabled ...

    if(event->buttons().testFlag(Qt::LeftButton))
    {
        int secStart=sectionPosition(timelineSection_);
        int secEnd=secStart+sectionSize(timelineSection_);

        //If we are in resize mode
        if(!inZoom_ && zoomStartPos_.isNull())
        {
            QHeaderView::mouseMoveEvent(event);
            return;
        }

        //In timeline zoom mode we show a zoom cursor
        if(!hasCursor || cursor().shape() ==  Qt::SplitHCursor )
            setCursor(zoomCursor_);

        inZoom_=true;
        zoomEndPos_=event->pos();

        if(event->pos().x() >= secStart && event->pos().x() <= secEnd)
        {
            if(event->pos().x() < zoomStartPos_.x())
            {
                zoomEndPos_=zoomStartPos_;
            }
        }
        else if(event->pos().x() < secStart)
        {
            zoomEndPos_=zoomStartPos_;
        }
        else //if(event->pos().x() > secEnd)
        {
            zoomEndPos_=event->pos();
            zoomEndPos_.setX(secEnd);
        }

        headerDataChanged(Qt::Horizontal,timelineSection_,timelineSection_);

        QDateTime sDt=posToDate(zoomStartPos_);
        QDateTime eDt=posToDate(zoomEndPos_);
        if(sDt.isValid() && eDt.isValid())
        {
            Q_EMIT periodBeingZoomed(sDt,eDt);
        }
    }
    else
    {
        //When we enter the timeline section we show a zoom cursor
        if(logicalIndexAt(event->pos()) == timelineSection_)
        {
            if((!hasCursor || cursor().shape() ==  Qt::SplitHCursor) &&
               (canBeZoomed()))
                setCursor(zoomCursor_);
        }
        //Otherwise remove the cursor unless it is the resize indicator
        else
        {
            if(hasCursor && cursor().shape() !=  Qt::SplitHCursor)
                unsetCursor();

            QHeaderView::mouseMoveEvent(event);
        }
    }
}

void TimelineHeader::mouseReleaseEvent(QMouseEvent *event)
{
    if(inZoom_)
    {        
        QDateTime sDt=posToDate(zoomStartPos_);
        QDateTime eDt=posToDate(zoomEndPos_);
        zoomStartPos_=QPoint();
        zoomEndPos_=QPoint();
        inZoom_=false;
        if(sDt.isValid() && eDt.isValid())
        {
            setPeriodCore(sDt,eDt,true);
            Q_EMIT periodSelected(sDt,eDt);
        }
        else
        {
            Q_EMIT periodBeingZoomed(startDate_,endDate_);
        }

        setZoomDisabled();

        //bool hasCursor=testAttribute(Qt::WA_SetCursor);
        //if(hasCursor && cursor().shape() !=  Qt::SplitHCursor)
        //    unsetCursor();
    }
    else
    {
        QHeaderView::mouseReleaseEvent(event);
    }
}

void TimelineHeader::slotSectionResized(int i)
{
    /*for (int j=visualIndex(i);j<count();j++)
    {
        int logical = logicalIndex(j);

        if(combo_[logical])
        {
            combo_[logical]->setGeometry(sectionViewportPosition(logical), height()/2,
                                   sectionSize(logical) - 16, height());
        }
   }*/
}

void TimelineHeader::setModel(QAbstractItemModel *model)
{
#if 0
    if(model)
    {
        for(int i=0; i< model->columnCount(); i++)
        {
            QString id=model->headerData(i,Qt::Horizontal,Qt::UserRole).toString();
            if(id == "status")
                customButton_.insert(i,TimelineHeaderButton(id));
        }
    }
#endif
    QHeaderView::setModel(model);
}

bool TimelineHeader::canBeZoomed() const
{
    return (endDate_.toMSecsSinceEpoch()-startDate_.toMSecsSinceEpoch()) > 60*1000;
}

void TimelineHeader::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    painter->save();
    //QHeaderView::paintSection(painter, rect, logicalIndex);
    //painter->restore();


    /*QPixmap customPix(":viewer/filter_decor.svg");
    QRect cbRect(0,0,12,12);
    cbRect.moveCenter(QPoint(rect.right()-16-6,rect.center().y()));
    customButton_[logicalIndex].setRect(cbRect);
    painter->drawPixmap(cbRect,pix);*/

    if (!rect.isValid())
        return;

     QStyleOptionHeader opt;
     initStyleOption(&opt);
     QStyle::State state = QStyle::State_None;
     if(isEnabled())
        state |= QStyle::State_Enabled;
     if(window()->isActiveWindow())
        state |= QStyle::State_Active;

    bool clickable;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    clickable=sectionsClickable();
#else
    clickable=isClickable();
#endif

     if(clickable)
     {
         /*if (logicalIndex == d->hover)
                state |= QStyle::State_MouseOver;
            if (logicalIndex == d->pressed)
                state |= QStyle::State_Sunken;
            else if (d->highlightSelected) {
                if (d->sectionIntersectsSelection(logicalIndex))
                    state |= QStyle::State_On;
                if (d->isSectionSelected(logicalIndex))
                    state |= QStyle::State_Sunken;
            }*/

        }

    // if(isSortIndicatorShown() && sortIndicatorSection() == logicalIndex)
    //        opt.sortIndicator = (sortIndicatorOrder() == Qt::AscendingOrder)
    //                            ? QStyleOptionHeader::SortDown : QStyleOptionHeader::SortUp;

     // setup the style options structure
     //QVariant textAlignment = model->headerData(logicalIndex, d->orientation,
     //                                                 Qt::TextAlignmentRole);
     opt.rect = rect;
     opt.section = logicalIndex;
     opt.state |= state;
     //opt.textAlignment = Qt::Alignment(textAlignment.isValid()
     //                                     ? Qt::Alignment(textAlignment.toInt())
     //                                     : d->defaultAlignment);

     //opt.text = model()->headerData(logicalIndex, Qt::Horizontal),
     //                                    Qt::DisplayRole).toString();

     QVariant foregroundBrush;
     if (foregroundBrush.canConvert<QBrush>())
         opt.palette.setBrush(QPalette::ButtonText, qvariant_cast<QBrush>(foregroundBrush));

     QPointF oldBO = painter->brushOrigin();
     QVariant backgroundBrush;
     if (backgroundBrush.canConvert<QBrush>())
     {
            opt.palette.setBrush(QPalette::Button, qvariant_cast<QBrush>(backgroundBrush));
            opt.palette.setBrush(QPalette::Window, qvariant_cast<QBrush>(backgroundBrush));
            painter->setBrushOrigin(opt.rect.topLeft());
     }

    // the section position
    int visual = visualIndex(logicalIndex);
    assert(visual != -1);

    if (count() == 1)
        opt.position = QStyleOptionHeader::OnlyOneSection;
    else if (visual == 0)
        opt.position = QStyleOptionHeader::Beginning;
    else if (visual == count() - 1)
        opt.position = QStyleOptionHeader::End;
    else
        opt.position = QStyleOptionHeader::Middle;

    opt.orientation = Qt::Horizontal;

    // the selected position
    /*bool previousSelected = d->isSectionSelected(logicalIndex(visual - 1));
        bool nextSelected =  d->isSectionSelected(logicalIndex(visual + 1));
        if (previousSelected && nextSelected)
            opt.selectedPosition = QStyleOptionHeader::NextAndPreviousAreSelected;
        else if (previousSelected)
            opt.selectedPosition = QStyleOptionHeader::PreviousIsSelected;
        else if (nextSelected)
            opt.selectedPosition = QStyleOptionHeader::NextIsSelected;
        else
            opt.selectedPosition = QStyleOptionHeader::NotAdjacent;
    */

    // draw the section
    style()->drawControl(QStyle::CE_Header, &opt, painter, this);
    painter->setBrushOrigin(oldBO);

    painter->restore();


    int rightPos=rect.right();
    if(isSortIndicatorShown() && sortIndicatorSection() == logicalIndex)
        opt.sortIndicator = (sortIndicatorOrder() == Qt::AscendingOrder)
                            ? QStyleOptionHeader::SortDown : QStyleOptionHeader::SortUp;

    if (opt.sortIndicator != QStyleOptionHeader::None)
        {
            QStyleOptionHeader subopt = opt;
            subopt.rect = style()->subElementRect(QStyle::SE_HeaderArrow, &opt, this);
            rightPos=subopt.rect.left();
            style()->drawPrimitive(QStyle::PE_IndicatorHeaderArrow, &subopt, painter, this);
         }

#if 0
    QMap<int,TimelineHeaderButton>::iterator it=customButton_.find(logicalIndex);
    if(it != customButton_.end())
    {
        //Custom button
        QStyleOptionButton optButton;

        //visPbOpt.text="Visualise";
        optButton.state = QStyle::State_AutoRaise ; //QStyle::State_Active | QStyle::State_Enabled;
        //optButton.icon=customIcon_;
        //optButton.iconSize=QSize(12,12);

        int buttonWidth=customPix_.width();
        int buttonHeight=buttonWidth;
        optButton.rect = QRect(rightPos-4-buttonWidth,(rect.height()-buttonWidth)/2,
                                   buttonWidth,buttonHeight);

        painter->drawPixmap(optButton.rect,customPix_);

        rightPos=optButton.rect.left();
        it.value().setRect(optButton.rect);
    }
#endif

    QString text=model()->headerData(logicalIndex,Qt::Horizontal).toString();
    QRect textRect=rect;
    textRect.setRight(rightPos-5);

    if(logicalIndex != timelineSection_)
    {
       painter->drawText(textRect,Qt::AlignHCenter | Qt::AlignVCenter,text);
       return;
    }

    renderTimeline(rect,painter);
}

void TimelineHeader::renderTimeline(const QRect& rect,QPainter* painter) const
{
    //painter->fillRect(rect.adjusted(0,0,0,-1),timelineFrameBgCol_);

    //The timeline area bounded by the frame
    QRect pRect=rect.adjusted(0,timelineFrameSize_,0,-timelineFrameSize_);

    //Special appearance in for the timeline area
    painter->fillRect(pRect,timelineBrush_);

    painter->setPen(QPen(timelineFrameBorderCol_));
    painter->drawLine(QPoint(rect.left(),pRect.top()),
                      QPoint(rect.right(),pRect.top()));

    painter->drawLine(QPoint(rect.left(),pRect.bottom()),
                      QPoint(rect.right(),pRect.bottom()));

    if(inZoom_)
    {
        QRect zRect=pRect; //rect.adjusted(0,zoomFrameTop,0,-zoomFrameBottom-1);
        zRect.setLeft(zoomStartPos_.x());
        zRect.setRight(zoomEndPos_.x());
        painter->fillRect(zRect,zoomCol_);
        painter->setPen(zoomCol_.darker(140));
        painter->drawRect(zRect.adjusted(0,1,0,-2));
    }

    int w=sectionSize(timelineSection_);

    //period in secs
    qint64 startSec=startDate_.toMSecsSinceEpoch()/1000;
    qint64 endSec=endDate_.toMSecsSinceEpoch()/1000;
    qint64 period=endSec-startSec;

    int minorTick=1; //in secs (it is a delta)
    int majorTick=1;  //in secs (it is a delta)
    qint64 firstTick=1; //in secs since epoch

    int hLineY=pRect.center().y()-majorTickSize_/2-1;
    int timeTextGap=3; //the gap between the top of the time text and the bottom of the major tick in pixels
    int majorTickTop=hLineY;
    int majorTickBottom=hLineY+majorTickSize_; //pRect.bottom()-fm_.height()-timeTextGap;
    int minorTickTop=hLineY;
    int minorTickBottom=majorTickBottom-3;
    int dateTextY= hLineY - (hLineY-pRect.y() - fm_.height())/2 - fm_.height() + 1;
    int timeTextY= majorTickBottom + (pRect.bottom()-majorTickBottom - fm_.height())/2 - 1;

    int timeItemW=fm_.width("223:442");
    int dateItemW=fm_.width("2229 May22");

    QList<int> majorTickSec;

    if(period < 60)
    {
        majorTickSec << 1 << 5 << 10 << 20 << 30;
    }
    else if(period < 600)
    {
        majorTickSec << 10 << 15 << 20 << 30 << 60 << 5*60 << 10*60;
    }
    if(period < 3600)
    {
        majorTickSec << 60 << 2*60 << 3*60 << 4*60 << 5*60 << 10*60 << 20*60 << 30*60;
    }
    if(period < 12*3600)
    {
        majorTickSec << 15*60 << 30*60 << 3600 << 2*3600 << 3*3600 << 4*3600 << 6*3600;
    }
    else if(period <= 86400)
    {
        majorTickSec << 3600 << 2*3600 << 3*3600 << 4*3600 << 6*3600 << 12*3600;
    }
    else if(period < 7*86400)
    {
        majorTickSec << 3600 << 2*3600 << 3*3600 << 4*3600 << 6*3600 << 12*3600 << 24*3600;
    }
    else if(period < 14*86400)
    {
        majorTickSec << 12*3600 << 24*3600 << 36*3600 << 48*3600 << 72*3600 << 96*3600;
    }
    else if(period < 28*86400)
    {
        majorTickSec << 86400 << 2*86400 << 3*86400 << 4*86400 << 5*86400 << 10*86400;
    }
    else if(period < 60*86400)
    {
        majorTickSec << 86400 << 2*86400 << 3*86400 << 4*86400 << 5*86400 << 10*86400 << 20*86400;
    }
    else if(365 * 86400)
    {
        majorTickSec << 5*86400 << 10*86400 << 20*86400 << 30*86400 << 60*86400 << 90*86400 << 180*86400;
    }
    else
    {
        majorTickSec << 30*86400 << 60*86400 << 90*86400 << 180*86400 << 365*86400;
    }

    Q_FOREACH(int mts,majorTickSec)
    {
       majorTick=mts;
       int majorTickNum=period/majorTick;
       int cover=timeItemW*majorTickNum;
       int diff=w-cover;
       if(diff > 100)
       {
           break;
       }
    }

    minorTick=majorTick/4;
    if(minorTick==0)
        minorTick = majorTick;

    firstTick=(startSec/minorTick)*minorTick+minorTick;

    //Find label positions for days
    QList<QPair<int,QString> > dateLabels;
    int dayNum=startDate_.date().daysTo(endDate_.date());

    if(dayNum == 0)
    {
        int xp=secToPos((startSec+endSec)/2-startSec,rect);
        dateLabels << qMakePair(xp,startDate_.toString("dd MMM"));
    }
    else
    {
        QDate  nextDay=startDate_.date().addDays(1);
        QDate  lastDay=endDate_.date();
        qint64 nextSec=QDateTime(nextDay).toMSecsSinceEpoch()/1000;

        if((nextSec-startSec) < 3600)
        {
            int xp=secToPos((startSec+nextSec)/2-startSec,rect);
            dateLabels << qMakePair(xp,nextDay.toString("dd MMM"));
        }

        int dayFreq=1;
        QList<int> dayFreqLst;
        dayFreqLst << 1 << 2 << 3 << 5 << 10 << 20 << 30 << 60 << 90 << 120 << 180 << 365;
        Q_FOREACH(int dfv,dayFreqLst)
        {
            if((dayNum/dfv)*dateItemW < w-100)
            {
                dayFreq=dfv;
                break;
            }
        }

        QDate firstDay=nextDay;
        for(QDate d=firstDay; d < lastDay; d=d.addDays(dayFreq))
        {
            int xp=secToPos((QDateTime(d).toMSecsSinceEpoch()/1000 +
            QDateTime(d.addDays(1)).toMSecsSinceEpoch()/1000)/2-startSec,rect);
            dateLabels << qMakePair(xp,d.toString("dd MMM"));
        }

        if(QDateTime(lastDay).toMSecsSinceEpoch()/1000 < endSec)
        {
            int xp=secToPos((QDateTime(lastDay).toMSecsSinceEpoch()/1000+endSec)/2 - startSec,rect);
            dateLabels << qMakePair(xp,lastDay.toString("dd MMM"));
        }
    }

    painter->save();
    painter->setClipRect(rect);


    //Draw date labels
    painter->setPen(dateTextCol_);
    for(int i=0; i < dateLabels.count(); i++)
    {
        int xp=dateLabels[i].first;
        int textW=fm_.width(dateLabels[i].second);
        //int yp=rect.bottom()-1-fm.height();
        painter->setFont(font_);
        painter->drawText(QRect(xp-textW/2, dateTextY,textW,fm_.height()),
                          Qt::AlignHCenter | Qt::AlignVCenter,dateLabels[i].second);
    }

    //horizontal line
    painter->setPen(timelineCol_);
    painter->drawLine(rect.x(),hLineY,rect.right(),hLineY);

    qint64 actSec=firstTick;
    Q_ASSERT(actSec >= startSec);
    painter->setPen(timeTextCol_);

    while(actSec <= endSec)
    {
        int xp=secToPos(actSec-startSec,rect);

        //draw major tick + label
        if(actSec % majorTick == 0)
        {
            painter->drawLine(xp,majorTickTop,xp,majorTickBottom);

            QString s;
            if(majorTick < 60)
            {
                s=QDateTime::fromMSecsSinceEpoch(actSec*1000,Qt::UTC).toString("H:mm:ss");
            }
            else
            {
                s=QDateTime::fromMSecsSinceEpoch(actSec*1000,Qt::UTC).toString("H:mm");
            }

            int textW=fm_.width(s);
            painter->setFont(font_);            
            painter->drawText(QRect(xp-textW/2, timeTextY,textW,fm_.height()),
                              Qt::AlignHCenter | Qt::AlignVCenter,s);
        }
        //draw minor tick
        else
        {
            painter->drawLine(xp,minorTickTop,xp,minorTickBottom);
        }

        actSec+=minorTick;
    }

    painter->restore();

    //for(int i=0; i < 10; i++)
    //{
    //    int xp=i*rect.width()/10;
    //    painter->drawLine(xp,rect.top(),xp,rect.bottom());
    //}

    //style()->drawControl(QStyle::CE_PushButton, &optButton,painter,this);
}

int TimelineHeader::secToPos(qint64 t,QRect rect) const
{
    //qint64 sd=startDate_.toMSecsSinceEpoch()/1000;
    qint64 period=(endDate_.toMSecsSinceEpoch()-startDate_.toMSecsSinceEpoch())/1000;
    return rect.x() + static_cast<int>(static_cast<float>(t)/static_cast<float>(period)*static_cast<float>(rect.width()));
}

int TimelineHeader::dateToPos(QDateTime dt) const
{
    qint64 period=(endDate_.toMSecsSinceEpoch()-startDate_.toMSecsSinceEpoch())/1000;

    int xp=sectionPosition(timelineSection_);
    int w=sectionSize(timelineSection_);

    return xp + static_cast<int>(static_cast<float>(dt.toMSecsSinceEpoch()/1000)/static_cast<float>(period)*static_cast<float>(w));
}

QDateTime TimelineHeader::posToDate(QPoint pos) const
{
    int xp=sectionPosition(timelineSection_);
    int w=sectionSize(timelineSection_);

    if(w <= 0 || pos.x() < xp)
        return QDateTime();

    float r=static_cast<float>(pos.x()-xp)/static_cast<float>(w);
    if(r < 0 || r > 1)
        return QDateTime();

    //qint64 sd=startDate_.toMSecsSinceEpoch()/1000;
    qint64 period=(endDate_.toMSecsSinceEpoch()-startDate_.toMSecsSinceEpoch());

    return startDate_.addMSecs(r*period);
}

qint64 TimelineHeader::zoomPeriodInSec(QPoint startPos,QPoint endPos) const
{
    if(endPos.x() < startPos.x())
        return false;

    int xp=sectionPosition(timelineSection_);
    int w=sectionSize(timelineSection_);

    if(w <= 0 || startPos.x() < xp)
        return false;

    float r=static_cast<float>(endPos.x()-startPos.x())/static_cast<float>(w);
    if(r < 0 || r > 1)
        return false;

    qint64 period=(endDate_.toMSecsSinceEpoch()-startDate_.toMSecsSinceEpoch());

    return r*period;
}

#if 0
void TimelineHeader::mousePressEvent(QMouseEvent *event)
{
  #if 0

    QMap<int,TimelineHeaderButton>::const_iterator it = customButton_.constBegin();
    while(it != customButton_.constEnd())
    {
        if(it.value().rect_.contains(event->pos()))
        {
            UiLog().dbg() << "header " << it.key() << " clicked";
            Q_EMIT customButtonClicked(it.value().id(),event->globalPos());
        }
         ++it;
     }
#endif
    QHeaderView::mousePressEvent(event);
}
#endif

/*void TimelineHeader::mouseMoveEvent(QMouseEvent *event)
{
    int prevIndex=hoverIndex_;
    QMap<int,TimelineHeaderButton>::const_iterator it = customButton_.constBegin();
    while(it != customButton_.constEnd())
    {
        if(it.value().rect_.contains(event->pos()))
        {
            hoverIndex_=it.key();
            if(hoveIndex != prevIndex)
            {
                rerender;
            }
        }
        ++it;
    }

    if(preIndex !=-1)
    {

    }
    hoverIndex_=-1;
}*/

void TimelineHeader::setZoomActions(QAction* zoomInAction,QAction* zoomOutAction)
{
    if(!zoomInAction_)
    {
        zoomInAction_=zoomInAction;
        zoomOutAction_=zoomOutAction;

        connect(zoomInAction_,SIGNAL(toggled(bool)),
                this,SLOT(slotZoomState(bool)));

        connect(zoomOutAction_,SIGNAL(triggered(bool)),
                this,SLOT(slotZoomOut(bool)));

        checkActionState();
    }
}

bool TimelineHeader::isZoomEnabled() const
{
    return (zoomInAction_ && zoomInAction_->isEnabled())?(zoomInAction_->isChecked()):false;
}

void TimelineHeader::setZoomDisabled()
{
    if(zoomInAction_)
        zoomInAction_->setChecked(false);
}

void TimelineHeader::slotZoomState(bool)
{
    Q_ASSERT(zoomInAction_);
    if(!zoomInAction_->isChecked())
    {
        bool hasCursor=testAttribute(Qt::WA_SetCursor);
        if(hasCursor && cursor().shape() !=  Qt::SplitHCursor)
                unsetCursor();
    }

    headerDataChanged(Qt::Horizontal,0,timelineSection_);
}

void TimelineHeader::slotZoomOut(bool)
{
    if(!inZoom_ && !isZoomEnabled())
    {
        if(zoomHistory_.count() >= 2)
        {
            zoomHistory_.pop();
            QDateTime sDt=zoomHistory_.top().first;
            QDateTime eDt=zoomHistory_.top().second;
            if(sDt.isValid() && eDt.isValid())
            {
                setPeriodCore(sDt,eDt,false);
                Q_EMIT periodSelected(sDt,eDt);
            }

            checkActionState();
        }
    }
}

void TimelineHeader::checkActionState()
{
    if(zoomInAction_)
    {
        zoomOutAction_->setEnabled(canBeZoomed() && zoomHistory_.count() >= 2);
        zoomInAction_->setEnabled(canBeZoomed());
    }
}

void TimelineHeader::setStartDate(QDateTime t)
{
    startDate_=t;
    zoomHistory_.clear();
    zoomHistory_.push(qMakePair<QDateTime,QDateTime>(startDate_,endDate_));
    headerDataChanged(Qt::Horizontal,0,timelineSection_);
    checkActionState();
}

void TimelineHeader::setEndDate(QDateTime t)
{
    endDate_=t;
    zoomHistory_.clear();
    zoomHistory_.push(qMakePair<QDateTime,QDateTime>(startDate_,endDate_));
    headerDataChanged(Qt::Horizontal,0,timelineSection_);
    checkActionState();
}

void TimelineHeader::setPeriod(QDateTime t1,QDateTime t2)
{
    zoomHistory_.clear();
    setPeriodCore(t1,t2,true);
}

void TimelineHeader::setPeriodCore(QDateTime t1,QDateTime t2,bool addToHistory)
{
    startDate_=t1;
    endDate_=t2;
    if(addToHistory)
    {
        zoomHistory_.push(qMakePair<QDateTime,QDateTime>(startDate_,endDate_));
    }
    headerDataChanged(Qt::Horizontal,0,timelineSection_);
    checkActionState();
}
