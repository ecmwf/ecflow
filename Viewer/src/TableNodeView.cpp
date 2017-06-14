//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TableNodeView.hpp"

#include <QtGlobal>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStyle>
#include <QToolButton>

#include "ActionHandler.hpp"
#include "FilterWidget.hpp"
#include "IconProvider.hpp"
#include "TableNodeSortModel.hpp"
#include "PropertyMapper.hpp"
#include "TableNodeModel.hpp"
#include "TableNodeViewDelegate.hpp"
#include "UiLog.hpp"
#include "VFilter.hpp"
#include "VSettings.hpp"

#define _UI_TABLENODEVIEW_DEBUG

TableNodeView::TableNodeView(TableNodeSortModel* model,NodeFilterDef* filterDef,QWidget* parent) :
     QTreeView(parent),
     NodeViewBase(filterDef),
     model_(model),
	 needItemsLayout_(false),
     prop_(NULL),
     setCurrentIsRunning_(false)
{
    setObjectName("view");
    setProperty("style","nodeView");
	setProperty("view","table");

	setRootIsDecorated(false);

    setSortingEnabled(true);
    //sortByColumn(0,Qt::AscendingOrder);

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

	actionHandler_=new ActionHandler(this);

	//expandAll();

	//Header
	header_=new TableNodeHeader(this);

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
    TableNodeViewDelegate *delegate=new TableNodeViewDelegate(this);
    setItemDelegate(delegate);

	connect(delegate,SIGNAL(sizeHintChangedGlobal()),
			this,SLOT(slotSizeHintChangedGlobal()));

    //Properties
	std::vector<std::string> propVec;
	propVec.push_back("view.table.background");
	prop_=new PropertyMapper(propVec,this);

	//Initialise bg
	adjustBackground(prop_->find("view.table.background")->value().value<QColor>());
}

TableNodeView::~TableNodeView()
{
    delete prop_;
}

void TableNodeView::setModel(TableNodeSortModel *model)
{
	model_= model;

	//Set the model.
	QTreeView::setModel(model_);
}

QWidget* TableNodeView::realWidget()
{
	return this;
}


//Collects the selected list of indexes
QModelIndexList TableNodeView::selectedList()
{
    QModelIndexList lst;
    Q_FOREACH(QModelIndex idx,selectedIndexes())
        if(idx.column() == 0)
            lst << idx;
    return lst;
}


// reimplement virtual function from QTreeView - called when the selection is changed
void TableNodeView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
	QModelIndexList lst=selectedIndexes();
	if(lst.count() > 0)
	{
		VInfo_ptr info=model_->nodeInfo(lst.front());
		if(info && !info->isEmpty())
		{
#ifdef _UI_TABLENODEVIEW_DEBUG
            UiLog().dbg() << "TableNodeView::selectionChanged --> emit=" << info->path();
#endif
			Q_EMIT selectionChanged(info);
		}
	}
	QTreeView::selectionChanged(selected, deselected);

    //The model has to know about the selection in order to manage the
    //nodes that are forced to be shown
    model_->selectionChanged(lst);
}

VInfo_ptr TableNodeView::currentSelection()
{
	QModelIndexList lst=selectedIndexes();
	if(lst.count() > 0)
	{
		return model_->nodeInfo(lst.front());
	}
	return VInfo_ptr();
}

void TableNodeView::setCurrentSelection(VInfo_ptr info)
{
    //While the current is being selected we do not allow
    //another setCurrent call go through
    if(setCurrentIsRunning_)
        return;

    setCurrentIsRunning_=true;
    QModelIndex idx=model_->infoToIndex(info);
    if(idx.isValid())
    {
#ifdef _UI_TABLENODEVIEW_DEBUG
    if(info)
        UiLog().dbg() << "TableNodeView::setCurrentSelection --> " <<  info->path();
#endif
        setCurrentIndex(idx);
    }
    setCurrentIsRunning_=false;
}

void TableNodeView::slotDoubleClickItem(const QModelIndex&)
{
}

void TableNodeView::slotContextMenu(const QPoint &position)
{
	QModelIndexList lst=selectedList();
	//QModelIndex index=indexAt(position);
	QPoint scrollOffset(horizontalScrollBar()->value(),verticalScrollBar()->value());

	handleContextMenu(indexAt(position),lst,mapToGlobal(position),position+scrollOffset,this);
}


void TableNodeView::handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget)
{
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
}

void TableNodeView::slotViewCommand(VInfo_ptr info,QString cmd)
{
}

void TableNodeView::rerender()
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

void TableNodeView::slotRerender()
{
	rerender();
}

void TableNodeView::slotSizeHintChangedGlobal()
{
	needItemsLayout_=true;
}

void TableNodeView::adjustBackground(QColor col)
{
	if(col.isValid())
	{
		QString sh="QTreeView { background : " + col.name() + ";}";
		setStyleSheet(sh);
	}
}

void TableNodeView::notifyChange(VProperty* p)
{
	if(p->path() == "view.table.background")
	{
		adjustBackground(p->value().value<QColor>());
	}
}

//=========================================
// Header
//=========================================

void TableNodeView::slotHeaderContextMenu(const QPoint &position)
{
    int section=header_->logicalIndexAt(position);

	if(section< 0 || section >= header_->count())
		return;

    int visCnt=0;
    for(int i=0; i <header_->count(); i++)
        if(!header_->isSectionHidden(i))
            visCnt++;

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
}

void TableNodeView::readSettings(VSettings* vs)
{
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
}

void TableNodeView::writeSettings(VSettings* vs)
{
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
}

//=========================================
// TableNodeHeader
//=========================================

TableNodeHeader::TableNodeHeader(QWidget *parent) : QHeaderView(Qt::Horizontal, parent)
{
	setStretchLastSection(true);

	connect(this, SIGNAL(sectionResized(int, int, int)),
    		 this, SLOT(slotSectionResized(int)));

    int pixId=IconProvider::add(":viewer/filter_decor.svg","filter_decor");

    customPix_=IconProvider::pixmap(pixId,10);


     //connect(this, SIGNAL(sectionMoved(int, int, int)), this,
     //        SLOT(handleSectionMoved(int, int, int)));

     //setMovable(true);
}

void TableNodeHeader::showEvent(QShowEvent *e)
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

void TableNodeHeader::slotSectionResized(int i)
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

QSize TableNodeHeader::sizeHint() const
{
	return QHeaderView::sizeHint();

	QSize s = size();
    //s.setHeight(headerSections[0]->minimumSizeHint().height() + 35);
    //s.setHeight(2*35);
    return s;
}

void TableNodeHeader::setModel(QAbstractItemModel *model)
{
	if(model)
	{
		for(int i=0; i< model->columnCount(); i++)
		{
			QString id=model->headerData(i,Qt::Horizontal,Qt::UserRole).toString();
			if(id == "status")
				customButton_.insert(i,TableNodeHeaderButton(id));
		}
	}
	QHeaderView::setModel(model);
}

void TableNodeHeader::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
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


	QMap<int,TableNodeHeaderButton>::iterator it=customButton_.find(logicalIndex);
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

	QString text=model()->headerData(logicalIndex,Qt::Horizontal).toString();
	QRect textRect=rect;
	textRect.setRight(rightPos-5);

	painter->drawText(textRect,Qt::AlignHCenter | Qt::AlignVCenter,text);


	//style()->drawControl(QStyle::CE_PushButton, &optButton,painter,this);
}

void TableNodeHeader::mousePressEvent(QMouseEvent *event)
{
	QMap<int,TableNodeHeaderButton>::const_iterator it = customButton_.constBegin();
	while(it != customButton_.constEnd())
	{
		if(it.value().rect_.contains(event->pos()))
		{
            UiLog().dbg() << "header " << it.key() << " clicked";
			Q_EMIT customButtonClicked(it.value().id(),event->globalPos());
		}
	     ++it;
	 }

	QHeaderView::mousePressEvent(event);
}

/*void TableNodeHeader::mouseMoveEvent(QMouseEvent *event)
{
	int prevIndex=hoverIndex_;
	QMap<int,TableNodeHeaderButton>::const_iterator it = customButton_.constBegin();
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

