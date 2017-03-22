/***************************** LICENSE START ***********************************

 Copyright 2009-2017 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#include "TreeNodeWidget.hpp"

#include <QHBoxLayout>

#include "AbstractNodeModel.hpp"
#include "DashboardDock.hpp"
#include "NodePathWidget.hpp"
#include "NodeViewBase.hpp"
#include "TreeNodeModel.hpp"
#include "TreeNodeView.hpp"
#include "VFilter.hpp"
#include "VConfig.hpp"
#include "VModelData.hpp"
#include "VSettings.hpp"
#include "WidgetNameProvider.hpp"

#include "FilterWidget.hpp"

AttributeFilter* TreeNodeWidget::lastAtts_=NULL;

TreeNodeWidget::TreeNodeWidget(ServerFilter* serverFilter,QWidget* parent) : NodeWidget("tree",serverFilter,parent)
{
	//Init qt-creator form
	setupUi(this);

	if(!lastAtts_)
	{
		lastAtts_=new AttributeFilter();
	}

	initAtts();

    bcWidget_=new NodePathWidget(this);

	//This defines how to filter the nodes in the tree. We only want to filter according to node status.
	filterDef_=new NodeFilterDef(serverFilter_,NodeFilterDef::NodeStateScope);

	//Create the tree model. It uses the datahandler to access the data.
	model_=new TreeNodeModel(serverFilter_,filterDef_,atts_,icons_,this);

	//Create the view
	QHBoxLayout *hb=new QHBoxLayout(viewHolder_);
	hb->setContentsMargins(0,0,0,0);
	hb->setSpacing(0);
    TreeNodeView *tv=new TreeNodeView((TreeNodeModel*)model_,filterDef_,this);
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

    connect(model_,SIGNAL(clearBegun(const VTreeNode*)),
            view_->realWidget(),SLOT(slotSaveExpand(const VTreeNode*)));

    connect(model_,SIGNAL(scanEnded(const VTreeNode*)),
            view_->realWidget(),SLOT(slotRestoreExpand(const VTreeNode*)));

    connect(model_,SIGNAL(firstScanEnded(const VTreeServer*)),
            this,SLOT(firstScanEnded(const VTreeServer*)));

	connect(model_,SIGNAL(rerender()),
	        view_->realWidget(),SLOT(slotRerender()));

    connect(model_,SIGNAL(filterChangeBegun()),
            view_->realWidget(),SLOT(slotSaveExpand()));

    connect(model_,SIGNAL(filterChangeEnded()),
            view_->realWidget(),SLOT(slotRestoreExpand()));

    connect(model_,SIGNAL(filterUpdateRemoveBegun(const VTreeNode*)),
            view_->realWidget(),SLOT(slotSaveExpand(const VTreeNode*)));

    connect(model_,SIGNAL(filterUpdateAddEnded(const VTreeNode*)),
            view_->realWidget(),SLOT(slotRestoreExpand(const VTreeNode*)));

    connect(atts_,SIGNAL(changed()),
		   this,SLOT(slotAttsChanged()));

	//This will not emit the trigered signal of the action!!
	//Synchronise the action and the breadcrumbs state
    actionBreadcrumbs->setChecked(bcWidget_->isGuiMode());

	//The node status filter is exposed via a menu. So we need a reference to it.
	states_=filterDef_->nodeState();

    viewHolder_->setObjectName("h");
    WidgetNameProvider::nameChildren(this);
}

TreeNodeWidget::~TreeNodeWidget()
{
}

void TreeNodeWidget::initAtts()
{
	if(VProperty *prop=VConfig::instance()->find("view.tree.attributesPolicy"))
	{
		if(prop->valueAsString() == "last")
		{
			atts_->setCurrent(lastAtts_->current());
		}
		else if(VProperty *propDef=VConfig::instance()->find("view.tree.defaultAttributes"))
		{
			atts_->setCurrent(propDef->value().toString().split("/"));
		}
	}
}

void TreeNodeWidget::populateDockTitleBar(DashboardDockTitleWidget* tw)
{
	//Builds the menu for the settings tool button
	QMenu *menu=new QMenu(this);
	menu->setTearOffEnabled(true);

	menu->addAction(actionBreadcrumbs);
    QMenu *menuState=new QMenu(this);
    QMenu *menuType=new QMenu(this);
	QMenu *menuIcon=menu->addMenu(tr("Icon"));

	menuState->setTearOffEnabled(true);
	menuType->setTearOffEnabled(true);
	menuIcon->setTearOffEnabled(true);

	//stateFilterMenu_=new StateFilterMenu(menuState,filter_->menu());
	attrFilterMenu_=new VParamFilterMenu(menuType,atts_,"Show attributes",VParamFilterMenu::ShowMode);
    iconFilterMenu_=new VParamFilterMenu(menuIcon,icons_,"Show icons",VParamFilterMenu::ShowMode,
                                         VParamFilterMenu::PixmapDecor);
    stateFilterMenu_=new VParamFilterMenu(menuState,states_,"Show statuses",
                   VParamFilterMenu::ShowMode,VParamFilterMenu::ColourDecor);

	//Sets the menu on the toolbutton
	tw->optionsTb()->setMenu(menu);

    //Add the bc to the titlebar
    tw->setBcWidget(bcWidget_);

    QList<QAction*> acLst;

    QAction* acState=new QAction(this);
    acState->setIcon(QPixmap(":viewer/status.svg"));
    acState->setToolTip("Show statuses");
    acState->setMenu(menuState);
    acLst << acState;

    QAction* acAttr=new QAction(this);
    acAttr->setIcon(QPixmap(":viewer/attribute.svg"));
    acAttr->setToolTip("Show attributes");
    acAttr->setMenu(menuType);
    acLst << acAttr;

    tw->addActions(acLst);
}

void TreeNodeWidget::slotSelectionChangedInView(VInfo_ptr info)
{
	updateActionState(info);
	bcWidget_->setPath(info);
    if(broadcastSelection())
        Q_EMIT selectionChanged(info);
}


void TreeNodeWidget::on_actionBreadcrumbs_triggered(bool b)
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

void TreeNodeWidget::rerender()
{
	bcWidget_->rerender();
	view_->rerender();
}


bool TreeNodeWidget::initialSelectionInView()
{
    //Seeting the initail selection is probably unsuccessful because at
    //this point the servers are probably not fully loaded
    VInfo_ptr selInfo=VInfo::createFromPath(firstSelectionPath_);
    if(selInfo)
        view_->setCurrentSelection(selInfo);
    else
        view_->selectFirstServer();

    return true;
}

//When the first successful scan ended we try to set the initial selection
void TreeNodeWidget::firstScanEnded(const VTreeServer* s)
{
    VInfo_ptr selInfo=VInfo::createFromPath(firstSelectionPath_);
    if(selInfo && selInfo->server() == s->realServer())
    {
        view_->setCurrentSelection(selInfo);
    }
}

void TreeNodeWidget::slotAttsChanged()
{
	lastAtts_->setCurrent(atts_->current());
}

void TreeNodeWidget::writeSettings(VSettings* vs)
{
	vs->put("type",type_);
	vs->put("dockId",id_);

    VInfo_ptr sel=currentSelection();
    if(sel)
    {
        vs->put("selection",sel->storedPath());
    }

	bcWidget_->writeSettings(vs);

	states_->writeSettings(vs);
	atts_->writeSettings(vs);
    icons_->writeSettings(vs);

    DashboardWidget::writeSettings(vs);
}

void TreeNodeWidget::readSettings(VSettings* vs)
{
	std::string type=vs->get<std::string>("type","");
	if(type != type_)
		return;

    //The selection on last exit. We will use it later when the server is fully loaded.
    firstSelectionPath_=vs->get<std::string>("selection","");

	//This will not emit the changed signal. So the "observers" will
	//not notice the change.
	states_->readSettings(vs);
	atts_->readSettings(vs);
	icons_->readSettings(vs);

	lastAtts_->setCurrent(atts_->current());

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

	attrFilterMenu_->reload();
	iconFilterMenu_->reload();
	stateFilterMenu_->reload();

    DashboardWidget::readSettings(vs);
}
