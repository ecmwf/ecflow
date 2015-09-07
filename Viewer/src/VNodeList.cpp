//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VNodeList.hpp"

#include "ServerHandler.hpp"
#include "VNode.hpp"

#include <algorithm>

VNodeList::VNodeList(QObject *parent) : QObject(parent)
{
}

VNodeList::~VNodeList()
{
	clear();
}

void VNodeList::add(VNode *node)
{
	std::vector<VNode*>::const_iterator it=std::find(data_.begin(),data_.end(),node);
	if(it != data_.end())
		return;

	ServerHandler *s=node->server();
	if(s)
	{
		s->addServerObserver(this);
		s->addNodeObserver(this);
	}

	Q_EMIT beginAppendRow();
	data_.push_back(node);
	Q_EMIT endAppendRow();
}

void VNodeList::clear()
{
	Q_EMIT beginReset();

	for(std::vector<VNode*>::const_iterator it=data_.begin(); it != data_.end(); it++)
	{
		if(ServerHandler *s=(*it)->server())
		{
			s->removeServerObserver(this);
			s->removeNodeObserver(this);
		}
	}

	data_.clear();

	Q_EMIT endReset();
}

void VNodeList::clear(ServerHandler* server)
{
	std::vector<VNode*> prev=data_;
	data_.clear();

	for(std::vector<VNode*>::const_iterator it=prev.begin(); it != prev.end(); it++)
	{
		ServerHandler *s=(*it)->server();
		if(s && s==server)
		{
			s->removeServerObserver(this);
			s->removeNodeObserver(this);
		}
		else
		{
			data_.push_back(*it);
		}
	}
}

void VNodeList::notifyServerDelete(ServerHandler* server)
{
	Q_EMIT beginReset();
	clear(server);
	Q_EMIT endReset();
}

void VNodeList::notifyBeginServerClear(ServerHandler* server)
{
	Q_EMIT beginReset();
	clear(server);
}

void VNodeList::notifyEndServerClear(ServerHandler* server)
{
	Q_EMIT endReset();
}

void VNodeList::notifyBeginNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&)
{

}

void VNodeList::notifyEndNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&)
{
}
