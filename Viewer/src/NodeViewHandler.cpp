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
	currentMode_=Viewer::NoViewMode;
}

void NodeViewHandler::add(Viewer::ViewMode mode,NodeViewBase* b,QWidget* w)
{
  	bases_[mode]=b;
	widgets_[mode]=w;
	stacked_->addWidget(w);
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

void NodeViewHandler::setCurrentMode(Viewer::ViewMode mode)
{
	if(mode==Viewer::NoViewMode)
	  	mode=Viewer::TreeViewMode;

	if(mode == currentMode_)
	  	return;

	NodeViewBase* b=base(currentMode_);

	NodeViewBase* bNew=base(mode);
	QWidget* wNew=widget(mode);

	//Set the mode
	currentMode_=mode;

	//Update the view
	if(wNew && bNew)
	{
	  	wNew->setEnabled(true);
		//if(b) bNew->changeFolder(b->currentFolder());

		//Set layout
		stacked_->setCurrentWidget(wNew);
	}

	//Disable the other views
	for(std::map<Viewer::ViewMode,QWidget*>::iterator it=widgets_.begin(); it != widgets_.end(); it++)
	  	if(it->first != currentMode_ && it->second)
		  	it->second->setEnabled(false);
}


