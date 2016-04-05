//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VFilter.hpp"

#include "NodeQuery.hpp"
#include "NodeQueryEngine.hpp"
#include "UserMessage.hpp"
#include "VNState.hpp"
#include "VAttribute.hpp"
#include "VIcon.hpp"
#include "VNode.hpp"
#include "VParam.hpp"
#include "VSettings.hpp"

#include "ServerFilter.hpp"
#include "ServerHandler.hpp"

#include <QDebug>
#include <QTime>

#include <algorithm>

#define _UI_VFILTER_DEBUG

//==============================================
//
// VFilter
//
//==============================================

VParamSet::VParamSet()
{
}

void VParamSet::init(const std::vector<VParam*>& items)
{
	for(std::vector<VParam*>::const_iterator it=items.begin(); it != items.end(); ++it)
	{
		all_.insert((*it));
	}
}

bool VParamSet::isSet(VParam* p) const
{
	if(!p)
		return false;

	return (current_.find(p) != current_.end());
}

bool VParamSet::isSet(const std::string &name) const
{
	for(std::set<VParam*>::const_iterator it=current_.begin(); it != current_.end(); ++it)
	{
		if((*it)->strName() == name)
				return true;
	}
	return false;
}

QStringList VParamSet::currentAsList() const
{
	QStringList lst;
	for(std::set<VParam*>::const_iterator it=current_.begin(); it != current_.end(); ++it)
	{
		lst << QString::fromStdString((*it)->strName());
	}
	return lst;
}

void VParamSet::setCurrent(const std::set<VParam*>& items)
{
	current_.clear();
	for(std::set<VParam*>::const_iterator it=all_.begin(); it != all_.end(); ++it)
	{
		if(items.find(*it) != items.end())
			current_.insert(*it);
	}

	Q_EMIT changed();
}

void VParamSet::current(const std::set<std::string>& names)
{
	current_.clear();
	for(std::set<VParam*>::const_iterator it=all_.begin(); it != all_.end(); ++it)
	{
			if(names.find((*it)->strName()) != names.end())
				current_.insert(*it);
	}

	Q_EMIT changed();
}

void VParamSet::setCurrent(QStringList names)
{
	current_.clear();
	for(std::set<VParam*>::const_iterator it=all_.begin(); it != all_.end(); ++it)
	{
			if(names.contains(QString::fromStdString((*it)->strName())))
				current_.insert(*it);
	}

	Q_EMIT changed();
}


void VParamSet::writeSettings(VSettings *vs)
{
	std::vector<std::string> array;

	for(std::set<VParam*>::const_iterator it=current_.begin(); it != current_.end(); ++it)
	{
		array.push_back((*it)->strName());
	}

	vs->put(settingsId_,array);
}

void VParamSet::readSettings(VSettings* vs)
{
	current_.clear();

	std::vector<std::string> array;
	vs->get(settingsId_,array);

	for(std::vector<std::string>::const_iterator it = array.begin(); it != array.end(); ++it)
	{
		std::string name=*it;
		for(std::set<VParam*>::const_iterator itA=all_.begin(); itA != all_.end(); ++itA)
		{
			if((*itA)->strName() == name)
					current_.insert(*itA);
		}
	}
}

//==============================================
//
// StateFilter
//
//==============================================

NodeStateFilter::NodeStateFilter() : VParamSet()
{
	settingsId_="state";
	std::vector<VParam*> v=VNState::filterItems();
	init(v);
}


//==============================================
//
// AttributeFilter
//
//==============================================

AttributeFilter::AttributeFilter() : VParamSet()
{
	settingsId_="attribute";
	std::vector<VParam*> v=VAttribute::filterItems();
	init(v);

	/*for(std::set<VParam*>::const_iterator it=all_.begin(); it != all_.end(); ++it)
	{
		if((*it)->strName() != "var" && (*it)->strName() != "genvar")
			current_.insert(*it);
	}*/
}

//==============================================
//
// IconFilter
//
//==============================================

IconFilter::IconFilter() : VParamSet()
{
	settingsId_="icon";
	std::vector<VParam*> v=VIcon::filterItems();
	init(v);
	current_=all_;
}


NodeFilterDef::NodeFilterDef(ServerFilter* serverFilter,Scope scope) :
	serverFilter_(serverFilter),
	nodeState_(0)
{
	nodeState_=new NodeStateFilter;

	//if(scope == NodeStateScope)
	//	nodeState_=new NodeStateFilter;

	//else if(scope == GeneralScope)
	//	nodeState_=new NodeStateFilter;

	if(nodeState_)
	{
		exprStr_="state = all";

		connect(nodeState_,SIGNAL(changed()),
					this,SIGNAL(changed()));
	}

	query_=new NodeQuery("tmp",true);
	//QStringList sel("aborted");
	//query_->setStateSelection(sel);
}

NodeFilterDef::~NodeFilterDef()
{
	delete query_;
}

NodeQuery* NodeFilterDef::query() const
{
	return query_;
}

void NodeFilterDef::setQuery(NodeQuery* q)
{
	query_->swap(q);
	Q_EMIT changed();
}

void NodeFilterDef::writeSettings(VSettings *vs)
{
	vs->beginGroup("query");
	query_->save(vs);
	vs->endGroup();
}

void NodeFilterDef::readSettings(VSettings *vs)
{
	vs->beginGroup("query");
	query_->load(vs);
	vs->endGroup();

	Q_EMIT changed();
}

NodeFilter::NodeFilter(NodeFilterDef* def,ServerHandler* server) :
	def_(def),
    matchMode_(VectorMatch),
    server_(server)
{
    assert(server_);

    queryEngine_=new NodeFilterEngine(this);
}

NodeFilter::~NodeFilter()
{
	delete queryEngine_;
}

//============================================
//
// TreeNodeFilter
//
//============================================

TreeNodeFilter::TreeNodeFilter(NodeFilterDef* def,ServerHandler* server) : NodeFilter(def,server)
{
}

void TreeNodeFilter::clear()
{
    match_=std::vector<VNode*>();
}

bool TreeNodeFilter::isNull()
{
    return def_->nodeState_->isComplete() || def_->nodeState_->isEmpty();
}

//
bool TreeNodeFilter::update(const std::vector<VNode*>& topChange,std::vector<VNode*>& topFilterChange)
{
#ifdef _UI_VFILTER_DEBUG
    UserMessage::debug("TreeNodeFilter::update --> " + server_->name());
#endif

    //nodes_.clear();

    //If all states are visible
    if(def_->nodeState_->isComplete() || def_->nodeState_->isEmpty())
    {
        //deallocate the match vector
        match_=std::vector<VNode*>();
        //assert(match_.capacity() == 0);
#ifdef _UI_VFILTER_DEBUG
        UserMessage::debug("  no filter is defined!");
#endif
        return false;
    }

#ifdef _UI_VFILTER_DEBUG
    QTime timer;
    timer.start();
#endif

    VServer* root=server_->vRoot();
    if(root->totalNum() > 0)
    {
        bool fullRun=false;

        //The number of nodes changed: we need to rerun everything
        if(match_.size() != root->totalNum())
        {
            //Deallocates the match vector
            match_=std::vector<VNode*>();
            //match_.reserve(root->totalNum());
            VNode *n=0;
            match_.resize(root->totalNum(),n);
            //td::fill(match_.begin(), match_.end(), n);
            fullRun=true;
        }

        //The topchange vector is empty: it can only happen when we need to rerun everything
        else if(topChange.empty())
        {
            VNode *n=0;
            //match_.clear();
            std::fill(match_.begin(), match_.end(), n);
            fullRun=true;
        }

        //We rerun everything
        if(fullRun)
        {
            for(size_t i=0; i < root->numOfChildren(); i++)
            {
                filterState(root->childAt(i),def_->nodeState_);
            }
        }

        //We only check the branches defined by the nodes in topChange
        else
        {           
            //save the latest results
            std::vector<VNode*> prevNodes=match_;

            //Update the filter results
            for(size_t i=0; i < topChange.size(); i++)
            {
                filterState(topChange[i],def_->nodeState_);
            }

#ifdef _UI_VFILTER_DEBUG
            int diffCnt=0;
            for(size_t i=0; i < match_.size(); i++)
            {
                if(prevNodes[i] != match_[i])
                    diffCnt++;
            }
            UserMessage::debug("  number of differences in filter: " + QString::number(diffCnt).toStdString());
#endif

            //We collect the topmost nodes with changes. It could be different to
            //topChange so we need this step!
            for(size_t i=0; i < topChange.size(); i++)
            {
                checkState(topChange[i],prevNodes,topFilterChange);
            }

#ifdef _UI_VFILTER_DEBUG
            assert(topFilterChange.size() <= diffCnt);
            if(diffCnt > 0)
                assert(topFilterChange.size() >0);
#endif
        }

#ifdef _UI_VFILTER_DEBUG
        UserMessage::debug("  Top level nodes that changed in filter:");
        for(size_t i= 0; i < topFilterChange.size(); i++)
            UserMessage::debug("     " +  topFilterChange.at(i)->strName());
#endif

    }
    else
    {
       match_.clear();
    }

#ifdef _UI_VFILTER_DEBUG
    UserMessage::debug("  elapsed time: " + QString::number(timer.elapsed()).toStdString() + " ms");
    UserMessage::debug("  filter size:" + QString::number(match_.size()).toStdString());
    UserMessage::debug("  capacity:" + QString::number(match_.capacity()).toStdString());
#endif

    return true;
}

bool TreeNodeFilter::update()
{
    std::vector<VNode*> topChange;
    std::vector<VNode*> topFilterChange;
    return update(topChange,topFilterChange);
}

bool TreeNodeFilter::checkState(VNode* n,const std::vector<VNode*>& prevNodes,std::vector<VNode*>& topFilterChange)
{
    int idx=n->index();
    if(prevNodes[idx] != match_[idx])
    {
        VNode *pn=n->parent();
        if(!pn) pn=n;

        if(std::find(topFilterChange.begin(),topFilterChange.end(),pn) == topFilterChange.end())
            topFilterChange.push_back(pn);

        return true;
    }
    else
    {
         for(unsigned int i=0; i < n->numOfChildren(); i++)
         {
            checkState(n->childAt(i),prevNodes,topFilterChange);
         }
    }

    return false;
}

bool TreeNodeFilter::filterState(VNode* node,VParamSet* stateFilter)
{
    bool ok=false;
    //Suites always match!!
    if(node->isSuite() || stateFilter->isSet(VNState::toState(node)))
    {
        ok=true;
    }

    for(unsigned int i=0; i < node->numOfChildren(); i++)
    {
        if(filterState(node->childAt(i),stateFilter) == true && ok == false)
        {
            ok=true;
        }
    }

    if(ok)
    {
        match_[node->index()]=node;
    }
    else
    {
        match_[node->index()]=NULL;
    }

    return ok;
}

//===========================================================
//
// TableNodeFilter
//
//===========================================================

TableNodeFilter::TableNodeFilter(NodeFilterDef* def,ServerHandler* server) :
    NodeFilter(def,server),
    matchCount_(0)
{
}

bool TableNodeFilter::isNull()
{
	return def_->nodeState_->isComplete();
}

void TableNodeFilter::clear()
{
	match_.clear();
    index_.clear();
    matchCount_=0;
}

int TableNodeFilter::indexOf(const VNode* node) const
{
    switch(matchMode_)
    {
    case VectorMatch:
         return index_[node->index()];
    case AllMatch:
        return node->index();
    case NoneMatch:
        return -1;
    default:
        assert(0);
        return -1;
    }

    return -1;
}

VNode* TableNodeFilter::nodeAt(int index) const
{
    switch(matchMode_)
    {
    case VectorMatch:
         return match_[index];
    case AllMatch:
        return server_->vRoot()->nodeAt(index);
    case NoneMatch:
        return NULL;
    default:
        assert(0);
        return NULL;
    }

    return NULL;
}

bool TableNodeFilter::update()
{
#ifdef _UI_VFILTER_DEBUG
    UserMessage::debug("TableNodeFilter::update --> " + server_->name());
#endif

    NodeQuery* q=def_->query_;

    if(!q->hasServer(server_->name()) || server_->vRoot()->totalNum() ==0)
    {
        matchMode_=NoneMatch;
        //Deallocates
        match_=std::vector<VNode*>();
        index_=std::vector<int>();
        matchCount_=0;
#ifdef _UI_VFILTER_DEBUG
        UserMessage::debug("  no nodes are filtered!");
#endif
        return true;
    }

    if(q->query().isEmpty() && q->rootNode().empty())
    {
        matchMode_=AllMatch;
        //Deallocates
        match_=std::vector<VNode*>();
        index_=std::vector<int>();
        matchCount_=server_->vRoot()->totalNum();
#ifdef _UI_VFILTER_DEBUG
        UserMessage::debug("  all the nodes are filtered!");
#endif
        return true;
    }

#ifdef _UI_VFILTER_DEBUG
     QTime timer;
     timer.start();
#endif

     matchMode_=VectorMatch;
     match_.clear();
     int num=server_->vRoot()->totalNum();
     if(num != index_.size())
     {
        //Reallocates
        index_=std::vector<int>();
        index_.resize(num,-1);
     }
     else
     {
        std::fill(index_.begin(), index_.end(), -1);
     }

     queryEngine_->setQuery(def_->query_);
     queryEngine_->runQuery(server_);

     matchCount_=match_.size();
     for(size_t i=0; i < match_.size(); i++)
     {
        index_[match_[i]->index()]=i;
     }

#ifdef _UI_VFILTER_DEBUG
    UserMessage::debug("  elapsed time: " + QString::number(timer.elapsed()).toStdString() + " ms");
    UserMessage::debug("  filter size:" + QString::number(match_.size()).toStdString());
    UserMessage::debug("  capacity:" + QString::number(match_.capacity()).toStdString());
#endif

    return true;

}

