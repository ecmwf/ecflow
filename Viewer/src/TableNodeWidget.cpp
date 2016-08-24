/***************************** LICENSE START ***********************************

 Copyright 2016 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#include "TableNodeWidget.hpp"

#include <QDebug>
#include <QHBoxLayout>

#include "AbstractNodeModel.hpp"
#include "DashboardDock.hpp"
#include "FilterWidget.hpp"
#include "NodePathWidget.hpp"
#include "NodeViewBase.hpp"
#include "TableFilterWidget.hpp"
#include "TableNodeModel.hpp"
#include "TableNodeSortModel.hpp"
#include "TableNodeView.hpp"
#include "VFilter.hpp"
#include "VSettings.hpp"

#include <QHBoxLayout>

TableNodeWidget::TableNodeWidget(ServerFilter* serverFilter,QWidget * parent) :
    NodeWidget("table",serverFilter,parent),
    sortModel_(0)
{
	//Init qt-creator form
	setupUi(this);

	//This defines how to filter the nodes in the tree. We only want to filter according to node status.
	filterDef_=new NodeFilterDef(serverFilter_,NodeFilterDef::GeneralScope);

	//Create the table model. It uses the datahandler to access the data.
    TableNodeModel* tModel=new TableNodeModel(serverFilter_,filterDef_,this);
    model_=tModel;

	//Create a filter model for the tree.
    sortModel_=new TableNodeSortModel(tModel,this);

	//Build the filter widget
	filterW_->build(filterDef_,serverFilter_);

	//Create the view
	QHBoxLayout *hb=new QHBoxLayout(viewHolder_);
	hb->setContentsMargins(0,0,0,0);
	hb->setSpacing(0);
    TableNodeView *tv=new TableNodeView(sortModel_,filterDef_,this);
	hb->addWidget(tv);

	//Store the pointer to the (non-QObject) base class of the view!!!
	view_=tv;

	//Signals-slots

    connect(view_->realWidget(),SIGNAL(selectionChanged(VInfo_ptr)),
            this,SLOT(slotSelectionChangedInView(VInfo_ptr)));

    connect(view_->realWidget(),SIGNAL(infoPanelCommand(VInfo_ptr,QString)),
    	    this,SIGNAL(popInfoPanel(VInfo_ptr,QString)));

	connect(view_->realWidget(),SIGNAL(dashboardCommand(VInfo_ptr,QString)),
			this,SIGNAL(dashboardCommand(VInfo_ptr,QString)));

    connect(bcWidget_,SIGNAL(selected(VInfo_ptr)),           
            this,SLOT(slotSelectionChangedInBc(VInfo_ptr)));

    connect(view_->realWidget(),SIGNAL(headerButtonClicked(QString,QPoint)),
    		filterW_,SLOT(slotHeaderFilter(QString,QPoint)));

#if 0
	connect(model_,SIGNAL(clearBegun(const VNode*)),
			view_->realWidget(),SLOT(slotSaveExpand(const VNode*)));

	connect(model_,SIGNAL(scanEnded(const VNode*)),
				view_->realWidget(),SLOT(slotRestoreExpand(const VNode*)));
#endif

	connect(model_,SIGNAL(rerender()),
				view_->realWidget(),SLOT(slotRerender()));

	//This will not emit the trigered signal of the action!!
	//Synchronise the action and the breadcrumbs state
	actionBreadcrumbs->setChecked(bcWidget_->active());

	//The node status filter is exposed via a menu. So we need a reference to it.
	states_=filterDef_->nodeState();
}

TableNodeWidget::~TableNodeWidget()
{

}

void TableNodeWidget::populateDockTitleBar(DashboardDockTitleWidget* tw)
{
	//Builds the menu for the settings tool button
	QMenu *menu=new QMenu(this);
	menu->setTearOffEnabled(true);

	menu->addAction(actionBreadcrumbs);

#if 0
    QMenu *menuState=menu->addMenu(tr("Status"));
	menuState->setTearOffEnabled(true);

	//stateFilterMenu_=new StateFilterMenu(menuState,filter_->menu());
	stateFilterMenu_=new VParamFilterMenu(menuState,states_,"Status filter",
                    //VParamFilterMenu::FilterMode,VParamFilterMenu::ColourDecor);
                     VParamFilterMenu::ShowMode,VParamFilterMenu::ColourDecor);

#endif
    //Sets the menu on the toolbutton
	tw->optionsTb()->setMenu(menu);

	//Sets the title
	tw->slotUpdateTitle("<b>Table</b>");

    QList<QAction*> acLst;
    QAction* acFilterEdit=new QAction(this);
    acFilterEdit->setIcon(QPixmap(":viewer/filter_edit.svg"));
    acFilterEdit->setToolTip("Edit filter ...");
    acLst << acFilterEdit;

    connect(acFilterEdit,SIGNAL(triggered()),
    		filterW_,SLOT(slotEdit()));

    tw->addActions(acLst);
}


void TableNodeWidget::slotSelectionChangedInView(VInfo_ptr info)
{
    updateActionState(info);
    bcWidget_->setPath(info);
    Q_EMIT selectionChanged(info);
}

void TableNodeWidget::on_actionBreadcrumbs_triggered(bool b)
{
	if(b)
	{
		bcWidget_->active(true);
        bcWidget_->setPath(view_->currentSelection());
	}
	else
	{
		bcWidget_->active(false);
	}
}

void TableNodeWidget::rerender()
{
	bcWidget_->rerender();
	view_->rerender();
}


void TableNodeWidget::writeSettings(VSettings* vs)
{
	vs->put("type",type_);
	vs->put("dockId",id_);

	bcWidget_->writeSettings(vs);

	states_->writeSettings(vs);
	filterDef_->writeSettings(vs);

    view_->writeSettings(vs);

    DashboardWidget::writeSettings(vs);
}

void TableNodeWidget::readSettings(VSettings* vs)
{
	std::string type=vs->get<std::string>("type","");
	if(type != type_)
		return;

	//This will not emit the changed signal. So the "observers" will
	//not notice the change.
	states_->readSettings(vs);
	filterDef_->readSettings(vs);

	//The model at this point is inactive (not using its data). We make it active:
	//	-it will instruct its data provider to filter the data according
	//   to the current settings
	//	-it will load and display the data
	model_->active(true);

	//--------------------------
	//Breadcrumbs
	//--------------------------

	bcWidget_->readSettings(vs);

	//Synchronise the action and the breadcrumbs state
	//This will not emit the trigered signal of the action!!
	actionBreadcrumbs->setChecked(bcWidget_->active());

    DashboardWidget::readSettings(vs);
}





