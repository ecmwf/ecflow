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

#include <QDebug>
#include <QString>

static std::string defaultStr("");

//==========================================
//
// VariableModelData
//
//==========================================

void VariableModelData::clear()
{
	vars_.clear();
	genVars_.clear();
}

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

bool VariableModelData::hasName(const std::string& n) const
{
	for(std::vector<std::pair<std::string,std::string> >::const_iterator it=vars_.begin(); it != vars_.end(); it++)
	{
		if((*it).first == n)
		{
			return true;
		}
	}

	for(std::vector<std::pair<std::string,std::string> >::const_iterator it=genVars_.begin(); it != genVars_.end(); it++)
	{
		if((*it).first == n)
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


/*    void variables::deleteCB( Widget, XtPointer )
{
   if (get_node()) {
      char *name = XmTextGetString(name_);
      const char* fullname = get_node()->full_name().c_str();
      if (confirm::ask(False, "Delete variable %s for node %s", name, fullname)) {
         // repeat get_node while suite may have been cancelled by another
         // while answering this question
         if (get_node()) {
            if (get_node()->__node__())
               get_node()->serv().command(clientName, "--alter", "delete", "variable", name,
                                          fullname, NULL);
            else
               get_node()->serv().command("alter", "-vr", fullname, name, NULL);
         }
      }
      XtFree(name);
      update();
   }
   else
      clear();
}

void variables::setCB( Widget, XtPointer )
{
   if (get_node()) {

      char *name = XmTextGetString(name_);
      char *value = XmTextGetString(value_);
      Boolean ok = True;
      node* n = get_node()->variableOwner(name);

      if (n != 0 && n != get_node()) {
         ok = confirm::ask(True, "This variable is already defined in the %s %s\n"
                           "A new variable will be created for the selected node\n"
                           "and hide the previous one\n"
                           "Do you want to proceed?",
                           n->type_name(), n->full_name().c_str());
      }

      if (n != 0 && n->isGenVariable(name) && ok) {
         ok = confirm::ask(True, "This variable is a generated variable\n"
                           "Do you want to proceed?");
      }

      if (ok) {
         bool add = true;
         if (get_node()->__node__()) add = get_node()->__node__()->variable(name)
                  == ecf_node::none();
         if (get_node()->__node__())
            get_node()->serv().command(clientName, "--alter", add ? "add" : "change", "variable",
                                       name, value, get_node()->full_name().c_str(), NULL);
         else
            get_node()->serv().command("alter", "-v", get_node()->full_name().c_str(), name, value,
                                       NULL);
         if (add) update();
      }
      XtFree(name);
      XtFree(value);
   }
   else
      clear();
}
    
}    
*/
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

const std::string& VariableServerData::dataName()
{
	return server_->longName();
}

std::string VariableServerData::type()
{
	return "server";
}

QColor VariableServerData::colour()
{
	return Qt::gray;
}

void VariableServerData::reload()
{
	clear();

	ServerDefsAccess defsAccess(server_);  // will reliquish its resources on destruction
	const std::vector<Variable>& v=defsAccess.defs()->server().server_variables();

	for(std::vector<Variable>::const_iterator it=v.begin(); it != v.end(); it++)
	{
		vars_.push_back(std::make_pair((*it).name(),(*it).theValue()));
	}
}


void VariableServerData::setValue(int index,const std::string& val)
{
}

void VariableServerData::add(const std::string& name,const std::string& val)
{

}

//==========================================
//
// VariableNodeData
//
//==========================================

VariableNodeData::VariableNodeData(Node *node)
{
	node_=node;
	reload();

}

const std::string& VariableNodeData::dataName()
{
	return node_->name();
}

std::string VariableNodeData::type()
{
	return "node";
}

QColor VariableNodeData::colour()
{
	return VNState::toColour(node_);
}


void VariableNodeData::reload()
{
	clear();

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

//Check if the total number of variables has changed.
bool VariableNodeData::sizeChanged()
{
	std::vector<Variable> v=node_->variables();
	std::vector<Variable> vg;
	node_->gen_variables(vg);

	return (v.size() != vars_.size() || vg.size() != genVars_.size());
}

void VariableNodeData::setValue(int index,const std::string& val)
{
	VInfo_ptr info(VInfo::make(node_));

	std::vector<std::string> cmd;
	buildAlterCommand(cmd,"change","variable",name(index),val);

	ServerHandler::command(info,cmd,false);
}

void VariableNodeData::add(const std::string& name,const std::string& val)
{
	VInfo_ptr info(VInfo::make(node_));

	std::vector<std::string> cmd;
	buildAlterCommand(cmd,"change","variable",name,val);


	ServerHandler::command(info,cmd,false);
}

void VariableNodeData::remove(int index,const std::string& varName)
{
	if(varName == name(index))
	{
		VInfo_ptr info(VInfo::make(node_));

		std::vector<std::string> cmd;
		buildAlterCommand(cmd,"delete","variable",varName,"");

		ServerHandler::command(info,cmd,false);
	}
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


void VariableModelDataHandler::nodeChanged(const Node* node, const std::vector<ecf::Aspect::Type>& aspect)
{
	bool changed=false;
	for(std::vector<ecf::Aspect::Type>::const_iterator it=aspect.begin(); it != aspect.end(); it++)
	{
		if(*it == ecf::Aspect::NODE_VARIABLE)
		{
			changed=true;
			break;
		}
	}

	if(changed)
	{
		for(unsigned int i=0; i < data_.size(); i++)
		{
			if(data_.at(i)->isNode(node))
			{
				if(data_.at(i)->sizeChanged())
				{
					//Notifies the model that a change will happen
					Q_EMIT reloadBegin();
					data_.at(i)->reload();
					Q_EMIT reloadEnd();
				}
				else
				{
					data_.at(i)->reload();
					Q_EMIT dataChanged(i);
				}
			}
		}
	}
}

