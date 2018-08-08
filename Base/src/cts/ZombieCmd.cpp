/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #24 $ 
//
// Copyright 2009-2017 ECMWF.
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
      case User::FOB:    return user_cmd(os,CtsApi::to_string(CtsApi::zombieFob(paths_,process_id_,password_))); break;
      case User::FAIL:   return user_cmd(os,CtsApi::to_string(CtsApi::zombieFail(paths_,process_id_,password_))); break;
      case User::ADOPT:  return user_cmd(os,CtsApi::to_string(CtsApi::zombieAdopt(paths_,process_id_,password_))); break;
      case User::REMOVE: return user_cmd(os,CtsApi::to_string(CtsApi::zombieRemove(paths_,process_id_,password_))); break;
      case User::BLOCK:  return user_cmd(os,CtsApi::to_string(CtsApi::zombieBlock(paths_,process_id_,password_))); break;
      case User::KILL:   return user_cmd(os,CtsApi::to_string(CtsApi::zombieKill(paths_,process_id_,password_))); break;
      default: break;
 	}
	return os;
}

bool ZombieCmd::equals(ClientToServerCmd* rhs) const
{
	auto* the_rhs = dynamic_cast< ZombieCmd* > ( rhs );
	if ( !the_rhs ) return false;
	if (paths_ != the_rhs->paths()) return false;
	if (process_id_ != the_rhs->process_or_remote_id()) return false;
	if (password_ != the_rhs->password()) return false;
	return UserCmd::equals(rhs);
}

STC_Cmd_ptr ZombieCmd::doHandleRequest(AbstractServer* as) const
{
   switch (user_action_) {
      case User::FOB:    as->update_stats().zombie_fob_++; break;
      case User::FAIL:   as->update_stats().zombie_fail_++; break;
      case User::ADOPT:  as->update_stats().zombie_adopt_++; break;
      case User::REMOVE: as->update_stats().zombie_remove_++; break;
      case User::BLOCK:  as->update_stats().zombie_block_++; break;
      case User::KILL:   as->update_stats().zombie_kill_++; break;
   }

	// To uniquely identify a zombie we need path to task and remote_id, This information
	// is available from the zombie class via get command. However we do not want to expose the password.
	// Hence the Command level interface  will make do with just the path to the task.
	// The first zombie whose corresponding task where password does *NOT* match is acted upon
	if ( process_id_.empty() && password_.empty()) {
	   for(const auto & path : paths_) {
	      node_ptr node = as->defs()->findAbsNode(path);
	      Task* task = nullptr;
	      if (node.get()) task = node->isTask();
	      switch (user_action_) {
	         case User::FOB:   as->zombie_ctrl().fobCli(path,task); break;
	         case User::FAIL:  as->zombie_ctrl().failCli(path,task); break;
	         case User::ADOPT: as->zombie_ctrl().adoptCli(path,task); break;
	         case User::REMOVE:as->zombie_ctrl().removeCli(path,task); break;
	         case User::BLOCK: as->zombie_ctrl().blockCli(path,task); break;
	         case User::KILL:  as->zombie_ctrl().killCli(path,task); break;
	      }
	   }
	}
	else {
	   // expect a single path, process_id and password only apply for a single task
	   if (paths_.size() == 1) {
	      switch (user_action_) {
	         case User::FOB:   as->zombie_ctrl().fob(paths_[0],process_id_,password_); break;
	         case User::FAIL:  as->zombie_ctrl().fail(paths_[0],process_id_,password_); break;
	         case User::ADOPT: as->zombie_ctrl().adopt(paths_[0],process_id_,password_);  break;
	         case User::REMOVE:as->zombie_ctrl().remove(paths_[0],process_id_,password_); break;
	         case User::BLOCK: as->zombie_ctrl().block(paths_[0],process_id_,password_);  break;
	         case User::KILL:  as->zombie_ctrl().kill(paths_[0],process_id_,password_); break;
	      }
	   }
	   else {
	      return PreAllocatedReply::error_cmd("ZombieCmd: process_or_remote_id and password can not be used when multiple paths are specified. Please specify a single path");
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
	return nullptr;
}

void ZombieCmd::addOption(boost::program_options::options_description& desc) const
{
	switch (user_action_) {

		case User::FOB:     {
			desc.add_options()( CtsApi::zombieFobArg(), po::value< vector<string> >()->multitoken(),
			         "Locates the task in the servers list of zombies, and sets to fob.\n"
                  "This default behaviour of the child commands(label,event,meter) is to fob\n"
			         "Next time the child commands (init,event,meter,label,abort,complete,wait,queue) communicate\n"
			         "with the server, they will complete successfully (but without updating the node tree)\n"
			         "allowing the job to finish.\n"
			         "The references to the zombie in the server is automatically deleted after 1 hour\n"
			         "  args = list of task paths, at least one expected\n"
			         "  --zombie_fob=/path/to/task1 /path/to/task2"
			);
			break;
		}
		case User::FAIL:    {
			desc.add_options()( CtsApi::zombieFailArg(), po::value< vector<string> >()->multitoken(),
			         "Locates the task in the servers list of zombies, and sets to fail.\n"
			         "Next time a child command(init,event,meter,label,abort,complete) which"
			         "matches zombie, communicates with the server, will be set to fail.\n"
			         "Depending on the job setup this may force a abort, the abort will also fail.\n"
			         "Hence job structure should use 'set -e' in the error trapping functions to prevent\n"
			         "infinite recursion.\n"
                  "The references to the zombie in the server is automatically deleted after 1 hour\n"
			         "  args = list of task paths, at least one expected\n"
                  "  --zombie_fail=/path/to/task  /path/to/task2"
			);
			break;
		}
		case User::ADOPT: {
			desc.add_options()( CtsApi::zombieAdoptArg(), po::value< vector<string> >()->multitoken(),
			         "Locates the task in the servers list of zombies, and sets to adopt.\n"
			         "Next time a child command(init,event,meter,label,abort,complete,wait queue)\n"
			         "communicates with the server, the password on the zombie is adopted by the task.\n"
			         "This is only allowed if the process id matches, otherwise an error is issued.\n"
			         "The zombie reference stored in the server is then deleted.\n"
			         "  args = list of task paths, at least one expected\n"
                  "  --zombie_adopt=/path/to/task  /path/to/task2"
			);
			break;
		}
		case User::REMOVE:  {
			desc.add_options()( CtsApi::zombieRemoveArg(), po::value< vector<string> >()->multitoken(),
			         "Locates the task in the servers list of zombies, and removes it.\n"
			         "Since a job typically has many child commands(i.e init, complete, event, meter, label, wait, queue)\n"
			         "the zombie may reappear\n"
			         "  args = list of task paths, at least one expected\n"
                  "  --zombie_remove=/path/to/task  /path/to/task2"
			);
			break;
		}
		case User::BLOCK: {
			desc.add_options()( CtsApi::zombieBlockArg(), po::value< vector<string> >()->multitoken(),
			         "Locates the task in the servers list of zombies, and sets flags to block it.\n"
			         "This is default behaviour of the child commands(init,abort,complete,wait,queue)\n"
			         "when the server can not match the passwords. Each child commands will continue\n"
			         "attempting to connect to the server for 24 hours, and will then return an error.\n"
			         "The connection timeout can be configured with environment ECF_TIMEOUT\n"
			         "  args = list of task paths, at least one expected\n"
                  "  --zombie_block=/path/to/task  /path/to/task2"
			);
			break;
		}
      case User::KILL: {
         desc.add_options()( CtsApi::zombieKillArg(), po::value< vector<string> >()->multitoken(),
                  "Locates the task in the servers list of zombies, and sets flags to kill\n"
                  "The kill is done using ECF_KILL_CMD, but using the process_id from the zombie\n"
                  "The job is allowed to continue until the kill is received\n"
                  "Can only kill zombies that have an associated Task, hence path zombies\n"
                  "must be killed manually.\n"
                  "  arg = list of task paths, at least one expected\n"
                  "  --zombie_kill=/path/to/task  /path/to/task2"
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
	// arg = process_or_remote_id   ( empty for CLI and when multiple paths specified)
	// arg = password               ( empty for CLI and when multiple paths specified)
	// arg = paths
	std::string process_or_remote_id;
	std::string password;

   std::vector<std::string> options,paths;
   split_args_to_options_and_paths(args,options,paths); // relative order is still preserved
   if (paths.empty()) {
      std::stringstream ss;
      ss << "ZombieCmd: No paths specified. At least one path expected. Paths must begin with a leading '/' character\n";
      throw std::runtime_error( ss.str() );
   }
   if ( paths.size() > 1 && !options.empty()) {
      std::stringstream ss;
      ss << "ZombieCmd: process_or_remote_id and password can not be used when multiple paths are specified. Please specify a single path\n";
      throw std::runtime_error( ss.str() );
   }
   if (options.size() >= 3) {
      std::stringstream ss;
      ss << "ZombieCmd: to many options expected only process_or_remote_id and password and a list of paths.\n";
      throw std::runtime_error( ss.str() );
   }
   if (options.size() == 1) process_or_remote_id = options[0];
   if (options.size() == 2) password = options[1];

   if (ace->get_cli()) {
      // We are using command line, expect only paths
      if (!process_or_remote_id.empty()) {
         throw std::runtime_error("ZombieCmd:: With the command line interface, we expected only paths i.e /path/to/task");
      }
      if (!password.empty()) {
         throw std::runtime_error("ZombieCmd:: With the command line interface, we expected only paths i.e /path/to/task");
      }
   }

	cmd = Cmd_ptr(new ZombieCmd(user_action_, paths, process_or_remote_id, password ));
}

std::ostream& operator<<(std::ostream& os, const ZombieCmd& c) { return c.print(os); }
