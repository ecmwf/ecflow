//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "EditProvider.hpp"

#include "NodeFwd.hpp"

#include "LogServer.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "ServerHandler.hpp"
#include "UserMessage.hpp"

//#include <boost/filesystem/operations.hpp>
//#include <boost/filesystem/path.hpp>
//#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>

/*void  InfoProvider::taskChanged(VTask_ptr task)
{
    if(task_ != task)
        return;

    switch(task->status())
    {
        case VTask::FINISHED:
            //We prepend the results to the existing text
            reply_->text(task->reply()->text());
            owner_->infoReady(reply_);
            //We do not need the task anymore.
            task_.reset();
            break;
        case VTask::ABORTED:
        case VTask::CANCELLED:
        case VTask::REJECTED:
            reply_->errorText(task->reply()->errorText());
            owner_->infoFailed(reply_);
            //We do not need the task anymore.
            task_.reset();break;
        default:
            break;
    }
}*/




/*
tmp_file ehost::edit( node& n, std::list<Variable>& l, Boolean preproc )
{
   gui::message("%s: fetching source", name());
   try {
      if (preproc)
         client_.edit_script_preprocess(n.full_name());
      else
         client_.edit_script_edit(n.full_name());
      return tmp_file(client_.server_reply().get_string());
   } catch ( std::exception &e ) {
       gui::error("host::edit-error: %s", e.what());
   } catch ( ... ) {
       gui::error("host::edit-error");
   }
  std::string error = "no script!\n"
"\n"
"check server->History:\n"
"\tsome suite variable may be 'unterminated' (micro character missing) in script or include files\n"
"\tcheck duplicate occurences of micro character when it is expected in the job (%% becomes %)\n"
"\tuse %nopp ... %end or %includenopp <file.h> to disable job preprocessing where needed\n"
"\tan include file may not be found\n"
"check ECF_FILE directory is accessible, by opening the Script panel\n"
"check ECF_INCLUDE directory is accessible from the server\n"
"\tit must contain the included files (or links)\n"
"client must be capable to create temporary file:\n"
"\tcheck /tmp directory with write access, and space available,\n"
"or preprocessed file may be truncated beyond some size.\n";
   return tmp_file(error);
}*/



//Node
void EditProvider::visit(VInfoNode* info)
{
	//Reset the reply
	reply_->reset();

	if(!info)
 	{
       	owner_->infoFailed(reply_);
   	}

	ServerHandler* server=info_->server();
	VNode *n=info->node();

    if(!n || !n->node())
   	{
       	owner_->infoFailed(reply_);
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

    //static std::string defMicro="%";
    static std::string comStartText = "comment - ecf user variables";
    static std::string comEndText   = "end - ecf user variables";

    NameValueVec vars;

    //Find out the micro
  	std::string microVar=node->findInheritedVariable("ECF_MICRO");
    std::string micro = (microVar.size() == 1) ? microVar.c_str() : micro;

	//Find out the full comment start and end texts
	std::string comStart= micro + comStartText;
	std::string comEnd= micro + comEndText;

    bool inVars = false;
    for(std::vector<std::string>::const_iterator it=txt.begin(); it != txt.end(); ++it)
    {
    	const std::string& line=*it;

    	//We are in the variable block
    	if(inVars)
    	{
    		std::size_t pos = (*it).find("=");
    		if(pos != std::string::npos && pos+1 != (*it).size())
    		{
    			std::string name=(*it).substr(0,pos);
    			std::string val=(*it).substr(pos+1);
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
    	UserMessage::message(UserMessage::DBG, false, std::string(" No user variables!"));
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
