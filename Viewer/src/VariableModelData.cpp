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
#include "UserMessage.hpp"
#include "VNode.hpp"
#include "VNState.hpp"

#include <QDebug>
#include <QString>

static std::string defaultStr("");

QStringList VariableModelData::readOnlyVars_;

#define _UI_VARIABLEMODELDATA_DEBBUG

//==========================================
//
// VariableModelData
//
//==========================================

VariableModelData::VariableModelData(VInfo_ptr info) :
		info_(info)
{
	reload();

	if(readOnlyVars_.isEmpty())
	{
		readOnlyVars_ << "ECF_NODE" << "ECF_PORT" << "ECF_PID" << "ECF_VERSION" <<
				"ECF_LISTS";
	}
}

VariableModelData::~VariableModelData()
{
}

void VariableModelData::clear()
{
	vars_.clear();
	genVars_.clear();
}

void VariableModelData::reload()
{
	clear();

	if(info_ && info_->node())
	{
		info_->node()->variables(vars_);
        info_->node()->genVariables(genVars_);
        removeDuplicates(vars_,genVars_);
	}
}

//When this function called duplicates must have already been removed!!
void VariableModelData::reset(const std::vector<Variable>& vars,const std::vector<Variable>& genVars)
{
    clear();

    if(info_ && info_->node())
    {
        vars_=vars;
        genVars_=genVars;
    }
}

void  VariableModelData::removeDuplicates(const std::vector<Variable>& vars,std::vector<Variable>& genVars)
{
    std::vector<Variable> gvOri=genVars;
    genVars.clear();

    for(std::vector<Variable>::const_iterator it=gvOri.begin(); it != gvOri.end(); ++it)
    {
         bool hasIt=false;
         for(std::vector<Variable>::const_iterator itV=vars.begin(); itV != vars.end(); ++itV)
         {
             if((*it).name() == (*itV).name())
             {
                hasIt=true;
                break;
             }
         }
         if(!hasIt)
             genVars.push_back(*it);
    }
}



std::string VariableModelData::fullPath()
{
	if(info_ && info_->node())
        return info_->path();

	return std::string();
}

std::string VariableModelData::name()
{
	return info_->name();
}

std::string VariableModelData::type()
{
	if(info_)
	{
		if(info_->isServer())
			return "server";
		else if(info_->node())
			return info_->node()->nodeType();
	}

	return std::string();
}

VNode* VariableModelData::node() const
{
	if(info_ && info_->isNode())
		return info_->node();

	return NULL;
}

const std::string& VariableModelData::name(int index) const
{
	if(index < 0 || index >= varNum())
		return defaultStr;

	if(!isGenVar(index))
	{
		return vars_.at(index).name();
	}
	else
	{
		return genVars_.at(index-vars_.size()).name();
	}

	return defaultStr;
}

const std::string& VariableModelData::value(int index) const
{
	if(index < 0 || index >= varNum())
		return defaultStr;

	if(!isGenVar(index))
	{
		return vars_.at(index).theValue();
	}
	else
	{
		return genVars_.at(index-vars_.size()).theValue();
	}

	return defaultStr;
}

bool VariableModelData::hasName(const std::string& n) const
{
	for(std::vector<Variable>::const_iterator it=vars_.begin(); it != vars_.end(); ++it)
	{
		if((*it).name() == n)
		{
			return true;
		}
	}

	for(std::vector<Variable>::const_iterator it=genVars_.begin(); it != genVars_.end(); ++it)
	{
		if((*it).name() == n)
		{
			return true;
		}
	}

	return false;

}

void VariableModelData::buildAlterCommand(std::vector<std::string>& cmd,
		                            const std::string& action, const std::string& type,
		                            const std::string& name,const std::string& value)
{
	cmd.push_back("ecflow_client");
	cmd.push_back("--alter");
	cmd.push_back(action);
	cmd.push_back(type);

	if(!name.empty())
	{
		cmd.push_back(name);
		cmd.push_back(value);
	}

	cmd.push_back("<full_name>");

}

void VariableModelData::setValue(int index,const std::string& val)
{
	std::vector<std::string> cmd;
	buildAlterCommand(cmd,"change","variable",name(index),val);

	ServerHandler::command(info_,cmd);
}

void VariableModelData::alter(const std::string& name,const std::string& val)
{
    std::string mode="add";

    //Existing name
    if(hasName(name))
    {
        //Generated variables cannot be changed. We instead add user variable with the same
        //name. It will shadow the original gen variable.
        if(isGenVar(name))
        {
            mode="add";
        }
        else
        {
            mode="change";
        }
    }

    std::vector<std::string> cmd;
    buildAlterCommand(cmd,mode,"variable",name,val);
    ServerHandler::command(info_,cmd);
}


void VariableModelData::add(const std::string& name,const std::string& val)
{
	std::vector<std::string> cmd;
    buildAlterCommand(cmd,(hasName(name))?"change":"add","variable",name,val);

	ServerHandler::command(info_,cmd);
}

void VariableModelData::remove(int index,const std::string& varName)
{
	if(varName == name(index))
	{
		std::vector<std::string> cmd;
		buildAlterCommand(cmd,"delete","variable",varName,"");

		ServerHandler::command(info_,cmd);
	}
}

bool VariableModelData::isGenVar(int index) const
{
	return (index >= vars_.size());
}

bool VariableModelData::isGenVar(const std::string& n) const
{
    for(std::vector<Variable>::const_iterator it=genVars_.begin(); it != genVars_.end(); ++it)
    {
        if((*it).name() == n)
        {
            return true;
        }
    }
    return false;
}

bool VariableModelData::isReadOnly(int index) const
{
	return isReadOnly(name(index));
}

bool VariableModelData::isReadOnly(const std::string& varName) const
{
	return readOnlyVars_.contains(QString::fromStdString(varName));
}


int VariableModelData::varNum() const
{
	return vars_.size() + genVars_.size();
}

void VariableModelData::latestVars(std::vector<Variable>& v,std::vector<Variable>& vg)
{
    if(info_ && info_->node())
    {
        info_->node()->variables(v);
        info_->node()->genVariables(vg);
        removeDuplicates(v,vg);
    }
}

//Check if the total number of variables will change. It does not update the local data!
int VariableModelData::checkUpdateDiff(std::vector<Variable>& v,std::vector<Variable>& vg)
{
    if(info_ && info_->node() && v.empty() && vg.empty())
	{
        latestVars(v,vg);
    }

	//Return the change in the total size of variables
	return v.size()+vg.size() -(vars_.size() + genVars_.size());
}


//Check if any of the names or values has changed. We suppose that the number of current and new
//variables are the same but some of their names or values have been changed.
bool VariableModelData::update(const std::vector<Variable>& v,const std::vector<Variable>& vg)
{
#ifdef _UI_VARIABLEMODELDATA_DEBBUG
    UserMessage::debug("VariableModelData::update -->");
#endif

#if 0
    if(info_ && info_->node() && v.empty() && vg.empty())
	{
        latestVars(v,vg);

#ifdef _UI_VARIABLEMODELDATA_DEBBUG
    UserMessage::debug("   call latestVars");
#endif

	}
#endif

#ifdef _UI_VARIABLEMODELDATA_DEBBUG
    UserMessage::debug("   new list of variables:");
    for(size_t i=0; i < v.size(); i++)
        UserMessage::debug("     " + v[i].name() + "=" + v[i].theValue());
    UserMessage::debug("   new list of generated variables:");
    for(size_t i=0; i < vg.size(); i++)
        UserMessage::debug("     " + vg[i].name() + "=" + vg[i].theValue());
#endif

    //We must have the same number of variables
    assert(v.size() + vg.size() == vars_.size() + genVars_.size());

    bool changed=false;
    if(v.size() != vars_.size() || vg.size() != genVars_.size())
    {
        changed=true;
#ifdef _UI_VARIABLEMODELDATA_DEBBUG
        UserMessage::qdebug("   variables size changed! var: " + QString::number(vars_.size()) +
                            " -> " +  QString::number(v.size()) + "gen var: " +
                            QString::number(genVars_.size()) + " -> " +  QString::number(vg.size()));
#endif
    }
    else
    {
        for(unsigned int i=0; i < vars_.size(); i++)
        {
            if(vars_[i].name() != v[i].name() || vars_[i].theValue() != v[i].theValue())
            {                
#ifdef _UI_VARIABLEMODELDATA_DEBBUG
                UserMessage::debug("   variable changed! name: " + vars_[i].name() + " -> " +
                           v[i].name()  + " value: " +   vars_[i].theValue() + " -> " +   v[i].theValue());
#endif
                changed=true;
                break;
            }
        }

        if(changed == false)
        {
            for(unsigned int i=0; i < genVars_.size(); i++)
            {
                if(genVars_[i].name() != vg[i].name() || genVars_[i].theValue() != vg[i].theValue())
                {
#ifdef _UI_VARIABLEMODELDATA_DEBBUG
                    UserMessage::debug("   generated variable changed! name: " + genVars_[i].name() + " -> " +
                           vg[i].name()  + " value: " +   genVars_[i].theValue() + " -> " +   vg[i].theValue());
#endif

                    changed=true;
                    break;
                }
            }
        }
    }

	if(changed)
	{
		vars_=v;
		genVars_=vg;        
#ifdef _UI_VARIABLEMODELDATA_DEBBUG
        UserMessage::debug("   updated vars and genvars");
#endif
	}

#ifdef _UI_VARIABLEMODELDATA_DEBBUG
    UserMessage::debug("<-- VariableModelData::update");
#endif

	return changed;

}

//==========================================
//
// VariableModelDataHandler
//
//==========================================

VariableModelDataHandler::VariableModelDataHandler() : server_(0)
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

	if(info.get() != 0 && info->node())
	{
		server_=info->server();

		std::vector<VNode*> nodes=info->node()->ancestors(VNode::ChildToParentSort);

		for(std::vector<VNode*>::iterator it=nodes.begin(); it != nodes.end(); ++it)
		{
			VNode* n=*it;

			if(n->isServer())
			{
				VInfo_ptr ptr=VInfoServer::create(n->server());
				data_.push_back(new VariableModelData(ptr));
			}
			else
			{
				VInfo_ptr ptr=VInfoNode::create(n);
				data_.push_back(new VariableModelData(ptr));
			}
		}
	}

	Q_EMIT reloadEnd();

	//Reset the model (views will be n
}

void VariableModelDataHandler::reload()
{
	//Notifies the model that a change will happen
	Q_EMIT reloadBegin();

	for(std::vector<VariableModelData*>::iterator it=data_.begin(); it != data_.end(); ++it)
	{
		(*it)->reload();
	}

	Q_EMIT reloadEnd();
}


void VariableModelDataHandler::clear()
{
	//Notifies the model that a change will happen
	Q_EMIT reloadBegin();

	for(std::vector<VariableModelData*>::iterator it=data_.begin(); it != data_.end(); ++it)
	{
		delete *it;
	}

	data_.clear();

	Q_EMIT reloadEnd();
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

//It is called when a node changed.
bool VariableModelDataHandler::nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect)
{
#ifdef _UI_VARIABLEMODELDATA_DEBBUG
    UserMessage::debug("VariableModelDataHandler::nodeChanged -->");
#endif
    int dataIndex=-1;
	for(unsigned int i=0; i < data_.size(); i++)
	{
		if(data_.at(i)->node() == node)
		{
			dataIndex=i;
			break;
		}
	}

#ifdef _UI_VARIABLEMODELDATA_DEBBUG
    UserMessage::qdebug("   dataIndex=" + QString::number(dataIndex));
#endif
    Q_ASSERT(dataIndex != -1);

    bool retVal=updateVariables(dataIndex);

#ifdef _UI_VARIABLEMODELDATA_DEBBUG
            UserMessage::debug("<-- VariableModelDataHandler::nodeChanged");
#endif

    return retVal;
}

//It is called when the server defs was changed
bool VariableModelDataHandler::defsChanged(const std::vector<ecf::Aspect::Type>& aspect)
{
#ifdef _UI_VARIABLEMODELDATA_DEBBUG
    UserMessage::debug("VariableModelDataHandler::defsChanged -->");
#endif

    if(data_.size() == 0)
    {
#ifdef _UI_VARIABLEMODELDATA_DEBBUG
        UserMessage::debug("<-- VariableModelDataHandler::defsChanged");
#endif
        return false;
    }

    int dataIndex=data_.size()-1;
    Q_ASSERT(dataIndex >=0 && dataIndex < data_.size());
    VariableModelData* d=data_.at(data_.size()-1);
    Q_ASSERT(d);
    Q_ASSERT(d->type() == "server");

    bool retVal=updateVariables(dataIndex);

#ifdef _UI_VARIABLEMODELDATA_DEBBUG
    UserMessage::debug("<-- VariableModelDataHandler::defsChanged");
#endif

    return retVal;
}

//It is called when the server defs was changed
bool VariableModelDataHandler::updateVariables(int dataIndex)
{
#ifdef _UI_VARIABLEMODELDATA_DEBBUG
    UserMessage::debug("VariableModelDataHandler::updateVariables -->");
#endif

    bool retVal=false;

    //There is no notification about generated variables. Basically they can change at any update!!
    //So we have to check all the variables at every update!!
    std::vector<Variable> v;
    std::vector<Variable> vg;

    //Get the current set of variables and check if the total number of variables
    //has changed. At this point v and vg do not contain any duplicates.
    int cntDiff=data_.at(dataIndex)->checkUpdateDiff(v,vg);

    //If the number of the variables is not the same as we store we reset the given block in the model
    if(cntDiff != 0)
    {
#ifdef _UI_VARIABLEMODELDATA_DEBBUG
        UserMessage::qdebug("    cntDiff=" + QString::number(dataIndex));
#endif
        const int numNew=v.size()+vg.size();

        //Notifies the model that rows will be added or removed for this data item
        Q_EMIT addRemoveBegin(dataIndex,cntDiff);

        //Reset the variables using v and vg.
        data_.at(dataIndex)->reset(v,vg);
        Q_ASSERT(data_.at(dataIndex)->varNum() == numNew);

        //Notifies the model that the change happened
        Q_EMIT addRemoveEnd(cntDiff);

        //Update the data item in the model
        Q_EMIT dataChanged(dataIndex);

        retVal=true;
    }
    //Check if some variables' name or value changed
    else
    {
#ifdef _UI_VARIABLEMODELDATA_DEBBUG
        UserMessage::debug("   Change: NODE_VARIABLE");
#endif
        //At this point we must have the same number of vars
        const int numNew=v.size()+vg.size();
        Q_ASSERT(data_.at(dataIndex)->varNum() == numNew);

        //Update the names/values
        if(data_.at(dataIndex)->update(v,vg))
        {
#ifdef _UI_VARIABLEMODELDATA_DEBBUG
            UserMessage::debug("   Variable name or value changed");
#endif
            //Update the data item in the model
            Q_EMIT dataChanged(dataIndex);
        }
        retVal=true;
    }

#ifdef _UI_VARIABLEMODELDATA_DEBBUG
        UserMessage::debug("<-- VariableModelDataHandler::updateVariables");
#endif

    return retVal;
}












