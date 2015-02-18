//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VariableModelData.hpp"

#include "ServerHandler.hpp"
#include "VNState.hpp"

static std::string defaultStr("");

//==========================================
//
// VariableModelData
//
//==========================================

const std::string& VariableModelData::name(int index) const
{
	if(index < 0 || index >= varNum())
		return defaultStr;

	if(!isGenVar(index))
	{
		return vars_.at(index).first;
	}
	else
	{
		return genVars_.at(index-vars_.size()).first;
	}
}

const std::string& VariableModelData::value(int index) const
{
	if(index < 0 || index >= varNum())
		return defaultStr;

	if(!isGenVar(index))
	{
		return vars_.at(index).second;
	}
	else
	{
		return genVars_.at(index-vars_.size()).second;
	}
}


bool VariableModelData::isGenVar(int index) const
{
	return (index >= vars_.size());
}

int VariableModelData::varNum() const
{
	return vars_.size() + genVars_.size();
}

VariableServerData::VariableServerData(ServerHandler *server)
{
	server_=server;
	reload();

}

const std::string& VariableServerData::name()
{
	return server_->longName();
}

QColor VariableServerData::colour()
{
	return Qt::gray;
}

void VariableServerData::reload()
{
	ServerDefsAccess defsAccess(server_);  // will reliquish its resources on destruction
	const std::vector<Variable>& v=defsAccess.defs()->server().server_variables();

	for(std::vector<Variable>::const_iterator it=v.begin(); it != v.end(); it++)
	{
		vars_.push_back(std::make_pair((*it).name(),(*it).theValue()));
	}
}

VariableNodeData::VariableNodeData(Node *node)
{
	node_=node;
	reload();

}

const std::string& VariableNodeData::name()
{
	return node_->name();
}

QColor VariableNodeData::colour()
{
	return VNState::toColour(node_);
}


void VariableNodeData::reload()
{
	std::vector<Variable> v=node_->variables();
	for(std::vector<Variable>::const_iterator it=v.begin(); it != v.end(); it++)
	{
		vars_.push_back(std::make_pair((*it).name(),(*it).theValue()));
	}

	v.clear();
	node_->gen_variables(v);
	for(std::vector<Variable>::const_iterator it=v.begin(); it != v.end(); it++)
	{
		genVars_.push_back(std::make_pair((*it).name(),(*it).theValue()));
	}
}


//==========================================
//
// VariableModelDataHandler
//
//==========================================

VariableModelDataHandler::VariableModelDataHandler()
{
}

VariableModelDataHandler::~VariableModelDataHandler()
{
	clear();
}
void VariableModelDataHandler::reload(VInfo_ptr info)
{
	//Notifies the model that a change will happen
	Q_EMIT reloadBegin();

	clear();

	server_=0;
	std::vector<Node*> nodes;

	if(info.get() != 0)
	{
		info->ancestors(&server_,nodes);

		for(std::vector<Node*>::iterator it=nodes.begin(); it != nodes.end(); it++)
		{
			data_.push_back(new VariableNodeData(*it));
		}

		if(server_)
		{
			data_.push_back(new VariableServerData(server_));
		}

	}

	Q_EMIT reloadEnd();

	//Reset the model (views will be n
}

void VariableModelDataHandler::clear()
{
	for(std::vector<VariableModelData*>::iterator it=data_.begin(); it != data_.end(); it++)
	{
		delete *it;
	}

	data_.clear();
}

int VariableModelDataHandler::varNum(int index) const
{
	if(index >=0 && index < data_.size())
		return data_.at(index)->varNum();

	return -1;
}

VariableModelData* VariableModelDataHandler::data(int index) const
{
	if(index >=0 && index < data_.size())
		return data_.at(index);

	return 0;
}


void VariableModelDataHandler::nodeChanged(const Node* node, const std::vector<ecf::Aspect::Type>&)
{
	//find in data
	//update
	//emit change signal
}

