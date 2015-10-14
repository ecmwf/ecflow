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

//========================================================
//
//  VNodeListItem
//
//========================================================

VNodeListItem::VNodeListItem(VNode* n) :node_(n)
{
	if(n)
	{
		server_=n->server()->name();
		path_=n->absNodePath();
		time_=QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
	}
}

bool VNodeListItem::sameAs(VNode *node) const
{
	if(!node) return false;

	if(node_)
		return (node_ == node);

	return (server_== node->server()->name() && path_ == node->absNodePath());
}

void VNodeListItem::invalidateNode()
{
	node_=NULL;
}

bool VNodeListItem::updateNode(ServerHandler* s)
{
	if(node_)
		return (node_->server() == s);
	else if(s->name() == server_)
	{
		node_=s->vRoot()->find(path_);
		return (node_ != NULL);
	}

	return false;
}

//========================================================
//
//  VNodeList
//
//========================================================

VNodeList::VNodeList(QObject *parent) :
   QObject(parent),
   maxNum_(200)
{
}

VNodeList::~VNodeList()
{
	clear();
}

VNodeListItem* VNodeList::itemAt(int i)
{
	if(i >= 0 && i < data_.size())
		return data_.at(i);

	return NULL;
}

void VNodeList::setMaxNum(int maxNum)
{
	if(maxNum_ != maxNum)
	{
		assert(maxNum>0);
		maxNum_=maxNum;
		trim();
	}
}

void VNodeList::add(VNode *node)
{
	if(contains(node))
		return;

	trim();

	ServerHandler *s=node->server();
	if(!s)
		return;

	attach(s);

	Q_EMIT beginAppendRow();
	data_.push_back(new VNodeListItem(node));
	serverCnt_[s]++;
	Q_EMIT endAppendRow();
}

void VNodeList::remove(VNode *node)
{
	for(std::vector<VNodeListItem*>::iterator it=data_.begin(); it != data_.end(); it++)
	{
		if((*it)->sameAs(node))
		{
			int row=it -data_.begin();

			Q_EMIT beginRemoveRow(row);
			delete *it;
			data_.erase(it);

			std::map<ServerHandler*,int>::iterator it=serverCnt_.find(node->server());
			if(it != serverCnt_.end())
			{
				it->second--;
				if(it->second == 0)
				{
					detach(node->server());
				}
			}

			Q_EMIT endRemoveRow(row);

			return;
		}
	}
}

void VNodeList::trim()
{
	int cnt=data_.size();
	bool doTrim=(cnt >0 && cnt > maxNum_);

	if(!doTrim)
		return;

	Q_EMIT beginRemoveRows(0,cnt-maxNum_-1);

	for(int row=cnt-1; row >= maxNum_; row--)
	{
		VNode *node=data_.front()->node();

		delete data_.front();

		if(node)
		{
			std::map<ServerHandler*,int>::iterator it=serverCnt_.find(node->server());
			if(it != serverCnt_.end())
			{
				it->second--;
				if(it->second == 0)
				{
					detach(node->server());
				}
			}
		}

		data_.erase(data_.begin());
	}

	Q_EMIT endRemoveRows(0,cnt-maxNum_-1);
}

bool VNodeList::contains(VNode *node)
{
	for(std::vector<VNodeListItem*>::const_iterator it=data_.begin(); it != data_.end(); it++)
	{
		if((*it)->sameAs(node))
			return true;
	}

	return false;
}


void VNodeList::clear()
{
	Q_EMIT beginReset();

	for(std::map<ServerHandler*,int>::const_iterator it=serverCnt_.begin(); it != serverCnt_.end(); it++)
	{
		it->first->removeServerObserver(this);
		it->first->removeNodeObserver(this);
	}
	serverCnt_.clear();

	for(std::vector<VNodeListItem*>::const_iterator it=data_.begin(); it != data_.end(); it++)
	{
		delete *it;
	}

	data_.clear();

	Q_EMIT endReset();
}

void VNodeList::clear(ServerHandler* server)
{
	std::vector<VNodeListItem*> prev=data_;
	data_.clear();

	detach(server);

	for(std::vector<VNodeListItem*>::const_iterator it=prev.begin(); it != prev.end(); ++it)
	{
		ServerHandler *s=(*it)->node_->server();
		if((*it)->server() == s->name())
		{
			delete *it;
		}
		else
		{
			data_.push_back(*it);
		}
	}
}

void VNodeList::serverClear(ServerHandler* server)
{
	for(std::vector<VNodeListItem*>::const_iterator it=data_.begin(); it != data_.end(); ++it)
	{
		if(server->name() == (*it)->server())
		{
			(*it)->invalidateNode();
		}
	}
}

void VNodeList::serverScan(ServerHandler* server)
{
	std::vector<VNodeListItem*> prev=data_;
	data_.clear();

	serverCnt_[server]=0;

	for(std::vector<VNodeListItem*>::const_iterator it=prev.begin(); it != prev.end(); ++it)
	{
		if(server->name() == (*it)->server())
		{
			if((*it)->updateNode(server))
			{
				data_.push_back(*it);
				serverCnt_[server]++;
			}
			else
				delete *it;
		}
	}

	if(serverCnt_[server] == 0)
		detach(server);
}


void VNodeList::attach(ServerHandler *s)
{
	if(serverCnt_.find(s) != serverCnt_.end())
	{
		s->addServerObserver(this);
		s->addNodeObserver(this);
		serverCnt_[s]=0;
	}
}

void VNodeList::detach(ServerHandler* s)
{
	std::map<ServerHandler*,int>::iterator it=serverCnt_.find(s);
	if(it != serverCnt_.end())
	{
		serverCnt_.erase(it);
		s->removeServerObserver(this);
		s->removeNodeObserver(this);
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
	serverClear(server);
}

void VNodeList::notifyEndServerClear(ServerHandler* server)
{
}

void VNodeList::notifyBeginServerScan(ServerHandler* server,const VServerChange&)
{
	Q_EMIT beginReset();
}

void VNodeList::notifyEndServerScan(ServerHandler* server)
{
	serverScan(server);
	Q_EMIT endReset();
}


void VNodeList::notifyBeginNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&)
{

}

void VNodeList::notifyEndNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&)
{
}
