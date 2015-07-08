//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "InfoPanelItem.hpp"

#include "ServerHandler.hpp"
#include "VNode.hpp"

#include <map>

static std::map<std::string,InfoPanelItemFactory*>* makers = 0;

InfoPanelItemFactory::InfoPanelItemFactory(const std::string& name)
{
	if(makers == 0)
		makers = new std::map<std::string,InfoPanelItemFactory*>;

	// Put in reverse order...
	(*makers)[name] = this;
}

InfoPanelItemFactory::~InfoPanelItemFactory()
{
	// Not called
}

InfoPanelItem* InfoPanelItemFactory::create(const std::string& name)
{
	std::map<std::string,InfoPanelItemFactory*>::iterator j = makers->find(name);
	if(j != makers->end())
		return (*j).second->make();

	//Default
	//return  new MvQTextLine(e,p);
	//return new MvQLineEditItem(e,p) ;
	return 0;
}

//=======================================================
//
// InfoPanelItem
//
//=======================================================

//Set the new VInfo object.
//We also we need to manage the node observers. The InfoItem 
//will be the observer of the server of the object stored in
//the new VInfo
void InfoPanelItem::adjust(VInfo_ptr info)
{
  	//Check if there is data in info
  	if(info.get())
  	{
  		ServerHandler *server=info->server();

  		bool sameServer=(info_)?(info_->server() == server):false;

  		//Handle observers
  		if(!sameServer)
  		{
  			if(info_ && info_->server())
  			{
  				info_->server()->removeNodeObserver(this);
  			}
  			info->server()->addNodeObserver(this);
  		}
  	}
  	//If the there is no data we clean everything and return
  	else
  	{
  	  	if(info_ && info_->server())
  	  	{
  	  		info_->server()->removeNodeObserver(this);
  	  	}
  	}

  	//Set the info 
  	info_=info;
}

void InfoPanelItem::clear()
{
	if(info_ && info_->server())
  	{
  	  	//info_->server()->removeServerObserver(this);
  	  	info_->server()->removeNodeObserver(this);
  	}

	loaded_=false;
}

void InfoPanelItem::setFrozen(bool b)
{

}

void InfoPanelItem::setDetached(bool b)
{

}

//From NodeObserver
void InfoPanelItem::notifyBeginNodeChange(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect,const VNodeChange&)
{
	if(!loaded_)
        return;
    
    //Check if there is data in info
	if(info_.get())
	{
		if(info_->isNode())
		{
			//Check if the updated node is handled by the item
            if(info_->node() == node ||
              (useAncestors_ && info_->node()->isAncestor(node)))
            {
			    //We call the method implemented in the concrete class 
                //to handle the changes
                nodeChanged(node,aspect);
				return;
            }
		}
	}
}
