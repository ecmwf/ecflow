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

NodeFilter::NodeFilter(NodeFilterDef* def,ResultMode resultMode) :
	def_(def),
	resultMode_(resultMode),
	beingReset_(false),
	res_(0),
	matchMode_(VectorMatch)
{
	queryEngine_=new NodeFilterEngine(this);
}

NodeFilter::~NodeFilter()
{
	delete queryEngine_;
}

TreeNodeFilter::TreeNodeFilter(NodeFilterDef* def) : NodeFilter(def,StoreNonMatched)
{

}

void TreeNodeFilter::clear()
{
    nodes_.clear();
    nodes_.resize(0);
}

bool TreeNodeFilter::isNull()
{
    return def_->nodeState_->isComplete() || def_->nodeState_->isEmpty();
}

bool TreeNodeFilter::isFiltered(VNode* node)
{
    if(nodes_.empty())
        return true;

    return nodes_[node->index()];

#if 0

    if(resultMode_==StoreNonMatched)
	{
		if(result_.empty())
			return true;

		return (std::find(result_.begin(), result_.end(), node) == result_.end());
	}
	else if(resultMode_==StoreMatched)
	{
		if(result_.empty())
			return false;

		return (std::find(result_.begin(), result_.end(), node) != result_.end());
	}
#endif

	return false;
}

#if 0
bool TreeNodeFilter::update(const VNode *node)
{
	beginReset(node->server());
	endReset();
	return true;
}
#endif

//
bool TreeNodeFilter::update(ServerHandler* server,const std::vector<VNode*>& topChange,std::vector<VNode*>& topFilterChange)
{
#ifdef _UI_VFILTER_DEBUG
    UserMessage::debug("TreeNodeFilter::update --> " + server->name());
#endif

    //nodes_.clear();

    //If all states are visible
    if(def_->nodeState_->isComplete() || def_->nodeState_->isEmpty())
    {
        nodes_.reserve(0);
        assert(nodes_.capacity() == 0);
#ifdef _UI_VFILTER_DEBUG
        UserMessage::debug("  no filter is defined!");
#endif
        return false;
    }

#ifdef _UI_VFILTER_DEBUG
    QTime timer;
    timer.start();
#endif

    VServer* root=server->vRoot();
    if(root->totalNum() > 0)
    {
        bool fullRun=false;
        //The number of nodes changed: we need to rerun everything
        if(nodes_.size() != root->totalNum())
        {
            nodes_.clear();
            nodes_.resize(root->totalNum());
            VNode *n=0;
            std::fill(nodes_.begin(), nodes_.end(), n);
            fullRun=true;
        }
        //The topchange vector is empty: it can only happen when we need to rerun everything
        else if(topChange.empty())
        {
            VNode *n=0;
            nodes_.clear();
            std::fill(nodes_.begin(), nodes_.end(), n);
            fullRun=true;
        }

        //We rerun everything
        if(fullRun)
        {
            for(size_t i=0; i < server->vRoot()->numOfChildren(); i++)
            {
                filterState(root->childAt(i),def_->nodeState_);
            }
        }

        //We only check the branches defined by the nodes in topChange
        else
        {           
            //save the latest results
            std::vector<VNode*> prevNodes=nodes_;

            //Update the filter results
            for(size_t i=0; i < topChange.size(); i++)
            {
                filterState(topChange[i],def_->nodeState_);
            }

            int diffCnt=0;
            for(size_t i=0; i < nodes_.size(); i++)
            {
                if(prevNodes[i] != nodes_[i])
                    diffCnt++;
            }
#ifdef _UI_VFILTER_DEBUG
            UserMessage::debug("  number of differences in filter: " + QString::number(diffCnt).toStdString());
#endif

            //We collect the topmost nodes with changes. It could be different to
            //topChange so we need this step!
            for(size_t i=0; i < topChange.size(); i++)
            {
                checkState(topChange[i],prevNodes,topFilterChange);
            }
        }
    }
    else
    {
       nodes_.clear();
    }

#ifdef _UI_VFILTER_DEBUG
    UserMessage::debug("  elapsed time: " + QString::number(timer.elapsed()).toStdString() + " ms");
    UserMessage::debug("  filter size:" + QString::number(nodes_.size()).toStdString());
#endif

    return true;
}

bool TreeNodeFilter::update(ServerHandler* server)
{
    std::vector<VNode*> topChange;
    std::vector<VNode*> topFilterChange;
    return update(server,topChange,topFilterChange);
}

bool TreeNodeFilter::checkState(VNode* n,const std::vector<VNode*>& prevNodes,std::vector<VNode*>& topFilterChange)
{
    int idx=n->index();
    if(prevNodes[idx] != nodes_[idx])
    {
        topFilterChange.push_back(n->parent());
        return true;
    }
    else
    {
         for(unsigned int i=0; i < n->numOfChildren(); i++)
         {
             return checkState(n->childAt(i),prevNodes,topFilterChange);
         }
    }

    return false;

}

void TreeNodeFilter::beginReset(ServerHandler* server)
{
#ifdef _UI_VFILTER_DEBUG
    UserMessage::debug("TreeNodeFilter::beginReset --> " + server->name());
#endif

    beingReset_=true;

#if 0
	result_.clear();
#endif

	resultMode_=StoreNonMatched;



    nodes_.clear();

	//If all states are visible
	if(def_->nodeState_->isComplete() || def_->nodeState_->isEmpty())
	{
        nodes_.reserve(0);
        assert(nodes_.capacity() == 0);
#ifdef _UI_VFILTER_DEBUG
        UserMessage::debug("  no filter is defined!");
#endif

        return;
	}

	else //if(def_->nodeState_->isEmpty())
	{
		resultMode_=StoreMatched;
	}


#ifdef _UI_VFILTER_DEBUG
    QTime timer;
    timer.start();
#endif

	VServer* root=server->vRoot();
    if(root->totalNum() > 0)
    {
        //nodes_.reserve(root->totalNum());
        nodes_.resize(root->totalNum());
        VNode *n=0;
        std::fill(nodes_.begin(), nodes_.end(), n);

        for(unsigned int j=0; j < root->numOfChildren();j++)
        {
            filterState(root->childAt(j),def_->nodeState_);
        }
    }

#ifdef _UI_VFILTER_DEBUG
    UserMessage::debug("  elapsed time: " + QString::number(timer.elapsed()).toStdString() + " ms");
    UserMessage::debug("  filter size:" + QString::number(nodes_.size()).toStdString());
#endif

}

void TreeNodeFilter::endReset()
{
	beingReset_=false;
}


bool TreeNodeFilter::filterState(VNode* node,VParamSet* stateFilter)
{
    bool ok=false;
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
        nodes_[node->index()]=node;
#if 0
        result_.insert(node);
#endif
    }

    return ok;
}

int TreeNodeFilter::matchCount()
{
	if(beingReset_)
		return 0;

	if(resultMode_==StoreMatched)
	{
		return static_cast<int>(result_.size());
	}
	return 0;
};

//===========================================================
// TableNodeFilter
//===========================================================

TableNodeFilter::TableNodeFilter(NodeFilterDef* def) : NodeFilter(def,StoreMatched)
{
}

bool TableNodeFilter::isNull()
{
	return def_->nodeState_->isComplete();
}

bool TableNodeFilter::isFiltered(VNode* node)
{
	if(beingReset_ || matchMode_ == NoneMatch)
		return false;

	else if(matchMode_ == AllMatch)
		return true;

	return res_[node->index()];
}

void TableNodeFilter::clear()
{
	match_.clear();
}

#if 0
bool TableNodeFilter::update(const VNode *node)
{
	beginReset(node->server());
	endReset();
	return true;
}
#endif

void TableNodeFilter::beginReset(ServerHandler* server)
{
	beingReset_=true;

	matchMode_=NoneMatch;

	NodeQuery* q=def_->query_;

	if(!q->hasServer(server->name()))
	{
		matchMode_=NoneMatch;
		//Deallocates
		res_=std::vector<bool>();
	}
	else
	{
		if(q->query().isEmpty() && q->rootNode().empty())
		{
			matchMode_=AllMatch;
			//Deallocates
			res_=std::vector<bool>();
		}
		else
		{
			matchMode_=VectorMatch;
			int num=server->vRoot()->totalNum();
			if(num != res_.size())
			{
				//Reallocates
				res_=std::vector<bool>();
				res_.reserve(num);
				for(size_t i=0; i < num; i++)
				{
					res_.push_back(false);
				}
			}
			else
			{
				std::fill(res_.begin(),res_.end(),false);
			}

			queryEngine_->setQuery(def_->query_);
			queryEngine_->runQuery(server);
		}
	}
}

void TableNodeFilter::endReset()
{
	beingReset_=false;
}


int TableNodeFilter:: matchCount()
{
	return 0;
}


