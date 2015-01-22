//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VFilter.hpp"

#include "VNState.hpp"
#include "VAttribute.hpp"
#include "VIcon.hpp"
#include "VParam.hpp"
#include "VSettings.hpp"

#include "ServerHandler.hpp"

//==============================================
//
// VFilter
//
//==============================================

VFilter::VFilter(VConfig * owner) : VConfigItem(owner)
{
}

void VFilter::init(const std::vector<VParam*>& items)
{
	for(std::vector<VParam*>::const_iterator it=items.begin(); it != items.end(); it++)
	{
		all_.insert((*it));
	}
}

bool VFilter::isSet(VParam* p) const
{
	return (current_.find(p) != current_.end());
}

bool VFilter::isSet(const std::string &name) const
{
	for(std::set<VParam*>::const_iterator it=current_.begin(); it != current_.end(); it++)
	{
		if((*it)->name() == name)
				return true;
	}
	return false;
}

void VFilter::current(const std::set<std::string>& names)
{
	current_.clear();
	for(std::set<VParam*>::const_iterator it=all_.begin(); it != all_.end(); it++)
	{
			if(names.find((*it)->name()) != names.end())
				current_.insert(*it);
	}
	notifyOwner();
}

void VFilter::writeSettings(VSettings *vs)
{
	std::vector<std::string> array;

	for(std::set<VParam*>::const_iterator it=current_.begin(); it != current_.end(); it++)
	{
		array.push_back((*it)->name());
	}

	vs->put(settingsId_,array);
}

void VFilter::readSettings(VSettings* vs)
{
	current_.clear();

	std::vector<std::string> array;
	vs->get(settingsId_,array);

	for(std::vector<std::string>::const_iterator it = array.begin(); it != array.end(); ++it)
	{
		std::string name=*it;
		for(std::set<VParam*>::const_iterator itA=all_.begin(); itA != all_.end(); itA++)
		{
			if((*itA)->name() == name)
					current_.insert(*itA);
		}
	}

/*	current_.clear();

	for(boost::property_tree::ptree::const_iterator it = array.begin(); it != array.end(); ++it)
	{
			std::string name=it->second.get_value<std::string>();
			for(std::set<VParam*>::const_iterator it=all_.begin(); it != all_.end(); it++)
			{
					if((*it)->name() == name)
						current_.insert(*it);
			}
	}*/
}

//==============================================
//
// StateFilter
//
//==============================================

StateFilter::StateFilter(VConfig * owner) : VFilter(owner)
{
	settingsId_="state";
	std::vector<VParam*> v=VNState::filterItems();
	init(v);
	current_=all_;
}

void StateFilter::notifyOwner()
{
	owner_->changed(this);
}

//==============================================
//
// AttributeFilter
//
//==============================================

AttributeFilter::AttributeFilter(VConfig * owner) : VFilter(owner)
{
	settingsId_="attribute";
	std::vector<VParam*> v=VAttribute::filterItems();
	init(v);
}

void AttributeFilter::notifyOwner()
{
	owner_->changed(this);
}

//==============================================
//
// IconFilter
//
//==============================================

IconFilter::IconFilter(VConfig * owner) : VFilter(owner)
{
	settingsId_="icon";
	std::vector<VParam*> v=VIcon::filterItems();
	init(v);
	current_=all_;
}

void IconFilter::notifyOwner()
{
	owner_->changed(this);
}



NodeFilter::NodeFilter()
{

}

TreeNodeFilter::TreeNodeFilter()
{

}

bool TreeNodeFilter::isFiltered(Node* node)
{
	return (std::find(nonMatch_.begin(), nonMatch_.end(), node) == nonMatch_.end());
}

void TreeNodeFilter::reset(ServerHandler* server,VFilter* sf)
{
	nonMatch_.clear();

	//If all states are visible
	if(sf->isComplete())
		return;

	for(unsigned int j=0; j < server->numSuites();j++)
	{
		filterState(server->suiteAt(j),sf);
	}
}

bool TreeNodeFilter::filterState(node_ptr node,VFilter* stateFilter)
{
	bool ok=false;
	if(stateFilter->isSet(VNState::toState(node.get())))
	{
		ok=true;
	}

	std::vector<node_ptr> nodes;
	node->immediateChildren(nodes);

	for(std::vector<node_ptr>::iterator it=nodes.begin(); it != nodes.end(); it++)
	{
		if(filterState(*it,stateFilter) == true && ok == false)
		{
			ok=true;
		}
	}

	if(!ok)
		nonMatch_.insert(node.get());

	return ok;
}


TableNodeFilter::TableNodeFilter()
{
	type_.insert("suite");
}

bool TableNodeFilter::isFiltered(Node* node)
{
	return (std::find(match_.begin(), match_.end(), node) != match_.end());
}

void TableNodeFilter::reset(ServerHandler* server,VFilter* sf)
{
	match_.clear();

	//If all states are visible
	//if(sf->isComplete())
	//	return;

	for(unsigned int j=0; j < server->numSuites();j++)
	{
		match_.push_back(server->suiteAt(j).get());
	}
}

Node* TableNodeFilter::match(int i)
{
		assert(i>=0 && i < match_.size());
		return match_.at(i);
}


