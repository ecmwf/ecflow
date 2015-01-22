//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "NodeModelData.hpp"

#include "VFilter.hpp"
#include "ServerHandler.hpp"

//==========================================
//
// NodeModelData
//
//==========================================


NodeModelData::NodeModelData(ServerHandler *server,NodeFilter* filter) :
   server_(server),
   filter_(filter),
   nodeNum_(-1)
{
}

NodeModelData::~NodeModelData()
{
	if(filter_)
		delete filter_;
}

int NodeModelData::nodeNum() const
{
	if(nodeNum_==-1)
		nodeNum_=server_->numberOfNodes();

	return nodeNum_;
}

void NodeModelData::filter(VFilter* stateFilter)
{
	if(filter_)
			filter_->reset(server_,stateFilter);

}

//==========================================
//
// NodeModelDataHandler
//
//==========================================

NodeModelData* NodeModelDataHandler::data(int n) const
{
	return (n >=0 && n < data_.size())?data_.at(n):0;
}

ServerHandler* NodeModelDataHandler::server(int n) const
{
	return (n >=0 && n < data_.size())?data_.at(n)->server_:0;
}

ServerHandler* NodeModelDataHandler::server(void* idPointer) const
{
	for(unsigned int i=0; i < data_.size(); i++)
		if(data_.at(i)->server_ == idPointer)
			return data_.at(i)->server_;

	return NULL;
}

void NodeModelDataHandler::add(ServerHandler *server,NodeFilter* filter)
{
	data_.push_back(new NodeModelData(server,filter));
}

void NodeModelDataHandler::clear()
{
	for(unsigned int i=0; i < data_.size(); i++)
		delete data_.at(i);

	data_.clear();
}

int NodeModelDataHandler::indexOf(ServerHandler* s) const
{
	for(unsigned int i=0; i < data_.size(); i++)
		if(data_.at(i)->server_ == s)
			return i;
	return -1;
}

bool NodeModelDataHandler::isFiltered(Node *node) const
{
	ServerHandler* server=ServerHandler::find(node);
	int id=indexOf(server);
	if(id != -1 && data_.at(id)->filter_)
	{
		return data_.at(id)->filter_->isFiltered(node);
	}

	return true;
}

int NodeModelDataHandler::numOfNodes(int index) const
{
	if(NodeModelData *d=data(index))
	{
		return d->nodeNum();
	}
	return 0;
}

void NodeModelDataHandler::filter(VFilter *stateFilter)
{
	for(unsigned int i=0; i < data_.size(); i++)
	{
		data_.at(i)->filter(stateFilter);
	}
}

Node* NodeModelDataHandler::getNodeFromFilter(int totalRow)
{
	int cnt=0;

	for(unsigned int i=0; i < data_.size(); i++)
	{
		NodeFilter *filter=data_.at(i)->filter_;
		if(totalRow-cnt < filter->matchCount())
		{
			return filter->match(totalRow-cnt);
		}
		cnt+=filter->matchCount();
	}

	return NULL;
}

int NodeModelDataHandler::numOfFiltered(int index) const
{
	if(NodeModelData *d=data(index))
	{
		if(d->filter_)
			return d->filter_->matchCount();
	}
	return 0;
}
