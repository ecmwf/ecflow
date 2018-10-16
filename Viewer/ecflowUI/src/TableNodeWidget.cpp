/***************************** LICENSE START ***********************************

 Copyright 2009-2017 ECMWF and INPE. This software is distributed under the terms
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
#include "WidgetNameProvider.hpp"

#include <QHBoxLayout>

TableNodeWidget::TableNodeWidget(ServerFilter* serverFilter,bool interactive,QWidget * parent) :
    NodeWidget("table",serverFilter,parent),
    sortModel_(nullptr)
{
	//Init qt-creator form
	setupUi(this);

    bcWidget_=new NodePathWidget(this);

    //This defines how to filter the nodes in the tree.
	filterDef_=new NodeFilterDef(serverFilter_,NodeFilterDef::GeneralScope);

    //Build the filter widget
    filterW_->build(filterDef_,serverFilter_);

    //pop up the editor to define a filter. Only when the users
    //creates a new table view
    if(interactive)
    {
        filterW_->slotEdit();
    }

	//Create the table model. It uses the datahandler to access the data.
    auto* tModel=new TableNodeModel(serverFilter_,filterDef_,this);
    model_=tModel;

	//Create a filter model for the tree.
    sortModel_=new TableNodeSortModel(tModel,this);

	//Create the view
	auto *hb=new QHBoxLayout(viewHolder_);
	hb->setContentsMargins(0,0,0,0);
	hb->setSpacing(0);
    auto *tv=new TableNodeView(sortModel_,filterDef_,this);
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
    actionBreadcrumbs->setChecked(bcWidget_->isGuiMode());

	//The node status filter is exposed via a menu. So we need a reference to it.
	states_=filterDef_->nodeState();

    WidgetNameProvider::nameChildren(this);
}

TableNodeWidget::~TableNodeWidget()
= default;

void TableNodeWidget::populateDockTitleBar(DashboardDockTitleWidget* tw)
{
	//Builds the menu for the settings tool button
	auto *menu=new QMenu(this);
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

    //Add the bc to the titlebar
    tw->setBcWidget(bcWidget_);

	//Sets the title
    //tw->slotUpdateTitle("<b>Table</b>");

    QList<QAction*> acLst;

    //Edit filter
    QAction* acFilterEdit=new QAction(this);
    acFilterEdit->setIcon(QPixmap(":viewer/filter_edit.svg"));
    acFilterEdit->setToolTip("Edit filter ...");
    acLst << acFilterEdit;

    connect(acFilterEdit,SIGNAL(triggered()),
    		filterW_,SLOT(slotEdit()));

    //Add variable column
    QAction* acVar=new QAction(this);
    acVar->setIcon(QPixmap(":viewer/dock_add_variable_column.svg"));
    acVar->setToolTip("Add variable column ...");
    acLst << acVar;

    connect(acVar,SIGNAL(triggered()),
            view_->realWidget(),SLOT(slotAddVariableColumn()));

    tw->addActions(acLst);
}


void TableNodeWidget::slotSelectionChangedInView(VInfo_ptr info)
{
    updateActionState(info);
    bcWidget_->setPath(info);
    if(broadcastSelection())
        Q_EMIT selectionChanged(info);
}

void TableNodeWidget::on_actionBreadcrumbs_triggered(bool b)
{
    if(b)
    {
        bcWidget_->setMode(NodePathWidget::GuiMode);
    }
    else
    {
        bcWidget_->setMode(NodePathWidget::TextMode);
    }
}

void TableNodeWidget::rerender()
{
	bcWidget_->rerender();
	view_->rerender();
}


void TableNodeWidget::writeSettings(VComboSettings* vs)
{
	vs->put("type",type_);
	vs->put("dockId",id_);

	bcWidget_->writeSettings(vs);

	states_->writeSettings(vs);
	filterDef_->writeSettings(vs);

    view_->writeSettings(vs);

    DashboardWidget::writeSettings(vs);
}

void TableNodeWidget::readSettings(VComboSettings* vs)
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
    actionBreadcrumbs->setChecked(bcWidget_->isGuiMode());

    view_->readSettings(vs);

    DashboardWidget::readSettings(vs);
}





