/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #52 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/make_shared.hpp>
#include "CtsCmdRegistry.hpp"
#include "ClientToServerCmd.hpp"
#include "AbstractClientEnv.hpp"
namespace po = boost::program_options;


CtsCmdRegistry::CtsCmdRegistry(bool addGroupCmd)
{
	// If a new client to server command is added. Make sure to add it here.
	// Could have used static initialisation' but this is less problematic.

   /// The order dictates how the --help is shown *BUT* Since we traverse this list.
   /// Place the command which require the fastest response first.
   vec_.reserve(70);

	vec_.push_back( boost::make_shared<CSyncCmd>(CSyncCmd::NEWS,0,0,0));
   vec_.push_back( boost::make_shared<CSyncCmd>(CSyncCmd::SYNC,0,0,0));
   vec_.push_back( boost::make_shared<CSyncCmd>(0)); // SYNC_FULL
   vec_.push_back( boost::make_shared<CtsNodeCmd>(CtsNodeCmd::GET));
   vec_.push_back( boost::make_shared<CtsNodeCmd>(CtsNodeCmd::GET_STATE));
   vec_.push_back( boost::make_shared<CtsNodeCmd>(CtsNodeCmd::MIGRATE));
   vec_.push_back( boost::make_shared<CheckPtCmd>());
	vec_.push_back( boost::make_shared<CtsCmd>(CtsCmd::PING));
	vec_.push_back( boost::make_shared<CtsCmd>(CtsCmd::RESTORE_DEFS_FROM_CHECKPT));
	vec_.push_back( boost::make_shared<CtsCmd>(CtsCmd::RESTART_SERVER));
	vec_.push_back( boost::make_shared<CtsCmd>(CtsCmd::HALT_SERVER));
	vec_.push_back( boost::make_shared<CtsCmd>(CtsCmd::SHUTDOWN_SERVER));
	vec_.push_back( boost::make_shared<CtsCmd>(CtsCmd::TERMINATE_SERVER));
	vec_.push_back( boost::make_shared<CtsCmd>(CtsCmd::RELOAD_WHITE_LIST_FILE));
	vec_.push_back( boost::make_shared<CtsCmd>(CtsCmd::FORCE_DEP_EVAL));
   vec_.push_back( boost::make_shared<CtsCmd>(CtsCmd::STATS));
   vec_.push_back( boost::make_shared<CtsCmd>(CtsCmd::STATS_RESET));
   vec_.push_back( boost::make_shared<CtsCmd>(CtsCmd::DEBUG_SERVER_ON));
   vec_.push_back( boost::make_shared<CtsCmd>(CtsCmd::DEBUG_SERVER_OFF));
   vec_.push_back( boost::make_shared<CtsCmd>(CtsCmd::SERVER_LOAD));
   vec_.push_back( boost::make_shared<CtsNodeCmd>(CtsNodeCmd::JOB_GEN));
   vec_.push_back( boost::make_shared<CtsNodeCmd>(CtsNodeCmd::CHECK_JOB_GEN_ONLY));
   vec_.push_back( boost::make_shared<PathsCmd>(PathsCmd::DELETE));
   vec_.push_back( boost::make_shared<PathsCmd>(PathsCmd::SUSPEND));
   vec_.push_back( boost::make_shared<PathsCmd>(PathsCmd::RESUME));
   vec_.push_back( boost::make_shared<PathsCmd>(PathsCmd::KILL));
   vec_.push_back( boost::make_shared<PathsCmd>(PathsCmd::STATUS));
   vec_.push_back( boost::make_shared<PathsCmd>(PathsCmd::CHECK));
   vec_.push_back( boost::make_shared<PathsCmd>(PathsCmd::EDIT_HISTORY));
	vec_.push_back( boost::make_shared<ZombieCmd>(ecf::User::FOB));
	vec_.push_back( boost::make_shared<ZombieCmd>(ecf::User::FAIL));
	vec_.push_back( boost::make_shared<ZombieCmd>(ecf::User::ADOPT));
	vec_.push_back( boost::make_shared<ZombieCmd>(ecf::User::BLOCK));
   vec_.push_back( boost::make_shared<ZombieCmd>(ecf::User::REMOVE));
   vec_.push_back( boost::make_shared<ZombieCmd>(ecf::User::KILL));
	vec_.push_back( boost::make_shared<CtsCmd>(CtsCmd::GET_ZOMBIES));
   vec_.push_back( boost::make_shared<CtsCmd>(CtsCmd::SUITES));
	vec_.push_back( boost::make_shared<ClientHandleCmd>(ClientHandleCmd::REGISTER));
   vec_.push_back( boost::make_shared<ClientHandleCmd>(ClientHandleCmd::DROP));
   vec_.push_back( boost::make_shared<ClientHandleCmd>(ClientHandleCmd::DROP_USER));
	vec_.push_back( boost::make_shared<ClientHandleCmd>(ClientHandleCmd::ADD));
	vec_.push_back( boost::make_shared<ClientHandleCmd>(ClientHandleCmd::REMOVE));
   vec_.push_back( boost::make_shared<ClientHandleCmd>(ClientHandleCmd::AUTO_ADD));
   vec_.push_back( boost::make_shared<ClientHandleCmd>(ClientHandleCmd::SUITES));
   vec_.push_back( boost::make_shared<LogCmd>());
   vec_.push_back( boost::make_shared<ServerVersionCmd>());
	vec_.push_back( boost::make_shared<LogMessageCmd>());
	vec_.push_back( boost::make_shared<BeginCmd>());
	vec_.push_back( boost::make_shared<InitCmd>());
	vec_.push_back( boost::make_shared<CompleteCmd>());
	vec_.push_back( boost::make_shared<AbortCmd>());
	vec_.push_back( boost::make_shared<CtsWaitCmd>());
	vec_.push_back( boost::make_shared<EventCmd>());
	vec_.push_back( boost::make_shared<MeterCmd>());
	vec_.push_back( boost::make_shared<LabelCmd>());
	vec_.push_back( boost::make_shared<RequeueNodeCmd>());
	vec_.push_back( boost::make_shared<OrderNodeCmd>());
	vec_.push_back( boost::make_shared<RunNodeCmd>());
	vec_.push_back( boost::make_shared<ForceCmd>());
	vec_.push_back( boost::make_shared<FreeDepCmd>());
	vec_.push_back( boost::make_shared<LoadDefsCmd>());
	vec_.push_back( boost::make_shared<ReplaceNodeCmd>());
	vec_.push_back( boost::make_shared<CFileCmd>());
	vec_.push_back( boost::make_shared<EditScriptCmd>());
	vec_.push_back( boost::make_shared<AlterCmd>());
	vec_.push_back( boost::make_shared<PlugCmd>());
	// Note: we deliberately do not add MoveCmd, as it should not appear in the public api
	//       It is created on the fly by the PlugCmd

	/// Command that can *ONLY* be used in a group command
	vec_.push_back( boost::make_shared<CtsNodeCmd>(CtsNodeCmd::WHY));
	vec_.push_back( boost::make_shared<ShowCmd>());
	if (addGroupCmd) vec_.push_back( boost::make_shared<GroupCTSCmd>());
}

bool CtsCmdRegistry::parse(Cmd_ptr& cmd,
                           boost::program_options::variables_map& vm,
                           AbstractClientEnv* clientEnv ) const
{
	size_t vec_size = vec_.size();
	for(size_t i = 0; i < vec_size; i++) {

		if ( vm.count( vec_[i]->theArg() ) ) {

			if (clientEnv->debug()) std::cout << "CtsCmdRegistry::parse matched with registered command " << vec_[i]->theArg() << "\n";

			vec_[i]->create(cmd,vm,clientEnv);
			return true;
		}
	}
	return false;
}

void CtsCmdRegistry::addAllOptions(boost::program_options::options_description& desc) const
{
	addCmdOptions(desc);
	addHelpOption(desc);
}

void CtsCmdRegistry::addCmdOptions(boost::program_options::options_description& desc) const
{
   size_t vec_size = vec_.size();
 	for(size_t i = 0; i < vec_size; i++) {
		vec_[i]->addOption(desc);
	}
}

void CtsCmdRegistry::addHelpOption(boost::program_options::options_description& desc) const
{
	/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
   desc.add_options() ("help,h",po::value< std::string >()->implicit_value( std::string("") ), "Produce help message");
   desc.add_options() ("version,v", "Show ecflow client version number, and version of the boost library used" );
	desc.add_options() ("debug,d",
			"Dump out client environment settings for debug\n"
			"Set environment variable ECF_DEBUG_CLIENT for additional debug" );
}
