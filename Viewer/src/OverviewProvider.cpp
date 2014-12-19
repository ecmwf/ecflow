//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OverviewProvider.hpp"

#include "ServerHandler.hpp"
#include "VNState.hpp"
#include "VSState.hpp"

OverviewProvider::OverviewProvider(InfoPresenter* owner) : InfoProvider(owner)
{

}

//==================================================
// Server
//==================================================

void OverviewProvider::visit(VInfoServer* info)
{
	reply_->reset();

	//Build the second part of the server info. We do not need
	//information from the ClientInvoker for this!!
	std::stringstream ss;
	serverInfo(info,ss);
	reply_->text(ss.str());

	//Define a task for getting the stats from the server.
	//We need ClientInvoker for this
	task_=VTask::create(VTask::StatsTask,this);

	//Run the task in the server. When it finish taskFinished() is called. The text returned
	//in the reply will be prepended to the string we generated above.
	info->server()->run(task_);
}

//Node
void OverviewProvider::visit(VInfoNode* ni)
{
	reply_->reset();
	std::stringstream ss;
	nodeInfo(ni,ss);
	reply_->text(ss.str());
	owner_->infoReady(reply_);
}

void OverviewProvider::visit(VInfoAttribute* ptr)
{
}

void  OverviewProvider::taskChanged(VTask_ptr task)
{
	if(task_ != task)
		return;

	switch(task->status())
	{
		case VTask::FINISHED:
			//We prepend the results to the existing text
			reply_->prependText(task->reply()->text());
			owner_->infoReady(reply_);
			//We do not need the task anymore.
			task_.reset();
			break;
		case VTask::ABORTED:
		case VTask::CANCELLED:
		case VTask::REJECTED:
			reply_->prependText(task->reply()->text());
			owner_->infoFailed(reply_);
			//We do not need the task anymore.
			task_.reset();break;
		default:
			break;
	}
}


void OverviewProvider::serverInfo(VInfoServer* info,std::stringstream& f)
{
	ServerHandler *server=info->server();

	if(!server) return;

	static const std::string inc = "  ";

	using namespace boost::posix_time;
	using namespace boost::gregorian;

	std::string typeName="server";
	std::string nodeName=server->name();
	std::string statusName(VSState::toName(server).toStdString());

	//Header
	f << "name    : " << nodeName << "\n";
	f << "type    : " << typeName << "\n";
	f << "status   : " << statusName << "\n";

    f << "----------\n";
    //Start block: Type, name
    f << typeName << " " << server->name() << "\n";

    //End block
    f << "end" << typeName << " # " << nodeName << "\n";
}

void OverviewProvider::nodeInfo(VInfoNode* info,std::stringstream& f)
{
	ServerHandler *server=info->server();
	if(!server) return;

	Node* node=info->node();
	if(!node) return;

	static const std::string inc = "  ";

	using namespace boost::posix_time;
	using namespace boost::gregorian;

	std::string typeName=info->nodeType();
	std::string nodeName=node->name();
	std::string statusName(VNState::toName(node).toStdString());

	//Header
	f << "name    : " << nodeName << "\n";
	f << "type    : " << typeName << "\n";
	f << "status   : " << statusName << "\n";

	boost::posix_time::ptime state_change_time = node->state_change_time();
	if(!state_change_time.is_special())
	{
		f << "at      : " << boost::posix_time::to_simple_string(state_change_time) << "\n";
	}

	f << "----------\n";

	//Start block: Type, name
	f << typeName << " " << node->name() << "\n";

	//Clock information for suites
	if(node->isSuite())
	{
		Suite* suite = dynamic_cast<Suite*>(node);
		// f << "clock    : ";
		if (suite->clockAttr())
		{
			suite->clockAttr().get()->print(f); // f << "\n";
		}
	}

	//Default status: the status the node should have when the begin/re-queue is called
	//if(st  != DState::QUEUED && st != DState::UNKNOWN)
	f << inc << "defstatus " <<  VNState::toDefaultStateName(node).toStdString() << "\n";

	//Zombies attribute
	const std::vector<ZombieAttr> & vect = node->zombies();
	for (std::vector<ZombieAttr>::const_iterator it = vect.begin(); it != vect.end(); ++it)
		f << inc << it->toString() << "\n";

	//Autocancel
	if(node->hasAutoCancel() && node->get_autocancel())
		f << inc << node->get_autocancel()->toString() << "\n";

	f << inc << "# " << typeName << " " << nodeName << " is " << statusName << "\n";

	if(node->hasTimeDependencies())
	{
		f << inc << "# time-date-dependencies: ";
		if (node->isTimeFree()) f << "free\n";
			else f << "holding\n";
	}

	//Generated variables
	std::vector<Variable> gvar;
	std::vector<Variable>::const_iterator gvar_end;
	node->gen_variables(gvar);
	for(std::vector<Variable>::const_iterator it = gvar.begin(); it != gvar.end(); ++it)
	{
		f << inc << "# edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
	}

	//Variables
	gvar = node->variables();
	for(std::vector<Variable>::const_iterator it = gvar.begin(); it != gvar.end(); ++it)
	{
		f << inc << "edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
	}

	//Print children
	std::vector<node_ptr> nodes;
	node->immediateChildren(nodes);
	for(unsigned int i=0; i < nodes.size(); i++)
	{
		f << inc << info->nodeType(nodes.at(i).get()) << " " << nodes.at(i)->name() << "\n";
	}

	//Here we should print some additional information from the attributes well. It i not clear exactly what!

	//End block
	f << "end" << typeName << " # " << nodeName << "\n";
}







