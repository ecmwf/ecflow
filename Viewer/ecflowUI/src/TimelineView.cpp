//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TimelineView.hpp"

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
#include "UiLog.hpp"
#include "VFilter.hpp"
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
    borderPen_(QPen(QColor(230,230,230)))
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
        renderTimeline(painter,option,index.row());

        //QString text=index.data(Qt::DisplayRole).toString();
        //renderNode(painter,index,vopt,text);
    }

#if 0
    else if(id == "status")
    {
         renderStatus(painter,index,vopt);
    }

    //Render attributes
    else if(id == "event" || id == "label" || id == "meter" || id == "trigger")
    {
        QVariant va=index.data(Qt::DisplayRole);
        if(va.type() == QVariant::StringList)
        {
            QStringList lst=va.toStringList();
            if(lst.count() > 0)
            {
                QMap<QString,AttributeRendererProc>::const_iterator it=attrRenderers_.find(lst.at(0));
                if(it != attrRenderers_.end())
                {
                    QSize size;
                    AttributeRendererProc a=it.value();
                    (this->*a)(painter,lst,vopt,size);
                }
            }
        }
    }
#endif

    //rest of the columns
    else
    {
        QString text=index.data(Qt::DisplayRole).toString();
        QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt,widget);
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
    TimelineData *data=model_->data();
    if(!data)
        return;

    bool selected=option.state & QStyle::State_Selected;
    QFontMetrics fm(font_);

    for(size_t i=0; i < data->items()[row].size(); i++)
    {
        int xp=timeToPos(option.rect,data->items()[row].start_[i]);
        UiLog().dbg() << "xp=" << xp << " time=" << data->items()[row].start_[i];
        painter->fillRect(QRect(xp,option.rect.y(),10,10),Qt::red);
    }

    //The initial filled rect (we will adjust its  width)
    //QRect itemRect=option.rect.adjusted(nodeBox_->leftMargin,nodeBox_->topMargin,0,-nodeBox_->bottomMargin);



}

int TimelineDelegate::timeToPos(QRect r,unsigned int time) const
{
    unsigned int start=model_->data()->startTime();
    unsigned int end=model_->data()->endTime()+3600;

    if(start >= end)
        return r.x();

    return r.x()+static_cast<float>(time-start)*static_cast<float>(r.width())/static_cast<float>((end-start));

}


#if 0
void TimelineDelegate::renderNode(QPainter *painter,const QModelIndex& index,
                                    const QStyleOptionViewItem& option,QString text) const
{
    bool selected=option.state & QStyle::State_Selected;
    QFontMetrics fm(font_);

    //The initial filled rect (we will adjust its  width)
    QRect itemRect=option.rect.adjusted(nodeBox_->leftMargin,nodeBox_->topMargin,0,-nodeBox_->bottomMargin);

    //The text rectangle
    QRect textRect = itemRect;

    int textWidth=fm.width(text);
    textRect.setWidth(textWidth+nodeBox_->leftPadding+nodeBox_->rightPadding);

    //Adjust the filled rect width
    int currentRight=textRect.x()+textRect.width();

    //Icons area
    QList<QPixmap> pixLst;
    QList<QRect> pixRectLst;

    QVariant va=index.data(AbstractNodeModel::IconRole);
    if(va.type() == QVariant::List)
    {
        QVariantList lst=va.toList();
        if(lst.count() >0)
        {
            int xp=currentRight+nodeBox_->iconPreGap;
            int yp=itemRect.center().y()+1-nodeBox_->iconSize/2;
            for(int i=0; i < lst.count(); i++)
            {
                int id=lst[i].toInt();
                if(id != -1)
                {
                    pixLst << IconProvider::pixmap(id,nodeBox_->iconSize);
                    pixRectLst << QRect(xp,yp,nodeBox_->iconSize,nodeBox_->iconSize);
                    xp+=nodeBox_->iconSize+nodeBox_->iconGap;
                }
            }

            if(!pixLst.isEmpty())
            {
                currentRight=xp-nodeBox_->iconGap;
            }
        }
    }

    //Define clipping
    int rightPos=currentRight+1;
    const bool setClipRect = rightPos > option.rect.right();
    if(setClipRect)
    {
        painter->save();
        painter->setClipRect(option.rect);
    }

    //Draw text
    QColor fg=index.data(Qt::ForegroundRole).value<QColor>();
    painter->setPen(fg);
    painter->setFont(font_);
    painter->drawText(textRect,Qt::AlignHCenter | Qt::AlignVCenter,text);

    if(selected)
    {
        QRect sr=textRect;
        sr.setX(option.rect.x()+nodeBox_->leftMargin);
        renderSelectionRect(painter,sr);
    }

    //Draw icons
    for(int i=0; i < pixLst.count(); i++)
    {
        painter->drawPixmap(pixRectLst[i],pixLst[i]);
    }

    if(setClipRect)
    {
        painter->restore();
    }
}
#endif

//======================================================================
//
// TimelineView
//
//======================================================================

TimelineView::TimelineView(TimelineModel* model,QWidget* parent) :
     QTreeView(parent),
     model_(model),
     needItemsLayout_(false),
     prop_(NULL),
     setCurrentIsRunning_(false)
{
    setObjectName("view");
    setProperty("style","nodeView");
    setProperty("view","table");

    setRootIsDecorated(false);

    //We enable sorting but do not want to perform it immediately
    setSortingEnabledNoExec(true);

    //setSortingEnabled(false);
    //sortByColumn(-1,Qt::AscendingOrder);

    setAllColumnsShowFocus(true);
    setUniformRowHeights(true);
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

    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
                        this, SLOT(slotContextMenu(const QPoint &)));

    //Selection
    connect(this,SIGNAL(doubleClicked(const QModelIndex&)),
            this,SLOT(slotDoubleClickItem(const QModelIndex)));

    actionHandler_=new ActionHandler(this,this);

    //expandAll();

    //Header
    header_=new TimelineHeader(this);

    setHeader(header_);

    //Set header ContextMenuPolicy
    header_->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(header_,SIGNAL(customContextMenuRequested(const QPoint &)),
                this, SLOT(slotHeaderContextMenu(const QPoint &)));

    connect(header_,SIGNAL(customButtonClicked(QString,QPoint)),
            this,SIGNAL(headerButtonClicked(QString,QPoint)));

    //for(int i=0; i < model_->columnCount(QModelIndex())-1; i++)
    //  	resizeColumnToContents(i);

    /*connect(header(),SIGNAL(sectionMoved(int,int,int)),
                this, SLOT(slotMessageTreeColumnMoved(int,int,int)));*/

    QTreeView::setModel(model_);

    //Create delegate to the view
    TimelineDelegate *delegate=new TimelineDelegate(model_,this);
    setItemDelegate(delegate);

    connect(delegate,SIGNAL(sizeHintChangedGlobal()),
            this,SLOT(slotSizeHintChangedGlobal()));

    //Properties
    std::vector<std::string> propVec;
    propVec.push_back("view.table.background");
    prop_=new PropertyMapper(propVec,this);

    //Initialise bg
    adjustBackground(prop_->find("view.table.background")->value().value<QColor>());

    header_->setSortIndicatorShown(true);
}

TimelineView::~TimelineView()
{
    delete prop_;
}

#if 0
void TimelineView::setModel(TableNodeSortModel *model)
{
    model_= model;

    //Set the model.
    QTreeView::setModel(model_);
}
#endif


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

    //The model has to know about the selection in order to manage the
    //nodes that are forced to be shown
    model_->selectionChanged(lst);
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

void TimelineView::slotDoubleClickItem(const QModelIndex&)
{
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
#if 0
    //Node actions
        if(indexClicked.isValid() && indexClicked.column() == 0)   //indexLst[0].isValid() && indexLst[0].column() == 0)
        {
            UiLog().dbg()  << "context menu " << indexClicked;

            std::vector<VInfo_ptr> nodeLst;
            for(int i=0; i < indexLst.count(); i++)
            {
                VInfo_ptr info=model_->nodeInfo(indexLst[i]);
                if(!info->isEmpty())
                    nodeLst.push_back(info);
            }

            actionHandler_->contextMenu(nodeLst,globalPos);
        }

        //Desktop actions
        else
        {
        }
#endif
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

void TimelineView::setStartDate(QDateTime t)
{
    header_->setStartDate(t);
}

void TimelineView::setEndDate(QDateTime t)
{
    header_->setEndDate(t);
}

void TimelineView::setPeriod(QDateTime t1,QDateTime t2)
{
    header_->setPeriod(t1,t2);

}

//=========================================
// Header
//=========================================

void TimelineView::slotHeaderContextMenu(const QPoint &position)
{
#if 0
    int section=header_->logicalIndexAt(position);

    if(section< 0 || section >= header_->count())
        return;

    int visCnt=0;
    for(int i=0; i <header_->count(); i++)
    {
        if(!header_->isSectionHidden(i))
            visCnt++;
    }

    QList<QAction*> lst;
    QMenu *menu=new QMenu(this);
    QAction *ac;

    for(int i=0; i <header_->count(); i++)
    {
        QString name=header_->model()->headerData(i,Qt::Horizontal).toString();
        ac=new QAction(menu);
        ac->setText(name);
        ac->setCheckable(true);
        ac->setData(i);

        bool vis=!header_->isSectionHidden(i);
        ac->setChecked(vis);

        if(vis && visCnt <=1)
        {
            ac->setEnabled(false);
        }

        menu->addAction(ac);
    }

    //stateFilterMenu_=new StateFilterMenu(menuState,filter_->menu());
    //VParamFilterMenu stateFilterMenu(menu,filterDef_->nodeState(),VParamFilterMenu::ColourDecor);

    ac=menu->exec(header_->mapToGlobal(position));
    if(ac && ac->isEnabled() && ac->isCheckable())
    {
        int i=ac->data().toInt();
        header_->setSectionHidden(i,!ac->isChecked());
    }
    delete menu;

#endif
}

void TimelineView::readSettings(VSettings* vs)
{
#if 0
    vs->beginGroup("column");

    std::vector<std::string> orderVec;
    std::vector<int> visVec, wVec;

    vs->get("order",orderVec);
    vs->get("visible",visVec);
    vs->get("width",wVec);

    vs->endGroup();

    if(orderVec.size() != visVec.size() || orderVec.size() != wVec.size())
        return;

    for(size_t i=0; i < orderVec.size(); i++)
    {
        std::string id=orderVec[i];
        for(int j=0; j < model_->columnCount(QModelIndex()); j++)
        {
            if(model_->headerData(j,Qt::Horizontal,Qt::UserRole).toString().toStdString() == id)
            {
                if(visVec[i] == 0)
                    header()->setSectionHidden(j,true);

                else if(wVec[i] > 0)
                    setColumnWidth(j,wVec[i]);

                break;
            }
        }
    }

    if(header_->count() > 0)
    {
        int visCnt=0;
        for(int i=0; i < header_->count(); i++)
            if(!header_->isSectionHidden(i))
                visCnt++;

        if(visCnt==0)
            header()->setSectionHidden(0,false);
    }
#endif
}

void TimelineView::writeSettings(VSettings* vs)
{
#if 0
    vs->beginGroup("column");

    std::vector<std::string> orderVec;
    std::vector<int> visVec, wVec;
    for(int i=0; i < model_->columnCount(QModelIndex()); i++)
    {
        std::string id=model_->headerData(i,Qt::Horizontal,Qt::UserRole).toString().toStdString();
        orderVec.push_back(id);
        visVec.push_back((header()->isSectionHidden(i))?0:1);
        wVec.push_back(columnWidth(i));
    }

    vs->put("order",orderVec);
    vs->put("visible",visVec);
    vs->put("width",wVec);

    vs->endGroup();
#endif
}

//=========================================
// TimelineHeader
//=========================================

TimelineHeader::TimelineHeader(QWidget *parent) :
    QHeaderView(Qt::Horizontal, parent),
    fm_(QFont())
{
    setStretchLastSection(true);

    connect(this, SIGNAL(sectionResized(int, int, int)),
             this, SLOT(slotSectionResized(int)));

    int pixId=IconProvider::add(":viewer/filter_decor.svg","filter_decor");

    customPix_=IconProvider::pixmap(pixId,10);

    font_= QFont();
    font_.setPointSize(font_.pointSize()-2);
    fm_=QFontMetrics(font_);


     //connect(this, SIGNAL(sectionMoved(int, int, int)), this,
     //        SLOT(handleSectionMoved(int, int, int)));

     //setMovable(true);
}




void TimelineHeader::showEvent(QShowEvent *e)
{
  /*  for(int i=0;i<count();i++)
    {


       if(1)
       {
           widgets_[i]->setGeometry(sectionViewportPosition(i),0,
                                sectionSize(i),height());
           widgets_[i]->show();
       }
    }
    */

    QHeaderView::showEvent(e);
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

QSize TimelineHeader::sizeHint() const
{
    //return QHeaderView::sizeHint();

    QSize s = QHeaderView::sizeHint(); //size();
    //s.setHeight(headerSections[0]->minimumSizeHint().height() + 35);
    //s.setHeight(2*35);
    s.setHeight(fm_.height()*2+20);
    return s;
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

    //painter->drawText(textRect,Qt::AlignHCenter | Qt::AlignVCenter,text);


    if(logicalIndex == 0)
    {
       painter->drawText(textRect,Qt::AlignHCenter | Qt::AlignVCenter,text);
       return;
    }

    renderTimeline(rect,painter);

#if 0
    qint64 startSec=startDate_.toMSecsSinceEpoch()/1000;
    qint64 endSec=endDate_.toMSecsSinceEpoch()/1000;

    qint64 period=endSec-startSec;

    int minorTic=1;
    qint64 firstTic=1;
    int tickGap=4;


    if(period < 3600)
    {
        minorTic=60;
        firstTic=(startSec/60)*60+60;
    }

    qint64 actSec=firstTic;


    Q_ASSERT(actSec >= startSec);
    while(actSec <= endSec)
    {
        int xp=secToPos(actSec-startSec,rect);

        int yp=rect.bottom()-1-fm_.height()-tickGap;


        painter->drawLine(xp,rect.top()+4,xp,yp);


        if(actSec % 600 == 0)
        {
            //Day
            QString majorText=QDateTime::fromMSecsSinceEpoch(actSec*1000).toString("yyyy-MM-dd");



            QString s=QDateTime::fromMSecsSinceEpoch(actSec * 1000).toString("H:mm");

            int textW=fm_.width(s);
            int tickTextY=yp+tickGap;
            //int yp=rect.bottom()-1-fm.height();
            painter->setFont(font_);
            painter->drawText(QRect(xp-textW/2, tickTextY,textW,fm_.height()),Qt::AlignHCenter | Qt::AlignVCenter,s);
        }


        actSec+=minorTic;
    }

    //for(int i=0; i < 10; i++)
    //{
    //    int xp=i*rect.width()/10;
    //    painter->drawLine(xp,rect.top(),xp,rect.bottom());
    //}

    //style()->drawControl(QStyle::CE_PushButton, &optButton,painter,this);
#endif
}


void TimelineHeader::renderTimeline(const QRect& rect,QPainter* painter) const
{
    //period in secs
    qint64 startSec=startDate_.toMSecsSinceEpoch()/1000;
    qint64 endSec=endDate_.toMSecsSinceEpoch()/1000;
    qint64 period=endSec-startSec;

    int minorTick=1; //in secs (it is a delta)
    int majorTick=1;  //in secs (it is a delta)
    qint64 firstTick=1; //in secs since epoch

    int hLineY=rect.center().y();
    int timeTextGap=4; //the gap between the top of the time text and the bottom of the major tick in pixels
    int majorTickTop=hLineY;
    int majorTickBottom=rect.bottom()-1-fm_.height()-timeTextGap;
    int minorTickTop=hLineY;
    int minorTickBottom=majorTickBottom-3;
    int dateTextY= hLineY-1;;
    int timeTextY=rect.bottom()-1-fm_.height();

    if(period < 3600)
    {
        minorTick=60;
        majorTick=600;
        firstTick=(startSec/60)*60+60;
    }

    //Find label positions for days
    QList<QPair<int,QString> > dateLabels;
    int dayNum=startDate_.date().daysTo(endDate_.date());

    if(dayNum == 0)
    {
        int xp=secToPos((startSec+endSec)/2,rect);
        dateLabels << qMakePair(xp,startDate_.toString("dd MMM"));
    }
    else
    {
        QDate  nextDay=startDate_.date().addDays(1);
        QDate  lastDay=endDate_.date();
        qint64 nextSec=QDateTime(nextDay).toMSecsSinceEpoch()/1000;

        if((nextSec-startSec) < 3600)
        {
            int xp=secToPos((startSec+nextSec)/2,rect);
            dateLabels << qMakePair(xp,nextDay.toString("dd MMM"));
        }

        QDate firstDay=nextDay;
        for(QDate d=firstDay; d < lastDay; d=d.addDays(1))
        {
            int xp=secToPos((QDateTime(d).toMSecsSinceEpoch()/1000 +
            QDateTime(d.addDays(1)).toMSecsSinceEpoch()/1000)/2,rect);
            dateLabels << qMakePair(xp,d.toString("dd MMM"));
        }

        if(QDateTime(lastDay).toMSecsSinceEpoch()/1000 < endSec)
        {
            int xp=secToPos((QDateTime(lastDay).toMSecsSinceEpoch()/1000+endSec)/2,rect);
            dateLabels << qMakePair(xp,lastDay.toString("dd MMM"));
        }
    }

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
    painter->drawLine(rect.x(),hLineY,rect.right(),hLineY);

    qint64 actSec=firstTick;
    Q_ASSERT(actSec >= startSec);

    while(actSec <= endSec)
    {
        int xp=secToPos(actSec-startSec,rect);

        //draw major tick + label
        if(actSec % majorTick == 0)
        {
            painter->drawLine(xp,majorTickTop,xp,majorTickBottom);

            //Day
            QString majorText=QDateTime::fromMSecsSinceEpoch(actSec*1000).toString("yyyy-MM-dd");

            QString s=QDateTime::fromMSecsSinceEpoch(actSec * 1000).toString("H:mm");

            int textW=fm_.width(s);
            //int yp=rect.bottom()-1-fm.height();
            painter->setFont(font_);
            painter->drawText(QRect(xp-textW/2, timeTextY,textW,fm_.height()),Qt::AlignHCenter | Qt::AlignVCenter,s);

        }
        //draw minor tick
        else
        {
            painter->drawLine(xp,minorTickTop,xp,minorTickBottom);
        }

        actSec+=minorTick;
    }

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


void TimelineHeader::setStartDate(QDateTime t)
{
   startDate_=t;
}

void TimelineHeader::setEndDate(QDateTime t)
{
    endDate_=t;
}

void TimelineHeader::setPeriod(QDateTime t1,QDateTime t2)
{
    startDate_=t1;
    endDate_=t2;
    update();
}
