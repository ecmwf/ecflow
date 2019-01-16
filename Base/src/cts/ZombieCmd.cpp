/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #24 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "CtsApi.hpp"
#include "AbstractClientEnv.hpp"
#include "Defs.hpp"
#include "Task.hpp"
#include "Str.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

std::ostream& ZombieCmd::print(std::ostream& os) const
{
	switch (user_action_) {
      case User::FOB:    return user_cmd(os,CtsApi::to_string(CtsApi::zombieFob(path_,process_id_,password_))); break;
      case User::FAIL:   return user_cmd(os,CtsApi::to_string(CtsApi::zombieFail(path_,process_id_,password_))); break;
      case User::ADOPT:  return user_cmd(os,CtsApi::to_string(CtsApi::zombieAdopt(path_,process_id_,password_))); break;
      case User::REMOVE: return user_cmd(os,CtsApi::to_string(CtsApi::zombieRemove(path_,process_id_,password_))); break;
      case User::BLOCK:  return user_cmd(os,CtsApi::to_string(CtsApi::zombieBlock(path_,process_id_,password_))); break;
      case User::KILL:   return user_cmd(os,CtsApi::to_string(CtsApi::zombieKill(path_,process_id_,password_))); break;
      default: break;
 	}
	return os;
}

bool ZombieCmd::equals(ClientToServerCmd* rhs) const
{
	ZombieCmd* the_rhs = dynamic_cast< ZombieCmd* > ( rhs );
	if ( !the_rhs ) return false;
	if (path_ != the_rhs->path_to_task()) return false;
	if (process_id_ != the_rhs->process_or_remote_id()) return false;
	if (password_ != the_rhs->password()) return false;
	return UserCmd::equals(rhs);
}

STC_Cmd_ptr ZombieCmd::doHandleRequest(AbstractServer* as) const
{
	// To uniquely identify a zombie we need path to task and remote_id, This information
	// is available from the zombie class via get command. However we do not want to
	// expose the password.
	// Hence the Command level interface  will make do with just the path to the task.
	// The first zombie whose corresponding task where password does *NOT* match is acted upon
	Task* task = NULL;
	if ( process_id_.empty() && password_.empty()) {
	   node_ptr node = as->defs()->findAbsNode(path_);
	   if (node.get()) task = node->isTask();
	}

	switch (user_action_) {

		case User::FOB:     {
			as->update_stats().zombie_fob_++;
			if ( process_id_.empty() && password_.empty())  as->zombie_ctrl().fobCli(path_,task);
			else                                            as->zombie_ctrl().fob(path_,process_id_,password_);
			break;
		}

		case User::FAIL:    {
			as->update_stats().zombie_fail_++;
			if ( process_id_.empty() && password_.empty()) as->zombie_ctrl().failCli(path_,task);
			else                                           as->zombie_ctrl().fail(path_,process_id_,password_);
			break;
		}

		case User::ADOPT:   {
			as->update_stats().zombie_adopt_++;
			if ( process_id_.empty() && password_.empty())  as->zombie_ctrl().adoptCli(path_,task);
			else                                            as->zombie_ctrl().adopt(path_,process_id_,password_);
			break;
		}

		case User::REMOVE:  {
			as->update_stats().zombie_remove_++;
			if ( process_id_.empty() && password_.empty())  as->zombie_ctrl().removeCli(path_,task);
			else                                            as->zombie_ctrl().remove(path_,process_id_,password_);
			break;
		}

		case User::BLOCK: {
			as->update_stats().zombie_block_++;
			if ( process_id_.empty() && password_.empty())  as->zombie_ctrl().blockCli(path_,task);
			else                                            as->zombie_ctrl().block(path_,process_id_,password_);
			break;
		}

      case User::KILL: {
         as->update_stats().zombie_kill_++;
         if ( process_id_.empty() && password_.empty())  as->zombie_ctrl().killCli(path_,task);
         else                                            as->zombie_ctrl().kill(path_,process_id_,password_);
         break;
      }
	}

	return PreAllocatedReply::ok_cmd();
}

const char* ZombieCmd::theArg() const {

	switch (user_action_) {
		case User::FOB:    return CtsApi::zombieFobArg(); break;
		case User::FAIL:   return CtsApi::zombieFailArg(); break;
		case User::ADOPT:  return CtsApi::zombieAdoptArg(); break;
		case User::REMOVE: return CtsApi::zombieRemoveArg(); break;
      case User::BLOCK:  return CtsApi::zombieBlockArg(); break;
      case User::KILL:   return CtsApi::zombieKillArg(); break;
		default: break;
 	}
	assert(false);
	return NULL;
}

void ZombieCmd::addOption(boost::program_options::options_description& desc) const
{
	switch (user_action_) {

		case User::FOB:     {
			desc.add_options()( CtsApi::zombieFobArg(), po::value< vector<string> >()->multitoken(),
			         "Locates the task in the zombie list, and sets to fob.\n"
			         "Next time the child commands (init,event,meter,label,abort,complete) communicate\n"
			         "with the server, they will complete successfully (but without updating the node tree)\n"
			         "allowing the job to finish.\n"
			         "The server zombie is automatically deleted after 1 hour\n"
			         "  arg = path to task   # Only a single path allowed\n"
			         "  --zombie_fob=/path/to/task"
			);
			break;
		}
		case User::FAIL:    {
			desc.add_options()( CtsApi::zombieFailArg(), po::value< vector<string> >()->multitoken(),
			         "Locates the task in the zombie list, and sets to fail.\n"
			         "Next time the child commands (init,event,meter,label,abort,complete) communicate\n"
			         "with the server, they will be set to fail. Depending on the job setup this may\n"
			         "force a abort, the abort will also fail.\n"
			         "Hence job structure should use 'set -e' in the error trapping functions to prevent\n"
			         "infinite recursion. The server zombie is automatically deleted after 1 hour\n"
			         "  arg = path to task   # Only a single path allowed\n"
                  "  --zombie_fail=/path/to/task"
			);
			break;
		}
		case User::ADOPT: {
			desc.add_options()( CtsApi::zombieAdoptArg(), po::value< vector<string> >()->multitoken(),
			         "Locates the task in the zombie list, and sets to adopt.\n"
			         "Next time the child commands (init,event,meter,label,abort,complete) communicate\n"
			         "with the server, the password on the zombie is adopted by the task.\n"
			         "The zombie is then deleted.\n"
			         "  arg = path to task   # Only a single path allowed\n"
                  "  --zombie_adopt=/path/to/task"
			);
			break;
		}
		case User::REMOVE:  {
			desc.add_options()( CtsApi::zombieRemoveArg(), po::value< vector<string> >()->multitoken(),
			         "Locates the task in the zombie list, and removes it.\n"
			         "Since a job typically has many child commands(i.e init, complete, event, meter, label)\n"
			         "the zombie may reappear\n"
			         "  arg = path to task   # Only a single path allowed\n"
                  "  --zombie_remove=/path/to/task"
			);
			break;
		}
		case User::BLOCK: {
			desc.add_options()( CtsApi::zombieBlockArg(), po::value< vector<string> >()->multitoken(),
			         "Locates the task in the zombie list, and blocks it.\n"
			         "This is default behaviour of the child commands(init,event,meter,label,abort,complete)\n"
			         "when the server can not match the passwords. Each child commands will continue\n"
			         "attempting to connect to the server for 24 hours, and will then return an error.\n"
			         "The connection timeout can be configured with environment ECF_TIMEOUT\n"
			         "  arg = path to task    # Only a single path allowed\n"
                  "  --zombie_block=/path/to/task"
			);
			break;
		}
      case User::KILL: {
         desc.add_options()( CtsApi::zombieKillArg(), po::value< vector<string> >()->multitoken(),
                  "Locates the task in the zombie list, and kills the associated job.\n"
                  "The kill is done using ECF_KILL_CMD, but using the process_id from the zombie\n"
                  "The job is allowed to continue until the kill is received\n"
                  "Can only kill zombies that have an associated Task, hence path zombies\n"
                  "must be killed manually.\n"
                  "  arg = path to task    # Only a single path allowed \n"
                  "  --zombie_kill=/path/to/task"
         );
         break;
      }
		default: assert(false); break;
 	}
}

void ZombieCmd::create( Cmd_ptr& cmd,
						boost::program_options::variables_map& vm,
						AbstractClientEnv* ace ) const
{
 	vector<string> args = vm[  theArg() ].as< vector<string> >();
	if (ace->debug()) dumpVecArgs( theArg(), args);

	// For Command Line Interface only the task_path is provided. Just have to make do.
	// arg1 = task_path
	// arg2 = process_or_remote_id ( empty for CLI)
	// arg3 = password             ( empty for CLI)
	std::string path;
	std::string process_or_remote_id;
	std::string password;
	for(size_t i = 0; i < args.size(); i++) {
		if (i == 0 ) path =  args[i];
		if (i == 1 ) process_or_remote_id =  args[i];
		if (i == 2 ) password =  args[i];
	}
	if (path.empty()) {
		throw std::runtime_error("ZombieCmd::create: expects at least one argument. path to task");
	}

   if (ace->get_cli()) {
      // We are using command line, only a single path argument is acceptable
      if (!process_or_remote_id.empty()) {
         throw std::runtime_error("ZombieCmd::create: expects one argument. path to task, i.e /path/to/task");
      }
      if (!password.empty()) {
         throw std::runtime_error("ZombieCmd::create: expects one argument. path to task, i.e /path/to/task");
      }
   }

   if (path.find('/') == std::string::npos) {
      throw std::runtime_error("ZombieCmd::create: Not a valid path. Expects one argument. path to task, i.e /path/to/task");
   }

	cmd = Cmd_ptr(new ZombieCmd(user_action_, path, process_or_remote_id, password ));
}

std::ostream& operator<<(std::ostream& os, const ZombieCmd& c) { return c.print(os); }
