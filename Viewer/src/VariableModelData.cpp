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

void VariableModelData::match(const std::string& txt,std::vector<int>& result) const
{
    QString str=QString::fromStdString(txt);
    
    qDebug() << "match" << QString::fromStdString(txt);

    for(int i=0; i < varNum(); i++)
    {
    	qDebug() << "  " << QString::fromStdString(name(i)) << QString::fromStdString(value(i)) ;

    	if(QString::fromStdString(name(i)).contains(str,Qt::CaseInsensitive) ||
            QString::fromStdString(value(i)).contains(str,Qt::CaseInsensitive))
        {
            qDebug() << "    -->YES";
    		result.push_back(i);
        }
    }
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

VariableNodeData::VariableNodeData(Node *node)
{
	node_=node;
	reload();

}

const std::string& VariableNodeData::dataName()
{
	return node_->name();
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

void VariableNodeData::setValue(int index,const std::string& val)
{
	VInfo_ptr info(VInfo::make(node_));

	std::string cmd="ecflow_client --alter change variable " + name(index) + " " + val + " <full_name>"  ;

	ServerHandler::command(info,cmd,false);

	/*get_node()->serv().command(clientName, "--alter", add ? "add" : "change", "variable",
	                                       name, value, get_node()->full_name().c_str(), NULL);*/

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
			if(data_.at(i)->dataName() == node->name())
			{
				//Notifies the model that a change will happen
			    Q_EMIT reloadBegin();
			    data_.at(i)->reload();
			    Q_EMIT reloadEnd();
			}
		}
	}
}

