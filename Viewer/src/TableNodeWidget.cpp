/***************************** LICENSE START ***********************************

 Copyright 2015 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#include "TableNodeWidget.hpp"
#include "NodeViewBase.hpp"

#include "AbstractNodeModel.hpp"
#include "NodeFilterModel.hpp"
#include "NodePathWidget.hpp"
#include "TableNodeModel.hpp"
#include "TableNodeView.hpp"
#include "VFilter.hpp"
#include "VModelData.hpp"
#include "VSettings.hpp"

#include <QMenu>


TableNodeWidget::TableNodeWidget(ServerFilter* servers,QWidget * parent) : NodeWidget(parent)
{
	//Init qt-creator form
	setupUi(this);

	//Define the icon filter for the model. It controls what icons
	//are displayed next to the nodes. This is exposed via a menu.
	icons_=new IconFilter;

	//This defines how to filter the nodes in the tree. We only want to filter according to node status.
	NodeFilterDef *filterDef_=new NodeFilterDef(NodeFilterDef::NodeState);

	//Create the data handler for the tree model.
	data_=new VModelData(servers,filterDef_,VModelData::TableModel);

	//Create the table model. It uses the datahandler to access the data.
	model_=new TableNodeModel(data_,icons_,parent);

	//Create a filter model for the tree.
	filterModel_=new NodeFilterModel(model_,parent);

	//Set the model on the view.
	viewWidget_->setModel(filterModel_);

	//Store the pointer to the (non-QObject) base class of the view!!!
	view_=viewWidget_;

	//Signals-slots

	connect(view_->realWidget(),SIGNAL(selectionChanged(VInfo_ptr)),
		    bcWidget_,SLOT(setPath(VInfo_ptr)));

	connect(bcWidget_,SIGNAL(selected(VInfo_ptr)),
			view_->realWidget(),SLOT(slotSetCurrent(VInfo_ptr)));

	connect(view_->realWidget(),SIGNAL(selectionChanged(VInfo_ptr)),
			this,SIGNAL(selectionChanged(VInfo_ptr)));


	//Builds the menu for the settings tool button
	QMenu *menu=new QMenu(this);

	menu->addAction(actionBreadcrumbs);

	//Sets the menu on the toolbutton
	viewTb->setMenu(menu);
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

	//bcWidget_->clear();
}

void TableNodeWidget::writeSettings(VSettings* vs)
{
	vs->put("type","table");
	vs->put("dockId",id_);

	bcWidget_->writeSettings(vs);

	//states_->writeSettings(vs);
	//atts_->writeSettings(vs);
	//icons_->writeSettings(vs);
}

void TableNodeWidget::readSettings(VSettings* vs)
{
	std::string type=vs->get<std::string>("type","");
	if(type != "table")
		return;

	//This will not emit the changed signal. So the "observers" will
	//not notice the change.
	//states_->readSettings(vs);
	//atts_->readSettings(vs);
	//icons_->readSettings(vs);

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

	//attrFilterMenu_->reload();
	//iconFilterMenu_->reload();
	//stateFilterMenu_->reload();

}
