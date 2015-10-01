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

#include <QDateTime>

VNodeList::VNodeList(QObject *parent) : QObject(parent)
{
}

VNodeList::~VNodeList()
{
	clear();
}

VNodeListItem* VNodeList::itemAt(int i)
{
	if(i > 0 && i < data_.size())
		return data_.at(i);

	return 0;
}

void VNodeList::add(VNode *node)
{
	if(contains(node))
		return;

	ServerHandler *s=node->server();
	if(s)
	{
		s->addServerObserver(this);
		s->addNodeObserver(this);
	}

	Q_EMIT beginAppendRow();
	data_.push_back(new VNodeListItem(node,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
	Q_EMIT endAppendRow();
}

void VNodeList::remove(VNode *node)
{
	for(std::vector<VNodeListItem*>::iterator it=data_.begin(); it != data_.end(); it++)
	{
		if((*it)->node_ == node)
		{
			int row=it -data_.begin();

			Q_EMIT beginRemoveRow(row);
			delete *it;
			data_.erase(it);
			Q_EMIT endRemoveRow(row);
			return;
		}
	}
}

bool VNodeList::contains(VNode *node)
{
	for(std::vector<VNodeListItem*>::const_iterator it=data_.begin(); it != data_.end(); it++)
	{
		if((*it)->node_ == node)
			return true;
	}
	return false;
}

void VNodeList::hide()
{
	clearData(true);
}

void VNodeList::clear()
{
	clearData(false);
}

void VNodeList::clearData(bool hideOnly)
{
	Q_EMIT beginReset();

	if(!hideOnly)
	{
		for(std::vector<VNodeListItem*>::const_iterator it=data_.begin(); it != data_.end(); it++)
		{
			if(ServerHandler *s=(*it)->node_->server())
			{
				s->removeServerObserver(this);
				s->removeNodeObserver(this);
			}

			delete *it;
		}

		data_.clear();
	}
	else
	{
		for(std::vector<VNodeListItem*>::const_iterator it=data_.begin(); it != data_.end(); it++)
			(*it)->visible_=false;
	}

	Q_EMIT endReset();
}

void VNodeList::clear(ServerHandler* server)
{
	std::vector<VNodeListItem*> prev=data_;
	data_.clear();

	for(std::vector<VNodeListItem*>::const_iterator it=prev.begin(); it != prev.end(); ++it)
	{
		ServerHandler *s=(*it)->node_->server();
		if(s && s==server)
		{
			s->removeServerObserver(this);
			s->removeNodeObserver(this);
			delete *it;
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
