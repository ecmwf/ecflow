//============================================================================
// Copyright 2009-2017 ECMWF.
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
#include "UiLog.hpp"

#include <algorithm>

#define _UI_SERVERCOMTHREAD_DEBUG

ServerComThread::ServerComThread(ServerHandler *server, ClientInvoker *ci) :
        server_(server),
        ci_(ci),
        taskType_(VTask::NoTask),
        rescanNeed_(false),
        hasSuiteFilter_(false),
        autoAddNewSuites_(false),
        maxLineNum_(-1),
        initialResetDone_(false)
{
    assert(server_);
}

ServerComThread::~ServerComThread()
{
    detach();
}

void ServerComThread::task(VTask_ptr task)
{
    //do not execute thread if already running

    if(isRunning())
    {
        UiLog(serverName_).err() << "ComThread::task - thread already running, will not execute command";
    }
    else
    {
        //if(!server_ && server)
        //    initObserver(server);

        //We set the parameters needed to run the task. These members are not protected by
        //a mutex, because apart from this function only run() can access them!!
        serverName_=server_->longName();
        command_=task->command();
        params_=task->params();
        contents_=task->contents();
        vars_=task->vars();
        nodePath_.clear();
        taskType_=task->type();
        nodePath_=task->targetPath();
        zombie_=task->zombie();

        //Suite filter
        hasSuiteFilter_=server_->suiteFilter()->isEnabled();
        autoAddNewSuites_=server_->suiteFilter()->autoAddNewSuites();
        if(hasSuiteFilter_)
            filteredSuites_=server_->suiteFilter()->filter();
        else
            filteredSuites_.clear();

        maxLineNum_=server_->conf()->intValue(VServerSettings::MaxOutputFileLines);

        //Start the thread execution
        start();
    }
}

void ServerComThread::run()
{    
    UiLog(serverName_).dbg() << "ComThread::run --> path="  <<  nodePath_;

    //Init flags
    rescanNeed_=false;
    bool isMessage = false;
    std::string errorString;

    try
    {
        switch (taskType_)
        {
            case VTask::CommandTask:
            {
                // call the client invoker with the saved command
                UiLog(serverName_).dbg() << " COMMAND";
                ArgvCreator argvCreator(command_);
#ifdef _UI_SERVERCOMTHREAD_DEBUG
                UiLog(serverName_).dbg() << " args="  << argvCreator.toString();
#endif
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
                UiLog(serverName_).dbg() << " NEWS";
                ci_->news_local(); // call the server
                break;
            }

            case VTask::SyncTask:
            {
                UiLog(serverName_).dbg() << " SYNC";
                sync_local();
                break;
            }

            //This is called during reset
            case VTask::ResetTask:
            {
                UiLog(serverName_).dbg() << " SYNC";
                reset();
                break;
            }

            case VTask::JobTask:
            case VTask::ManualTask:
            case VTask::ScriptTask:
            case VTask::OutputTask:
            {
                UiLog(serverName_).dbg() << " FILE" << " " << params_["clientPar"];
                if(maxLineNum_ < 0)
                    ci_->file(nodePath_,params_["clientPar"]);
                else
                    ci_->file(nodePath_,params_["clientPar"],boost::lexical_cast<std::string>(maxLineNum_));

                break;
            }

            case VTask::MessageTask:
            {
                UiLog(serverName_).dbg() << " EDIT HISTORY";
                ci_->edit_history(nodePath_);
                break;
            }

            case VTask::StatsTask:
            {
                UiLog(serverName_).dbg() << " STATS";
                ci_->stats();
                break;
            }

            case VTask::HistoryTask:
            {
                UiLog(serverName_).dbg() << " SERVER LOG";
                ci_->getLog(100);
                break;
            }

            case VTask::ScriptPreprocTask:
                UiLog(serverName_).dbg() << " SCRIP PREPROCESS";
                ci_->edit_script_preprocess(nodePath_);
                break;

            case VTask::ScriptEditTask:
                UiLog(serverName_).dbg() << " SCRIP EDIT";
                ci_->edit_script_edit(nodePath_);
                break;

            case VTask::ScriptSubmitTask:
                UiLog(serverName_).dbg() << " SCRIP SUBMIT";
                ci_->edit_script_submit(nodePath_, vars_, contents_,
                        (params_["alias"]=="1")?true:false,
                        (params_["run"] == "1")?true:false);
                break;

            case VTask::SuiteListTask:
                UiLog(serverName_).dbg() << " SUITES";
                ci_->suites();
                break;

            case VTask::SuiteAutoRegisterTask:
                UiLog(serverName_).dbg() << " SUITE AUTO REGISTER";
                if(hasSuiteFilter_)
                {
                    ci_->ch1_auto_add(autoAddNewSuites_);
                }
                break;

            case VTask::ZombieCommandTask:
            {
                std::string cmd=params_["command"];
                UiLog(serverName_).dbg() << " ZOMBIE COMMAND " << cmd << " path=" << zombie_.path_to_task();
                if(cmd == "zombie_fob")
                    ci_->zombieFob(zombie_);
                else if(cmd == "zombie_fail")
                    ci_->zombieFail(zombie_);
                else if(cmd == "zombie_adopt")
                    ci_->zombieAdopt(zombie_);
                else if(cmd == "zombie_block")
                    ci_->zombieBlock(zombie_);
                else if(cmd == "zombie_remove")
                    ci_->zombieRemove(zombie_);
                else if(cmd == "zombie_kill")
                    ci_->zombieKill(zombie_);

                break;
            }

            case VTask::ZombieListTask:
                UiLog(serverName_).dbg() << " ZOMBIES";
                ci_->zombieGet();
                break;

            case VTask::LogOutTask:
                UiLog(serverName_).dbg() << " LOGOUT";
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
        isMessage = true;
        errorString = e.what();
    }

    // we can get an error string in one of two ways - either an exception is raised, or
    // the get_string() of the server reply is non-empty.
    if (!isMessage && (taskType_ == VTask::CommandTask) && !(ci_->server_reply().get_string().empty()))
    {
        isMessage = true;
        errorString = ci_->server_reply().get_string();
    }

    if (isMessage)
    {
        // note that we need to emit a signal rather than directly call a message function
        // because we can't call Qt widgets from a worker thread

        UiLog(serverName_).dbg() << " thread failed: " <<  errorString;
        Q_EMIT failed(errorString);

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

        UiLog(serverName_).dbg() << "ComThread::sync_local --> sync begin";
        ci_->sync_local();
        UiLog(serverName_).dbg() << " sync end";

        //If a rescan or fullscan is needed we have either added/remove nodes or deleted the defs.
        //So there were significant changes.

        //We detach the nodes currently available in defs, then we attach them again. We can still have nodes
        //that were removed from the defs but are still attached. These will be detached when in ServerHandler we
        //clear the tree. This tree contains shared pointers to the nodes, so when the tree is cleared
        //the shared pointer are reset, the node descturctor is called and finally update_delete is called and
        //we can detach the node.

        if(rescanNeed_ || ci_->server_reply().full_sync())
        {
            UiLog(serverName_).dbg() << " rescan needed!";
            detach(defsAccess.defs());
            attach(defsAccess.defs());
        }
    }
}

void ServerComThread::reset()
{
    UiLog(serverName_).dbg() << "ComThread::reset -->";

    //Lock the mutex on defs
    ServerDefsAccess defsAccess(server_);

    //Detach the defs and the nodes from the observer
    detach(defsAccess.defs());

    //We drop all the handles belonging to the current user to have a proper clean-up!
    //Other running instances of ecflow_ui under the same user will properly react
    //to these changes and reset their handles! So it is a safe operation!!!
    try
    {
        //If it is not the first reset we drop the handle belonging to the current ecflowui instance
        if(initialResetDone_)
        {
            if(ci_->client_handle() > 0)
            {
                ci_->ch1_drop();
            }
        }

        //If it is the first reset we need a proper clean-up!
        //We drop all the handles belonging to the current user!
        //Other running instances of ecflow_ui under the same user will properly react
        //to these changes and reset their handles! So it is a safe operation!!!
        else
        {
           initialResetDone_=true;
           ci_->ch_drop_user();
        }

    }
    catch (std::exception &e)
    {
        UiLog(serverName_).warn() << " cannot drop handle for current user: " << e.what();
    }

    if(hasSuiteFilter_)
    {
        //reset client handle + defs
        ci_->reset();

        if(!filteredSuites_.empty())
        {
            UiLog(serverName_).dbg() << " register suites=" << filteredSuites_;

            //This will add a new handle to the client
            ci_->ch_register(autoAddNewSuites_, filteredSuites_);
        }
        //If the suite filter is empty
        else
        {
            //Registering with empty set would lead to retrieve all server content,
            //opposite of expected result. So we just register a dummy suite
            //to achive the our goal: for an empty suite filter no suites are retrieved.
            UiLog(serverName_).dbg() << " register empty suite list";

            std::vector<std::string> fsl;
            fsl.push_back(SuiteFilter::dummySuite());
            ci_->ch_register(autoAddNewSuites_, fsl);
        }
    }
    else
    {
        // reset client handle + defs
        ci_->reset();
    }

    UiLog(serverName_).dbg() << " sync begin";
    ci_->sync_local();
    UiLog(serverName_).dbg() << " sync end";

    //Attach the nodes to the observer
    attach(defsAccess.defs());

    UiLog(serverName_).dbg() << "<-- ComThread::reset";
}

//This is an observer notification method!!
void ServerComThread::update(const Node* node, const std::vector<ecf::Aspect::Type>& types)
{
    //This function can only be called during a SYNC_LOCAl task!!!!
    assert(taskType_ == VTask::SyncTask);

    std::vector<ecf::Aspect::Type> typesCopy=types;

    UiLog(serverName_).dbg() << "ComThread::update --> node: " << node->name();
    std::stringstream ss;
    aspectToStr(ss,types);
    UiLog(serverName_).dbg() << " aspects: " << ss.str();

    //If a node was already requested to be added/deleted in the thread we do not go further. At the end of the sync
    //we will regenerate everything (the tree as well in ServerHandle).
    if(rescanNeed_)
    {
        UiLog(serverName_).dbg() << " rescanNeed already set";
        return;
    }

    //This is a radical change
    if((std::find(types.begin(),types.end(),ecf::Aspect::ADD_REMOVE_NODE) != types.end()) ||
       (std::find(types.begin(),types.end(),ecf::Aspect::ORDER)           != types.end()))
    {
        UiLog(serverName_).dbg() << " emit rescanNeed()";
        rescanNeed_=true;

        //We notify ServerHandler about the radical changes. When ServerHandler receives this signal
        //it will clear its tree, which stores shared pointers to the nodes (node_ptr). If these pointers are
        //reset update_delete() might be called, so it should not write any shared variables!
        Q_EMIT rescanNeed();

        return;
    }

    //This will notify SeverHandler
    UiLog(serverName_).dbg() << " emit nodeChanged()";
    Q_EMIT nodeChanged(node,types);
}


void ServerComThread::update(const Defs* dc, const std::vector<ecf::Aspect::Type>& types)
{
    std::vector<ecf::Aspect::Type> typesCopy=types;

    UiLog(serverName_).dbg() << "ComThread::update --> defs";
    std::stringstream ss;
    aspectToStr(ss,types);
    UiLog(serverName_).dbg() << " aspects: " << ss.str();

    //If anything was requested to be deleted in the thread we do not go further
    //because it will trigger a full rescan in ServerHandler!
    if(rescanNeed_)
    {
        UiLog(serverName_).dbg() << " rescanNeed already set";
        return;
    }

    //This is a radical change
    if(std::find(types.begin(),types.end(),ecf::Aspect::ORDER) != types.end())
    {
        UiLog(serverName_).dbg() << " emit rescanNeed()";
        rescanNeed_=true;

        //We notify ServerHandler about the radical changes. When ServerHandler receives this signal
        //it will clear its tree, which stores shared pointers to the nodes (node_ptr). If these pointers are
        //reset update_delete() might be called, so it should not write any shared variables!
        Q_EMIT rescanNeed();

        return;
    }

    //This will notify SeverHandler
    UiLog(serverName_).dbg() << " emit defsChanged()";
    Q_EMIT defsChanged(typesCopy);
}

void ServerComThread::update_delete(const Node* nc)
{
    auto *n=const_cast<Node*>(nc);
    n->detach(this);
}

//This only be called when ComThread is running or from the ComThread desctructor. So it is safe to set
//rescanNeed in it.
void ServerComThread::update_delete(const Defs* dc)
{
    UiLog(serverName_).dbg() << "ServerComThread::update_delete -->";

    auto *d=const_cast<Defs*>(dc);
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
    if(d == nullptr)
        return;

    attach(d);
}

//Attach  each node and the defs to the observer. The access to
//the defs is safe so we do not need to set a mutex on it.
void ServerComThread::attach(defs_ptr d)
{
    if(d == nullptr)
        return;

    d->attach(this);

    const std::vector<suite_ptr> &suites = d->suiteVec();
    for(const auto & suite : suites)
    {
        attach(suite.get());
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
    if(d == nullptr)
        return;

    detach(d);
}


//Detach each node and the defs from the observer.  The access to
//the defs is safe so we do not need to set a mutex on it.
void ServerComThread::detach(defs_ptr d)
{
    if(d == nullptr)
        return;

    d->detach(this);

    const std::vector<suite_ptr> &suites = d->suiteVec();
    for(const auto & suite : suites)
    {
        detach(suite.get());
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

void ServerComThread::aspectToStr(std::stringstream& ss,const std::vector<ecf::Aspect::Type>& t) const
{
    for(auto it : t)
    {
        if(!ss.str().empty())
            ss << ",";
        ss << it;
    }
}
