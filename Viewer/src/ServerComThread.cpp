//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ServerComThread.hpp"

#include "Defs.hpp"
#include "ClientInvoker.hpp"
#include "ArgvCreator.hpp"

#include "ServerDefsAccess.hpp"
#include "ServerComQueue.hpp"
#include "ServerHandler.hpp"
#include "SuiteFilter.hpp"
#include "UserMessage.hpp"

#include <algorithm>
#include <sstream>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#include <QDebug>

ServerComThread::ServerComThread(ServerHandler *server, ClientInvoker *ci) :
		server_(server),
		ci_(ci),
		taskType_(VTask::NoTask),
		rescanNeed_(false),
		hasSuiteFilter_(false),
		autoAddNewSuites_(false)
{
}

ServerComThread::~ServerComThread()
{
	detach();
}

void ServerComThread::task(VTask_ptr task)
{
	// do not execute thread if already running

	if (isRunning())
	{
		UserMessage::message(UserMessage::ERROR, true, std::string("ServerComThread::sendCommand - thread already running, will not execute command"));
	}
	else
	{
		//if(!server_ && server)
		//	initObserver(server);

		//We set the parameters needed to run the task. These members are not protected by
		//a mutex, because apart from this function only run() can access them!!
		command_=task->command();
		params_=task->params();
		contents_=task->contents();
		vars_=task->vars();
		nodePath_.clear();
		taskType_=task->type();
		nodePath_=task->targetPath();

		//Suite filter
		hasSuiteFilter_=server_->suiteFilter()->isEnabled();
		autoAddNewSuites_=server_->suiteFilter()->autoAddNewSuites();
		filteredSuites_=server_->suiteFilter()->filter();

		//Start the thread execution
		start();
	}
}

void ServerComThread::run()
{
	//Can we use it? We are in the thread!!!
	//UserMessage::message(UserMessage::DBG, false, std::string("  ServerComThread::run start"));

	UserMessage::message(UserMessage::DBG, false, std::string("ServerComThread::run path: ") + nodePath_);

    //Init flags
    rescanNeed_=false;

	try
 	{
		switch (taskType_)
		{
			case VTask::CommandTask:
			{
				// call the client invoker with the saved command
				UserMessage::message(UserMessage::DBG, false, std::string(" COMMAND"));
				ArgvCreator argvCreator(command_);
				//UserMessage::message(UserMessage::DBG, false, argvCreator.toString());

				ci_->invoke(argvCreator.argc(), argvCreator.argv());

				/*ci_->news_local();
				switch (ci_->server_reply().get_news())
				{
					case ServerReply::NO_NEWS:
					case ServerReply::NEWS:
						ci_->sync_local();
						break;
					case ServerReply::DO_FULL_SYNC:

				break;
				}*/
				break;
			}

			case VTask::NewsTask:
			{
				UserMessage::message(UserMessage::DBG, false, std::string(" NEWS"));
				ci_->news_local(); // call the server
				break;
			}

			case VTask::SyncTask:
			{
				UserMessage::message(UserMessage::DBG, false, std::string(" SYNC"));
				sync_local();
				break;
			}

			//This is called during reset
			case VTask::ResetTask:
			{
				UserMessage::message(UserMessage::DBG, false, std::string(" RESET"));
				reset();
				break;
			}

			case VTask::JobTask:
			case VTask::ManualTask:
			case VTask::ScriptTask:
			case VTask::OutputTask:
			{
				UserMessage::message(UserMessage::DBG, false, std::string(" FILE"));
				ci_->file(nodePath_,params_["clientPar"]);
				break;
			}

			case VTask::MessageTask:
			{
				UserMessage::message(UserMessage::DBG, false, std::string(" EDIT HISTORY"));
				ci_->edit_history(nodePath_);
				break;
			}

			case VTask::StatsTask:
			{
				UserMessage::message(UserMessage::DBG, false, std::string(" STATS"));
				ci_->stats();
				break;
			}

			case VTask::HistoryTask:
			{
				UserMessage::message(UserMessage::DBG, false, std::string(" HISTORY"));
				ci_->getLog(100);
				break;
			}

			case VTask::ScriptPreprocTask:
				UserMessage::message(UserMessage::DBG, false, std::string(" SCRIP PREPROCESS"));
				ci_->edit_script_preprocess(nodePath_);
				break;

			case VTask::ScriptEditTask:
				UserMessage::message(UserMessage::DBG, false, std::string(" SCRIP EDIT"));
				ci_->edit_script_edit(nodePath_);
				break;

			case VTask::ScriptSubmitTask:
				UserMessage::message(UserMessage::DBG, false, std::string(" SCRIP SUBMIT"));
				ci_->edit_script_submit(nodePath_, vars_, contents_,
						(params_["alias"]=="1")?true:false,
						(params_["run"] == "1")?true:false);
				break;

			case VTask::SuiteListTask:
				UserMessage::message(UserMessage::DBG, false, std::string(" SUITES"));
				ci_->suites();
				break;

			case VTask::SuiteAutoRegisterTask:
				UserMessage::message(UserMessage::DBG, false, std::string(" SUITE AUTO REGISTER"));
				if(hasSuiteFilter_)
				{
					ci_->ch1_auto_add(autoAddNewSuites_);
				}
				break;

			case VTask::ZombieListTask:
				UserMessage::message(UserMessage::DBG, false, std::string(" ZOMBIES"));
				ci_->zombieGet();
				break;

			case VTask::LogOutTask:
				UserMessage::message(UserMessage::DBG, false, std::string(" LOGOUT"));
                detach();
                if(ci_->client_handle() > 0)
				{
					ci_->ch1_drop();
				}
				break;
			default:
				break;
		}
	}

	catch(std::exception& e)
	{
		// note that we need to emit a signal rather than directly call a message function
		// because we can't call Qt widgets from a worker thread

		std::string errorString = e.what();
		Q_EMIT failed(errorString);

		UserMessage::message(UserMessage::DBG, false, std::string("  ServerComThread::run failed: ") + errorString);

        //Reset flags
        rescanNeed_=false;

		//This will stop the thread.
		return;
	}

    //Reset flags
    rescanNeed_=false;

	//Can we use it? We are in the thread!!!
	//UserMessage::message(UserMessage::DBG, false, std::string("  ServerComThread::run finished"));
}


void ServerComThread::sync_local()
{
	//For this part we need to lock the mutex on defs
	{
		ServerDefsAccess defsAccess(server_);

		UserMessage::message(UserMessage::DBG, false, "ServerComThread::sync -- begin");
		ci_->sync_local();
		UserMessage::message(UserMessage::DBG, false, std::string("ServerComThread::sync -- end"));

		//If a rescan or fullscan is needed we have either added/remove nodes or deleted the defs.
		//So there were significant changes.

		//We detach the nodes currently available in defs, then we attach them again. We can still have nodes
		//that were removed from the defs but are still attached. These will be detached when in ServerHandler we
		//clear the tree. This tree contains shared pointers to the nodes, so when the tree is cleared
		//the shared pointer are reset, the node descturctor is called and finally update_delete is called and
		//we can detach the node.

		if(rescanNeed_ || ci_->server_reply().full_sync())
		{
			UserMessage::message(UserMessage::DBG, false, std::string("   --> rescan needed!"));
			detach(defsAccess.defs());
			attach(defsAccess.defs());
		}
	}

	/*if(rescanNeed_ || ci_->server_reply().full_sync())
	{
		updateRegSuites();
	}*/
}

void ServerComThread::reset()
{
	UserMessage::message(UserMessage::DBG, false,"ServerComThread::reset -- begin");

	//Lock the mutex on defs
	ServerDefsAccess defsAccess(server_);

    //Detach the defs and the nodes from the observer
    detach(defsAccess.defs());

	/// registering with empty set would lead
	//      to retrieve all server content,
	//      opposite of expected result

	//If we have already set a handle we
	//need to drop it.
	if(ci_->client_handle() > 0)
	{
		try
		{
			ci_->ch1_drop();
		}
		catch (std::exception &e)
		{
			UserMessage::message(UserMessage::DBG, false, std::string("no drop possible") + e.what());
		}
	}

	if(hasSuiteFilter_)
	{
		//reset client handle + defs
		ci_->reset();

		if(!filteredSuites_.empty())
		{
			UserMessage::message(UserMessage::DBG, false, std::string(" REGISTER SUITES"));

			//This will add a new handle to the client
			ci_->ch_register(autoAddNewSuites_, filteredSuites_);
		}
	}
	else
	{
		// reset client handle + defs
		ci_->reset();
	}

	UserMessage::message(UserMessage::DBG, false, std::string(" INIT SYNC"));
	ci_->sync_local();
	UserMessage::message(UserMessage::DBG, false, std::string(" INIT SYNC FINISHED"));

    //Attach the nodes to the observer
	attach(defsAccess.defs());

	UserMessage::message(UserMessage::DBG, false,"ServerComThread::reset -- end");
}


//Called from sync local!!!
void ServerComThread::updateRegSuites()
{
	if(!hasSuiteFilter_)
		return;

	//----------------------------------------
	// Get the registered list of suites!!
	//----------------------------------------
	try
	{
		ci_->ch_suites();
	}
	catch ( std::exception& e )
	{
		UserMessage::message(UserMessage::DBG, false, std::string("host::update-reg-suite-error:") + e.what());
	}

	const std::vector<std::pair<unsigned int, std::vector<std::string> > >& vct=ci_->server_reply().get_client_handle_suites();

	std::vector<std::string> regSuites;
	for(size_t i = 0; i < vct.size(); ++i)
	{
		if(vct[i].first == static_cast<unsigned int>(ci_->client_handle()))
		{
		    regSuites = vct[i].second;
		    break;
		}
    }

	//-----------------------------------------
	// Get the list of suites from the defs
	//-----------------------------------------
	const std::vector<suite_ptr>& defSuites = ci_->defs()->suiteVec();

	//-----------------------------------------------------------------------
	// If something is registered but not in the defs we need to remove it
	//-----------------------------------------------------------------------

	std::vector<std::string> delSuites;
	for(std::vector<std::string>::iterator it=regSuites.begin(); it != regSuites.end(); ++it)
	{
		bool found=0;
		for(size_t i = 0; i < defSuites.size(); ++i)
		{
			if(defSuites.at(i)->name() == *it)
			{
				found=true;
				break;
			}
		}
		if(!found)
		{
			delSuites.push_back(*it);
		}
	}

	if(!delSuites.empty())
	{
		ci_->ch_remove(ci_->client_handle(),delSuites);
	}

	//------------------------------------------------------
	// If something is loaded and in the filter but
	// not registered we need to register it!!
	//------------------------------------------------------

	if(!autoAddNewSuites_)
	{
		//get the list of loaded suites
		ci_->suites();
		const std::vector<std::string>& loadedSuites=ci_->server_reply().get_string_vec();

		std::vector<std::string> addSuites;
		for(std::vector<std::string>::const_iterator it=loadedSuites.begin(); it != loadedSuites.end(); ++it)
		{
			if(std::find(filteredSuites_.begin(),filteredSuites_.end(),*it) != filteredSuites_.end() &&
			   std::find(regSuites.begin(),regSuites.end(),*it) != regSuites.end())
			{
				addSuites.push_back(*it);
			}
		}

		if(!addSuites.empty())
		{
			ci_->ch_add(ci_->client_handle(),addSuites);
		}
	}

	UserMessage::message(UserMessage::DBG, false, std::string("Suite update finished"));
}

//This is an observer notification method!!
void ServerComThread::update(const Node* node, const std::vector<ecf::Aspect::Type>& types)
{
	//This function can only be called during a SYNC_LOCAl task!!!!
	assert(taskType_ == VTask::SyncTask);

	std::vector<ecf::Aspect::Type> typesCopy=types;

	UserMessage::message(UserMessage::DBG, false, std::string("ServerComThread::update - node: ") + node->name());
	for(std::vector<ecf::Aspect::Type>::const_iterator it=types.begin(); it != types.end(); ++it)
	{
		int i=*it;
		std::stringstream ss;
		ss << i;
		UserMessage::message(UserMessage::DBG, false, std::string(" aspect: ") + ss.str());
	}

    //If a node was already requested to be added/deleted in the thread we do not go further. At the end of the sync
	//we will regenerate everything (the tree as well in ServerHandle).
    if(rescanNeed_)
	{
		UserMessage::message(UserMessage::DBG, false, std::string(" -->  No signal emitted (rescan needed)"));
		return;
	}

    //This is a radical change
	if(std::find(types.begin(),types.end(),ecf::Aspect::ADD_REMOVE_NODE) != types.end())
	{
		UserMessage::message(UserMessage::DBG, false, std::string(" --> Rescan needed"));
		rescanNeed_=true;

		//We notify ServerHandler about the radical changes. When ServerHandler receives this signal
		//it will clear its tree, which stores shared pointers to the nodes (node_ptr). If these pointers are
		//reset update_delete() might be called, so it should not write any shared variables!
		Q_EMIT rescanNeed();

		return;

	}

    //This will notify SeverHandler
	UserMessage::message(UserMessage::DBG, false, std::string(" -->  nodeChanged() emitted"));
	Q_EMIT nodeChanged(node,types);
}


void ServerComThread::update(const Defs* dc, const std::vector<ecf::Aspect::Type>& types)
{
	std::vector<ecf::Aspect::Type> typesCopy=types;

	UserMessage::message(UserMessage::DBG, false, std::string("ServerComThread::update - defs: "));
	for(std::vector<ecf::Aspect::Type>::const_iterator it=types.begin(); it != types.end(); ++it)
	{
		int i=*it;
		std::stringstream ss;
		ss << i;
		UserMessage::message(UserMessage::DBG, false, std::string(" aspect: ") + ss.str());
	}

    //If anything was requested to be deleted in the thread we do not go further
    //because it will trigger a full rescan in ServerHandler!
    if(rescanNeed_)
	{
		UserMessage::message(UserMessage::DBG, false, std::string(" -->  No signal emitted (rescan needed)"));
	    return;
	}

    //This will notify SeverHandler
	UserMessage::message(UserMessage::DBG, false, std::string(" -->  defsChanged() emitted"));
	Q_EMIT defsChanged(typesCopy);
}

void ServerComThread::update_delete(const Node* nc)
{
	Node *n=const_cast<Node*>(nc);
	n->detach(this);
    //UserMessage::message(UserMessage::DBG, false, std::string("Update delete: ") + n->name());
}

//This only be called when ComThread is running or from the ComThread desctructor. So it is safe to set
//rescanNeed in it.
void ServerComThread::update_delete(const Defs* dc)
{
    UserMessage::message(UserMessage::DBG, false, std::string("Update defs delete: "));

    Defs *d=const_cast<Defs*>(dc);
    d->detach(this);

    //If we are in  a SYNC_LOCAl task!!!!
    if(taskType_ == VTask::SyncTask)
    {
    	//We notify ServerHandler about the radical changes. When ServerHandler receives this signal
    	//it will clear its tree, which stores shared pointers to the nodes (node_ptr). If these pointers are
    	//reset update_delete() might be called, so it should not write any shared variables!
    	Q_EMIT rescanNeed();

    	rescanNeed_=true;
    }
}

//Attach each node and the defs to the observer
void ServerComThread::attach()
{
	ServerDefsAccess defsAccess(server_);  // will reliquish its resources on destruction
	defs_ptr d = defsAccess.defs();
	if(d == NULL)
		return;

	attach(d);
}

//Attach  each node and the defs to the observer. The access to
//the defs is safe so we do not need to set a mutex on it.
void ServerComThread::attach(defs_ptr d)
{
	if(d == NULL)
		return;

	d->attach(this);

	const std::vector<suite_ptr> &suites = d->suiteVec();
	for(unsigned int i=0; i < suites.size();i++)
	{
		attach(suites.at(i).get());
	}
}

//Attach a node to the observer
void ServerComThread::attach(Node *node)
{
	std::vector<node_ptr> nodes;
	node->immediateChildren(nodes);

	node->attach(this);

	for(std::vector<node_ptr>::const_iterator it=nodes.begin(); it != nodes.end(); ++it)
	{
		attach((*it).get());
	}
}

//Detach each node and the defs from the observer
void ServerComThread::detach()
{
	ServerDefsAccess defsAccess(server_);  // will reliquish its resources on destruction
	defs_ptr d = defsAccess.defs();
	if(d == NULL)
		return;

	detach(d);
}


//Detach each node and the defs from the observer.  The access to
//the defs is safe so we do not need to set a mutex on it.
void ServerComThread::detach(defs_ptr d)
{
	if(d == NULL)
		return;

	d->detach(this);

	const std::vector<suite_ptr> &suites = d->suiteVec();
	for(unsigned int i=0; i < suites.size();i++)
	{
		detach(suites.at(i).get());
	}
}

//Detach a node from the observer
void ServerComThread::detach(Node *node)
{
	std::vector<node_ptr> nodes;
	node->immediateChildren(nodes);

	node->detach(this);

	for(std::vector<node_ptr>::const_iterator it=nodes.begin(); it != nodes.end(); ++it)
	{
		detach((*it).get());
	}
}
