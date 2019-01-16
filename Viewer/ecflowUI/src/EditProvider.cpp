//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "EditProvider.hpp"

#include "NodeFwd.hpp"

#include "VNode.hpp"
#include "VReply.hpp"
#include "ServerHandler.hpp"
#include "UiLog.hpp"

#include <boost/algorithm/string.hpp>

//Node
void EditProvider::visit(VInfoNode* info)
{
	//Reset the reply
	reply_->reset();

	if(!info)
 	{
       	owner_->infoFailed(reply_);
        return;
   	}

	ServerHandler* server=info_->server();
	VNode *n=info->node();

    if(!n || !n->node())
   	{
       	owner_->infoFailed(reply_);
        return;
   	}

    if (preproc_)
    {
    	//Define a task for getting the info from the server.
    	task_=VTask::create(VTask::ScriptPreprocTask,n,this);
    }
    else
    {
    	task_=VTask::create(VTask::ScriptEditTask,n,this);
    }

	//Run the task in the server. When it finishes taskFinished() is called. The text returned
	//in the reply will be prepended to the string we generated above.
	server->run(task_);
}

void EditProvider::submit(const std::vector<std::string>& txt,bool alias)
{
	if(!(info_ && info_.get() && info_->isNode() && info_->node()) && info_->server())
		return;

	VNode *node=info_->node();
    ServerHandler* server=info_->server();

	bool run=true;

    //---------------------------
    // Extract user variables
    //---------------------------

    static std::string defMicro="%";
    static std::string comStartText = "comment - ecf user variables";
    static std::string comEndText   = "end - ecf user variables";

    NameValueVec vars;

    //Find out the micro
  	std::string microVar=node->findInheritedVariable("ECF_MICRO");
    std::string micro = (microVar.size() == 1) ? microVar.c_str() : defMicro;

	//Find out the full comment start and end texts
	std::string comStart= micro + comStartText;
	std::string comEnd= micro + comEndText;

    bool inVars = false;
    for(const auto & it : txt)
    {
    	const std::string& line=it;

    	//We are in the variable block
    	if(inVars)
    	{
    		std::size_t pos = it.find("=");
    		if(pos != std::string::npos && pos+1 != it.size())
    		{
    			std::string name=it.substr(0,pos);
    			std::string val=it.substr(pos+1);
    			boost::trim(name);
    			boost::trim(val);
    			vars.push_back(std::make_pair(name,val));
    		}
    	}

    	if(line == comStart)
    		inVars = true;

    	if(line == comEnd)
    	    break;
    }

    if(vars.empty())
    {
        UiLog().dbg() << " No user variables!";
    }


	task_=VTask::create(VTask::ScriptSubmitTask,node,this);
	task_->contents(txt);
	task_->vars(vars);
	task_->param("alias",(alias)?"1":"0");
	task_->param("run",(run)?"1":"0");

	//Run the task in the server. When it finishes taskFinished() is called. The text returned
	//in the reply will be prepended to the string we generated above.
	server->run(task_);
}
