/***************************** LICENSE START ***********************************

 Copyright 2013 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#include "NodeViewHandler.hpp"
#include "NodeViewBase.hpp"

#include <QStackedLayout>
#include <QWidget>


NodeViewHandler::NodeViewHandler(QStackedLayout* p) : stacked_(p)
{
	currentMode_=Viewer::TreeViewMode;
}

void NodeViewHandler::add(Viewer::ViewMode mode,NodeViewBase* b)
{
	//Check if the same widget/base has been already set for another mode
	Viewer::ViewMode parentMode=mode;
	for(std::map<Viewer::ViewMode,NodeViewBase*>::const_iterator it=bases_.begin(); it != bases_.end() ; it++)
		if(it->second == b)
		{
			parentMode=it->first;
			break;
		}

	bases_[mode]=b;
	QWidget *w=b->realWidget();
	widgets_[mode]=w;

	int cnt=stacked_->count();
	if(parentMode == mode)
	{
		stacked_->addWidget(w);
		indexes_[mode]=cnt;
	}
	//If the widget is already in the stack we just register its layout index
	else
	{
		indexes_[mode]=indexes_[parentMode];
	}
}

NodeViewBase* NodeViewHandler::base(Viewer::ViewMode mode) const
{
   	std::map<Viewer::ViewMode,NodeViewBase*>::const_iterator it=bases_.find(mode);
	return (it != bases_.end())?it->second:0;
}

QWidget* NodeViewHandler::widget(Viewer::ViewMode mode)
{
   	std::map<Viewer::ViewMode,QWidget*>::iterator it=widgets_.find(mode);
	return (it != widgets_.end())?it->second:0;
}

bool NodeViewHandler::setCurrentMode(Viewer::ViewMode mode)
{
	bool retVal=false;

	//Check if currentMode is valid
	if(bases_.find(currentMode_) == bases_.end())
	{
		//If the currentMode is invalid we set it to icon view
		currentMode_=Viewer::TreeViewMode;

		//Check if there is icon view
		if(bases_.find(currentMode_) == bases_.end())
		{
			//If there area any views defined we set current to the first
			if(bases_.size() > 0)
				currentMode_=bases_.begin()->first;
			//Otherwise something really bad happened!
			else
				return false;
		}
	}

	if(mode != currentMode_ && bases_.find(mode) != bases_.end())
	{
		QWidget* wNew=widget(mode);

		//Set the mode
		currentMode_=mode;

		//Update the view
		if(wNew)
		{
				wNew->setEnabled(true);
		}
	}

	//Update the folder in the current view
	if(NodeViewBase* b=base(currentMode_))
	{
		//If changing the folder is successfull it calls reset and returns true
		//retVal=b->changeFolder(folder,iconSize);
		//We need to reset if the
		//folder change was not successful (we stayed in the same folder)
		if(!retVal)
		{
			//b->doReset();
		}
	}

	//Set the layout
	stacked_->setCurrentIndex(indexes_[currentMode_]);

	//Disable the other views
	QWidget *currentWidget=widget(currentMode_);
	for(std::map<Viewer::ViewMode,QWidget*>::iterator it=widgets_.begin(); it != widgets_.end(); it++)
		  if(it->first != currentMode_ && it->second && it->second != currentWidget)
			  it->second->setEnabled(false);

	return retVal;
}

bool NodeViewHandler::setCurrentMode(int id)
{
	Viewer::ViewMode m=static_cast<Viewer::ViewMode>(id); //need a proper way to do it
	return setCurrentMode(m);
}


QList<QWidget*> NodeViewHandler::uniqueWidgets()
{
  	QList<QWidget*> lst;
  	for(std::map<Viewer::ViewMode,QWidget*>::iterator it=widgets_.begin(); it != widgets_.end(); it++)
	{
	  	if(lst.indexOf(it->second) == -1)
	  		lst << it->second;
	}
	return lst;
}

ViewNodeInfo_ptr NodeViewHandler::currentSelection()
{
	if(NodeViewBase* b=currentBase())
	{
		return b->currentSelection();
	}

	return ViewNodeInfo_ptr();
}
