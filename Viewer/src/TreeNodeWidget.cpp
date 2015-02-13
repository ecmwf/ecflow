/***************************** LICENSE START ***********************************

 Copyright 2015 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#include "TreeNodeWidget.hpp"


#include "NodeViewBase.hpp"

#include "AbstractNodeModel.hpp"
#include "NodeFilterModel.hpp"
#include "NodeModelData.hpp"
#include "NodePathWidget.hpp"
#include "TreeNodeModel.hpp"
#include "TreeNodeView.hpp"
#include "VFilter.hpp"
#include "VSettings.hpp"

#include "FilterWidget.hpp"

TreeNodeWidget::TreeNodeWidget(ServerFilter* servers,QWidget* parent) : NodeWidget(parent)
{
	//Init qt-creator form
	setupUi(this);

	//Define the icon filter for the model. It controls what icons
	//are displayed next to the nodes. This is exposed via a menu.
	icons_=new IconFilter;

	//Define the attribute filter for the model. It controls what attributes
	//are displayed for a given node. This is exposed via a menu.
	atts_=new AttributeFilter;

	//This defines how to filter the nodes in the tree. We only want to filter according to node status.
	NodeFilterDef *filterDef_=new NodeFilterDef(NodeFilterDef::NodeState);

	//The node status filter is exposed via a menu. So we need a reference to it.
	states_=filterDef_->nodeState();

	//Create the data handler for the tree model.
	data_=new TreeNodeModelDataHandler(filterDef_);
	data_->reset(servers);

	//Create the tree model. It uses the datahandler to access the data.
	model_=new TreeNodeModel(data_,atts_,icons_,parent);

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
	QMenu *menuState=menu->addMenu(tr("Status"));
	QMenu *menuType=menu->addMenu(tr("Attribute"));
	QMenu *menuIcon=menu->addMenu(tr("Icon"));

	//stateFilterMenu_=new StateFilterMenu(menuState,filter_->menu());
	attrFilterMenu_=new VParamFilterMenu(menuType,atts_);
	iconFilterMenu_=new VParamFilterMenu(menuIcon,icons_);
	stateFilterMenu_=new VParamFilterMenu(menuState,states_,VParamFilterMenu::ColourDecor);

	//Sets the menu on the toolbutton
	viewTb->setMenu(menu);
}

void TreeNodeWidget::on_actionBreadcrumbs_toggled(bool b)
{
	bcWidget_->setVisible(b);
	//bcWidget_->clear();
}

void TreeNodeWidget::writeSettings(VSettings* vs)
{
	vs->put("type","tree");
	vs->put("dockId",id_);

	states_->writeSettings(vs);
	atts_->writeSettings(vs);
	icons_->writeSettings(vs);
}

void TreeNodeWidget::readSettings(VSettings* vs)
{
	std::string type=vs->get<std::string>("type","");
	if(type != "tree")
		return;

	//This will not emit the changed signal. So the "observers" will
	//not notice the change.
	states_->readSettings(vs);
	atts_->readSettings(vs);
	icons_->readSettings(vs);

	//The model at this point is inactive (not using its data). We make it active:
	//	-it will instruct its data provider to filter the data according
	//   to the current settings
	//	-it will load and display the data
	model_->active(true);

	attrFilterMenu_->reload();
	iconFilterMenu_->reload();
	stateFilterMenu_->reload();
}
