/***************************** LICENSE START ***********************************

 Copyright 2013 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#include "NodeViewHandler.hpp"
#include "NodeViewBase.hpp"

#include "AbstractNodeModel.hpp"
#include "NodeFilterModel.hpp"
#include "TableNodeModel.hpp"
#include "TableNodeView.hpp"
#include "TreeNodeModel.hpp"
#include "TreeNodeView.hpp"

#include <QStackedLayout>
#include <QVBoxLayout>
#include <QWidget>

QWidget* NodeWidget::widget()
{
	return view_->realWidget();
}

VInfo_ptr NodeWidget::currentSelection()
{
	return view_->currentSelection();
}

void NodeWidget::currentSelection(VInfo_ptr info)
{
	view_->currentSelection(info);
}

void NodeWidget::reload()
{
	model_->reload();
}

void NodeWidget::active(bool b)
{
	model_->active(b);
	view_->realWidget()->setEnabled(b);
}

bool NodeWidget::active() const
{
	return model_->active();
}

TreeNodeWidget::TreeNodeWidget(VConfig* config,QWidget* parent)
{
	QVBoxLayout* layout=new QVBoxLayout(this);
	layout->setSpacing(0);
	layout->setContentsMargins(1,1,1,1);

	model_=new TreeNodeModel(config,parent);
	filterModel_=new NodeFilterModel(model_,parent);
	view_= new TreeNodeView(filterModel_,parent);

	layout->addWidget(view_->realWidget());
}

TableNodeWidget::TableNodeWidget(VConfig* config,QWidget * parent)
{
	QVBoxLayout* layout=new QVBoxLayout(this);
	layout->setSpacing(0);
	layout->setContentsMargins(1,1,1,1);

	model_=new TableNodeModel(config,parent);
	filterModel_=new NodeFilterModel(model_,parent);
	view_= new TableNodeView(filterModel_,parent);

	layout->addWidget(view_->realWidget());
}

//===========================================================
//
// NodeViewhandler
//
//===========================================================

NodeViewHandler::NodeViewHandler(QStackedLayout* p) : stacked_(p)
{
	currentMode_=Viewer::NoViewMode;
}

void NodeViewHandler::add(Viewer::ViewMode mode,NodeWidget* c)
{
	QWidget *w=c->view()->realWidget();

	int cnt=stacked_->count();
	stacked_->addWidget(w);
	indexes_[mode]=cnt;
	controls_[mode]=c;
}

NodeWidget* NodeViewHandler::control(Viewer::ViewMode mode) const
{
   	std::map<Viewer::ViewMode,NodeWidget*>::const_iterator it=controls_.find(mode);
	return (it != controls_.end())?it->second:0;
}

bool NodeViewHandler::setCurrentMode(Viewer::ViewMode mode)
{
	bool retVal=false;

	//Check if currentMode is valid
	if(currentMode_!= Viewer::NoViewMode && controls_.find(currentMode_) == controls_.end())
	{
		//If the currentMode is invalid we set it to Tree View Mode
		currentMode_=Viewer::TreeViewMode;

		//Check if there is tree view view
		if(controls_.find(currentMode_) == controls_.end())
		{
			//If there are any views defined we set current to the first
			if(controls_.size() > 0)
				currentMode_=controls_.begin()->first;
			//Otherwise something really bad happened!
			else
				return false;
		}
	}

	//Set the mode
	currentMode_=mode;

	//Disable the other views
	NodeWidget *cnt=control(currentMode_);

	//Set the layout
	stacked_->setCurrentIndex(indexes_[currentMode_]);

	//Deactivate the other views
	for(std::map<Viewer::ViewMode,NodeWidget*>::iterator it=controls_.begin(); it != controls_.end(); it++)
		if(it->first != currentMode_)
			it->second->active(false);

	//Activate the current view
	cnt->active(true);

	return retVal;
}

bool NodeViewHandler::setCurrentMode(int id)
{
	Viewer::ViewMode m=static_cast<Viewer::ViewMode>(id); //need a proper way to do it
	return setCurrentMode(m);
}
