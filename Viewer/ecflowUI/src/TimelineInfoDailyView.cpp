//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TimelineInfoDailyView.hpp"

#include <QtGlobal>
#include <QBuffer>
#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QPainter>
#include <QSettings>
#include <QToolButton>
#include <QVBoxLayout>

#include "SessionHandler.hpp"
#include "TextFormat.hpp"
#include "TimelineData.hpp"
#include "TimelineHeaderView.hpp"
#include "TimelineModel.hpp"
#include "TimelineView.hpp"
#include "TextFormat.hpp"
#include "UiLog.hpp"
#include "PropertyMapper.hpp"
#include "ViewerUtil.hpp"
#include "VNState.hpp"
#include "WidgetNameProvider.hpp"

#include "ui_TimelineInfoWidget.h"


static std::vector<std::string> propVec;

//=======================================================
//
// TimelineInfoDailyModel
//
//=======================================================

TimelineInfoDailyModel::TimelineInfoDailyModel(QObject *parent) :
          QAbstractItemModel(parent), data_(0)
{
}

TimelineInfoDailyModel::~TimelineInfoDailyModel()
{
}

void TimelineInfoDailyModel::load(TimelineItem *data,unsigned int viewStartDateSec,unsigned int viewEndDateSec,
                                unsigned int endDateSec)
{
    beginResetModel();
    viewStartDateSec_=viewStartDateSec;
    viewEndDateSec_=viewEndDateSec;
    endDateSec_=endDateSec;
    data_=data;
    days_.clear();
    if(data_)
    {
        data_->days(days_);
        unsigned int lastDay = 0;
        if(!days_.empty())
        {
            lastDay = days_.back();
        }
        if(lastDay/86400 != endDateSec_/86400)
        {
            days_.push_back((endDateSec_/86400)*86400);
        }

    }
    endResetModel();
}

void TimelineInfoDailyModel::clearData()
{
    beginResetModel();
    data_=0;
    days_.clear();
    endResetModel();
}

bool TimelineInfoDailyModel::hasData() const
{
    return (data_ != NULL);
}

int TimelineInfoDailyModel::columnCount( const QModelIndex& /*parent */) const
{
     return 2;
}

int TimelineInfoDailyModel::rowCount( const QModelIndex& parent) const
{
    if(!hasData())
        return 0;

    //Parent is the root:
    if(!parent.isValid())
    {
        return static_cast<int>(days_.size());
    }

    return 0;
}

Qt::ItemFlags TimelineInfoDailyModel::flags ( const QModelIndex & index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TimelineInfoDailyModel::data(const QModelIndex& index, int role ) const
{
    if(!index.isValid() || !hasData())
    {
        return QVariant();
    }

    int row=index.row();
    if(row < 0 || row >= static_cast<int>(days_.size()))
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        if(index.column() == 0)
        {
            return TimelineItem::toQDateTime(days_[row]).toString("dd-MMM-yyyy");
        }
        else if(index.column() == 1)
        {
            return days_[row];

        }
    }

    return QVariant();
}

QVariant TimelineInfoDailyModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
    if ( orient != Qt::Horizontal || (role != Qt::DisplayRole && role != Qt::UserRole ))
              return QAbstractItemModel::headerData( section, orient, role );

    if(role == Qt::DisplayRole)
    {
        switch(section)
        {
        case 0:
            return "Date";
        case 1:
            return "Daily cycle";
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QModelIndex TimelineInfoDailyModel::index( int row, int column, const QModelIndex & parent ) const
{
    if(!hasData() || row < 0 || column < 0)
    {
        return QModelIndex();
    }

    //When parent is the root this index refers to a node or server
    if(!parent.isValid())
    {
        return createIndex(row,column);
    }

    return QModelIndex();

}

QModelIndex TimelineInfoDailyModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

//======================================================================
//
// TimelineInfoDailyDelegate
//
//======================================================================

TimelineInfoDailyDelegate::TimelineInfoDailyDelegate(TimelineInfoDailyModel *model,QWidget *parent) :
    QStyledItemDelegate(parent),
    model_(model),
    fm_(QFont()),
    borderPen_(QPen(QColor(216,216,216))),
    topPadding_(2),
    bottomPadding_(2),
    startTime_(0,0,0),
    endTime_(23,0,0)
{
    Q_ASSERT(model_);

    fm_=QFontMetrics(font_);

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

TimelineInfoDailyDelegate::~TimelineInfoDailyDelegate()
{
}

void TimelineInfoDailyDelegate::notifyChange(VProperty* p)
{
    updateSettings();
}

void TimelineInfoDailyDelegate::updateSettings()
{
    if(VProperty* p=prop_->find("view.table.font"))
    {
        QFont newFont=p->value().value<QFont>();

        if(font_ != newFont)
        {
            font_=newFont;
            fm_=QFontMetrics(font_);
            //Q_EMIT sizeHintChangedGlobal();
        }
    }

    //Update the settings handled by the base class
    //updateBaseSettings();
}

QSize TimelineInfoDailyDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QSize size=QStyledItemDelegate::sizeHint(option,index);
    return QSize(size.width(),fm_.height() + topPadding_ + bottomPadding_);
    return size;
}


void TimelineInfoDailyDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,
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

    bool selected=option.state & QStyle::State_Selected;

    //Save painter state
    painter->save();

    if(index.column() == 1)
    {
        renderTimeline(painter,option,index);
    }

    //The path column
    else
    {
        QRect bgRect=option.rect;
        painter->fillRect(bgRect,QColor(244,244,245));
        painter->setPen(borderPen_);
        painter->drawLine(bgRect.x()+bgRect.width()-1,bgRect.top(),
                          bgRect.x()+bgRect.width()-1,bgRect.bottom());

        QString text=index.data(Qt::DisplayRole).toString();
        //QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt,widget);
        QRect textRect = bgRect.adjusted(2,topPadding_,-3,-bottomPadding_);
        text=fm_.elidedText(text,Qt::ElideMiddle,textRect.width());
        textRect.setWidth(fm_.width(text));
        painter->setFont(font_);
        painter->setPen(Qt::black);
        painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

        if(selected)
        {
            QRect sr=textRect;
            sr.setX(option.rect.x()+1);
            sr.setY(option.rect.y()+2);
            sr.setBottom(option.rect.bottom()-2);
            sr.setRight(textRect.x()+textRect.width()+1);
            painter->setPen(QPen(Qt::black,2));
            painter->drawRect(sr);
        }
    }

    //Render the horizontal border for rows. We only render the top border line.
    //With this technique we miss the bottom border line of the last row!!!
    //QRect fullRect=QRect(0,option.rect.y(),painter->device()->width(),option.rect.height());
    QRect bgRect=option.rect;
    painter->setPen(borderPen_);
    painter->drawLine(bgRect.topLeft(),bgRect.topRight());

    painter->restore();
}


void TimelineInfoDailyDelegate::renderTimeline(QPainter *painter,const QStyleOptionViewItem& option,const QModelIndex& index) const
{
    if(!index.isValid())
        return;

    TimelineItem *item=model_->data();
    if(!item)
        return;

    //bool selected=option.state & QStyle::State_Selected;

    int leftEdge=option.rect.x();
    int rightEdge=option.rect.x()+option.rect.width();
    int xpPrev=leftEdge-2;
    int xpNext=rightEdge+2;
    int extendedRight=-1;

    unsigned int day=index.data().toUInt();

    for(size_t i=0; i < item->size(); i++)
    {
        if(item->start_[i] < day)
            continue;
        if(item->start_[i] >= day + 86400)
            break;

        bool hasGrad=false;
        bool lighter=false;
        int xpLeft=0,xpRight=0;
        QColor fillCol;
        int xp=timeToPos(option.rect,item->start_[i]);

        if(i > 0 && item->start_[i-1] < day)
        {
            if(VNState* vn=VNState::find(item->status_[i-1]))
            {
                hasGrad=true;
                xpLeft=leftEdge;
                xpRight=xp;
                fillCol=vn->colour();
                lighter=(vn->name() == "complete");
                drawCell(painter,QRect(xpLeft,option.rect.y(),xpRight-xpLeft+1,option.rect.height()-1),
                         fillCol,hasGrad,lighter);
            }
        }

        if(VNState* vn=VNState::find(item->status_[i]))
        {
            fillCol=vn->colour();
            lighter=(vn->name() == "complete");
            if(i < item->size()-1)
            {
                if(item->start_[i+1] < day+86400)
                {
                    hasGrad=true;
                    xpLeft=xp;
                    xpRight=timeToPos(option.rect,item->start_[i+1]);
                }
                else
                {
                    hasGrad=false;
                    xpLeft=xp;
                    xpRight=rightEdge;
                }
            }
            else
            {
                hasGrad=false;
                xpLeft=xp;
                if(item->start_[i]/86400 < model_->endDateSec()/86400)
                   xpRight=rightEdge;
                else
                {
                    xpRight=timeToPos(option.rect,model_->endDateSec());
                    if(xpRight > rightEdge)
                        xpRight=rightEdge;
                }
            }

            //small rects are extended and use no gradient
            if(xpRight-xpLeft < 5)
            {
                hasGrad=false;
                xpRight=xpLeft+4;
                if(extendedRight == xpRight)
                {
                    xpLeft+=2;
                    xpRight+=2;
                }
                else if(extendedRight > xpRight)
                {
                    xpLeft=extendedRight-2;
                    xpRight=extendedRight;
                }
                extendedRight=xpRight;
            }
            else
            {
                if(extendedRight > xpLeft)
                {
                    xpLeft=extendedRight;
                }
                extendedRight = -1;
            }

            drawCell(painter,QRect(xpLeft,option.rect.y(),xpRight-xpLeft+1,option.rect.height()-1),
                      fillCol,hasGrad,lighter);
        }
    }
}

void TimelineInfoDailyDelegate::drawCell(QPainter *painter,QRect r,QColor fillCol,bool hasGrad,bool lighter) const
{
    QColor endCol;
    if(lighter)
    {
        fillCol.light(130);
        fillCol.setAlpha(110);
    }
    else
    {
        fillCol.dark(110);
    }

    QBrush fillBrush(fillCol);
    if(hasGrad)
    {
        QLinearGradient gr;
        gr.setCoordinateMode(QGradient::ObjectBoundingMode);
        gr.setStart(0,0);
        gr.setFinalStop(1,0);
        gr.setColorAt(0,fillCol);
        fillCol.setAlpha(lighter?40:128);
        gr.setColorAt(1,fillCol);
        fillBrush=QBrush(gr);
    }

    painter->fillRect(r,fillBrush);
}

int TimelineInfoDailyDelegate::timeToPos(QRect r,unsigned int time) const
{
    unsigned int start=startTime_.msecsSinceStartOfDay()/1000;
    unsigned int end=endTime_.msecsSinceStartOfDay()/1000;

    time = time % 86400;

    if(time < start)
        return r.x()-2;

    if(time >= end)
        return r.x()+r.width()+2;

    if(start >= end)
        return r.x()-2;

    return r.x()+static_cast<float>(time-start)*static_cast<float>(r.width())/static_cast<float>((end-start));

}

void TimelineInfoDailyDelegate::setStartTime(QTime t)
{
    startTime_=t;
}

void TimelineInfoDailyDelegate::setEndTime(QTime t)
{
    endTime_=t;
}

void TimelineInfoDailyDelegate::setPeriod(QTime t1,QTime t2)
{
    startTime_=t1;
    endTime_=t2;
}



//======================================================================
//
// TimelineInfoDayView
//
//======================================================================

TimelineInfoDailyView::TimelineInfoDailyView(QWidget* parent) :
     QTreeView(parent),
     model_(NULL),
     header_(NULL),
     headerBeingAdjusted_(false),
     needItemsLayout_(false),
     startTime_(QTime(0,0,0)),
     endTime_(QTime(23,59,59))
{
    setObjectName("view");
    //setProperty("style","nodeView");
    //setProperty("view","table");
    setProperty("log","1");
    setRootIsDecorated(false);
    setSortingEnabled(false);

    setAutoScroll(true);
    setAllColumnsShowFocus(true);
    setUniformRowHeights(true);
    setAlternatingRowColors(false);
    setMouseTracking(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    //setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    model_=new TimelineInfoDailyModel(this);
    QTreeView::setModel(model_);

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

    //Header
    header_=new NodeTimelineHeader(this);
    setHeader(header_);

    connect(header_,SIGNAL(periodSelected(QTime,QTime)),
        this,SLOT(periodSelectedInHeader(QTime,QTime)));

    connect(header_,SIGNAL(periodBeingZoomed(QTime,QTime)),
        this, SIGNAL(periodBeingZoomed(QTime,QTime)));

    //adjustHeader();

    //When the sort model is invalidated it might change the column visibility
    //which we need to readjust!
    connect(model_,SIGNAL(invalidateCalled()),
            this,SLOT(adjustHeader()));

    //Create delegate to the view
    delegate_=new TimelineInfoDailyDelegate(model_,this);
    setItemDelegate(delegate_);

    connect(delegate_,SIGNAL(sizeHintChangedGlobal()),
            this,SLOT(slotSizeHintChangedGlobal()));
}

TimelineInfoDailyView::~TimelineInfoDailyView()
{
}


void TimelineInfoDailyView::load(TimelineItem *data,unsigned int viewStartDateSec,unsigned int viewEndDateSec,
                                unsigned int endDateSec)

{
    model_->load(data, viewStartDateSec, viewEndDateSec, endDateSec);
}

void TimelineInfoDailyView::setZoomActions(QAction* zoomInAction,QAction* zoomOutAction)
{
    header_->setZoomActions(zoomInAction,zoomOutAction);
}

void TimelineInfoDailyView::periodSelectedInHeader(QTime t1,QTime t2)
{
    startTime_=t1;
    endTime_=t2;
    delegate_->setPeriod(t1,t2);
    //model_->tlModel()->setPeriod(t1,t2);
    rerender();
    Q_EMIT periodSelected(t1,t2);
}

void TimelineInfoDailyView::setStartTime(QTime t)
{
    startTime_=t;
    delegate_->setStartTime(t);
    //model_->tlModel()->setStartDate(t);
    header_->setStartTime(t);
    rerender();
}

void TimelineInfoDailyView::setEndTime(QTime t)
{
    endTime_=t;
    delegate_->setEndTime(t);
    //model_->tlModel()->setEndDate(t);
    header_->setEndTime(t);
    rerender();
}

void TimelineInfoDailyView::setPeriod(QTime t1,QTime t2)
{
    startTime_=t1;
    endTime_=t2;
    delegate_->setPeriod(t1,t2);
   // model_->tlModel()->setPeriod(t1,t2);
    header_->setPeriod(t1,t2);
    rerender();
}

void TimelineInfoDailyView::rerender()
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

void TimelineInfoDailyView::readSettings(QSettings& vs)
{
    vs.beginGroup("dailyview");
    ViewerUtil::saveTreeColumnWidth(vs,"columnWidth",this);
    vs.endGroup();
}

void TimelineInfoDailyView::writeSettings(QSettings& vs)
{
    vs.beginGroup("dailyview");
    ViewerUtil::initTreeColumnWidth(vs,"columnWidth",this);
    vs.endGroup();
}
