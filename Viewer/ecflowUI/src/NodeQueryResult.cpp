//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "NodeQueryResult.hpp"

#include <QDebug>

#include "ServerHandler.hpp"
#include "VNode.hpp"

//========================================================
//
//  NodeQueryResultItem
//
//========================================================

NodeQueryResultItem::NodeQueryResultItem(VNode* node)  :
	node_(node),
	server_(nullptr)
{
	if(node_)
		server_=node_->server();
}

NodeQueryResultItem::NodeQueryResultItem(NodeQueryResultTmp_ptr d)
{
	node_=d->node_;
	attr_=d->attr_;

	if(node_)
		server_=node_->server();
}

void NodeQueryResultItem::invalidateNode()
{
	assert(node_);
	path_=node_->absNodePath();
	node_=nullptr;
}

bool NodeQueryResultItem::updateNode()
{
	if(node_)
		return (node_->server() != nullptr);

	else
	{
		node_=server_->vRoot()->find(path_);
		path_.clear();
		return (node_ != nullptr);
	}

	return false;
}

QString NodeQueryResultItem::serverStr() const
{
	return (server_)?QString::fromStdString(server_->name()):QString();
}

QString NodeQueryResultItem::pathStr() const
{
	if(node_)
		return QString::fromStdString(node_->absNodePath());

	return QString::fromStdString(path_);
}

QString NodeQueryResultItem::typeStr() const
{
	if(node_)
		return QString::fromStdString(node_->nodeType());

	return QString("???");
}

QString NodeQueryResultItem::stateStr() const
{
	if(node_)
		return node_->stateName();
	return QString("???");
}

QColor NodeQueryResultItem::stateColour() const
{
	if(node_)
		return node_->stateColour();
	return {Qt::transparent};
}

QString NodeQueryResultItem::stateChangeTimeAsString() const
{
    QString s;
    if(node_)
        node_->statusChangeTime(s);

    return s;
}

unsigned int NodeQueryResultItem::stateChangeTime() const
{
    if(node_)
        return node_->statusChangeTime();

    return 0;
}

QStringList NodeQueryResultItem::attr() const
{
    return attr_;
}

void NodeQueryResultBlock::add(VNode* node,int pos)
{
	if(pos_==-1)
		pos_=pos;

	cnt_++;

	if(nodes_[node].pos_==-1)
	{
		nodes_[node].pos_=pos;
	}
	nodes_[node].cnt_++;
}

void NodeQueryResultBlock::clear()
{
	pos_=-1;
	cnt_=0;
	nodes_.clear();
}

bool NodeQueryResultBlock::find(const VNode* nc,int &pos, int &cnt)
{
	QHash<VNode*,Pos>::const_iterator it = nodes_.find(const_cast<VNode*>(nc));
	if(it != nodes_.end())
	{
		pos=it.value().pos_;
		cnt=it.value().cnt_;
		return true;
	}

	return false;
}

//========================================================
//
//  NodeQueryResult
//
//========================================================

NodeQueryResult::NodeQueryResult(QObject *parent) :
   QObject(parent)
{
}

NodeQueryResult::~NodeQueryResult()
{
	clear();
}

NodeQueryResultItem* NodeQueryResult::itemAt(int i)
{
    if(i >= 0 && i < static_cast<int>(data_.size()))
		return data_.at(i);

	return nullptr;
}

void NodeQueryResult::add(NodeQueryResultTmp_ptr item)
{
	ServerHandler *s=item->node_->server();
	if(!s)
		return;

	attach(s);

	Q_EMIT beginAppendRow();

    data_.push_back(new NodeQueryResultItem(item));
	blocks_[s].add(item->node_,data_.size()-1);

	Q_EMIT endAppendRow();
}

void NodeQueryResult::add(QList<NodeQueryResultTmp_ptr> items)
{
	if(items.count() == 0)
		return;

	Q_EMIT beginAppendRows(items.count());

	for(int i=0; i < items.count(); i++)
	{
        VNode *node=items[i]->node_;
		ServerHandler *s=node->server();
		attach(s);
        data_.push_back(new NodeQueryResultItem(items[i]));
		blocks_[s].add(node,data_.size()-1);
	}

	Q_EMIT endAppendRows(items.count());
}

void NodeQueryResult::add(std::vector<VInfo_ptr> items)
{
	if(items.size() == 0)
		return;

    //Count the needed items
    int num=0;
    for(auto & item : items)
	{   
        //assert(items.at(i) && items.at(i).get());
        if(item->isServer() || item->isNode())
        {
            num++;
        }
    }
        
	Q_EMIT beginAppendRows(items.size());

	for(auto & item : items)
	{           
        if(item->isServer() || item->isNode())
        {
            VNode *node=item->node();
            ServerHandler *s=item->server();
            attach(s);
            data_.push_back(new NodeQueryResultItem(node));
            blocks_[s].add(node,data_.size()-1);
        }
	}

	Q_EMIT endAppendRows(items.size());
}
void NodeQueryResult::clear()
{
	Q_EMIT beginReset();

	for(std::map<ServerHandler*,NodeQueryResultBlock>::const_iterator it=blocks_.begin(); it != blocks_.end(); it++)
	{
		it->first->removeServerObserver(this);
		it->first->removeNodeObserver(this);
	}
	blocks_.clear();

	for(std::vector<NodeQueryResultItem*>::const_iterator it=data_.begin(); it != data_.end(); it++)
	{
		delete *it;
	}

	data_.clear();

	Q_EMIT endReset();
}

void NodeQueryResult::clear(ServerHandler* server)
{
	std::vector<NodeQueryResultItem*> prev=data_;
	data_.clear();

	//Adjust the servers
	detach(server);

	for(auto & block : blocks_)
	{
		block.second.clear();
	}

	//Adjust data
	for(std::vector<NodeQueryResultItem*>::const_iterator it=prev.begin(); it != prev.end(); ++it)
	{
		ServerHandler *s=(*it)->node_->server();
		if(s == server)
		{
			delete *it;
		}
		else
		{
			data_.push_back(*it);
			blocks_[s].add((*it)->node_,data_.size()-1);
		}
	}
}

void NodeQueryResult::serverClear(ServerHandler* server)
{
	for(std::vector<NodeQueryResultItem*>::const_iterator it=data_.begin(); it != data_.end(); ++it)
	{
		if(server == (*it)->server_)
		{
			(*it)->invalidateNode();
		}
	}
}

void NodeQueryResult::serverScan(ServerHandler* server)
{
	std::vector<NodeQueryResultItem*> prev=data_;
	data_.clear();

	for(auto & block : blocks_)
	{
		block.second.clear();
	}

	for(std::vector<NodeQueryResultItem*>::const_iterator it=prev.begin(); it != prev.end(); ++it)
	{
		if(server == (*it)->server_)
		{
			if((*it)->updateNode())
			{
				data_.push_back(*it);
				blocks_[server].add((*it)->node_,data_.size()-1);
			}
			else
				delete *it;
		}
		else
		{
			data_.push_back(*it);
			blocks_[server].add((*it)->node_,data_.size()-1);
		}
	}

	if(blocks_[server].cnt_ == 0)
		detach(server);
}


void NodeQueryResult::attach(ServerHandler *s)
{
	if(blocks_.find(s) == blocks_.end())
	{
		s->addServerObserver(this);
		s->addNodeObserver(this);
	}
}

void NodeQueryResult::detach(ServerHandler* s)
{
	auto it=blocks_.find(s);
	if(it != blocks_.end())
	{
		blocks_.erase(it);

		s->removeServerObserver(this);
		s->removeNodeObserver(this);
	}
}

/*void NodeQueryResult::detach(VNode *node)
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
}*/

void NodeQueryResult::notifyServerDelete(ServerHandler* server)
{
	Q_EMIT beginReset();
	clear(server);
	Q_EMIT endReset();
}

void NodeQueryResult::notifyBeginServerClear(ServerHandler* server)
{
	serverClear(server);
}

void NodeQueryResult::notifyEndServerClear(ServerHandler* server)
{
}

void NodeQueryResult::notifyBeginServerScan(ServerHandler* server,const VServerChange&)
{
	Q_EMIT beginReset();
}

void NodeQueryResult::notifyEndServerScan(ServerHandler* server)
{
	serverScan(server);
	Q_EMIT endReset();
}


void NodeQueryResult::notifyBeginNodeChange(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect,const VNodeChange& vn)
{
	bool changed=false;
	for(auto it : aspect)
	{
		//Changes in the nodes
		if(it == ecf::Aspect::STATE || it == ecf::Aspect::SUSPENDED)
		{
			changed=true;
			break;
		}
	}

	if(changed)
	{
		int pos=-1;
		int cnt=0;
		if(range(node,pos,cnt))
			Q_EMIT stateChanged(node,pos,cnt);
	}
}

void NodeQueryResult::notifyEndNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&)
{

}

//-----------------------------------------------
// Mapping
//-----------------------------------------------

bool NodeQueryResult::range(const VNode* node,int &pos,int &cnt)
{
	 ServerHandler *server=node->server();
	 auto it=blocks_.find(server);
	 if(it != blocks_.end())
	 {
		 if(it->second.find(node,pos,cnt))
			 return true;
	 }
	 return false;
}

