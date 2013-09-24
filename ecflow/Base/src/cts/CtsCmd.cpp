/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #81 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/lexical_cast.hpp>

#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "AbstractClientEnv.hpp"
#include "CtsApi.hpp"
#include "Jobs.hpp"
#include "JobsParam.hpp"
#include "Defs.hpp"
#include "Log.hpp"
#include "Ecf.hpp"
#include "Gnuplot.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

std::ostream& CtsCmd::print(std::ostream& os) const
{
   switch (api_) {
      case CtsCmd::GET_ZOMBIES:                return user_cmd(os,CtsApi::zombieGet()); break;
      case CtsCmd::RESTORE_DEFS_FROM_CHECKPT:  return user_cmd(os,CtsApi::restoreDefsFromCheckPt()); break;
      case CtsCmd::RESTART_SERVER:             return user_cmd(os,CtsApi::restartServer()); break;
      case CtsCmd::SHUTDOWN_SERVER:            return user_cmd(os,CtsApi::shutdownServer()); break;
      case CtsCmd::HALT_SERVER:                return user_cmd(os,CtsApi::haltServer()); break;
      case CtsCmd::TERMINATE_SERVER:           return user_cmd(os,CtsApi::terminateServer()); break;
      case CtsCmd::RELOAD_WHITE_LIST_FILE:     return user_cmd(os,CtsApi::reloadwsfile()); break;
      case CtsCmd::FORCE_DEP_EVAL:             return user_cmd(os,CtsApi::forceDependencyEval()); break;
      case CtsCmd::PING:                       return user_cmd(os,CtsApi::pingServer()); break;
      case CtsCmd::STATS:                      return user_cmd(os,CtsApi::stats()); break;
      case CtsCmd::SUITES:                     return user_cmd(os,CtsApi::suites()); break;
      case CtsCmd::DEBUG_SERVER_ON:            return user_cmd(os,CtsApi::debug_server_on()); break;
      case CtsCmd::DEBUG_SERVER_OFF:           return user_cmd(os,CtsApi::debug_server_off()); break;
      case CtsCmd::SERVER_LOAD:                return user_cmd(os,CtsApi::server_load("")); break;
      case CtsCmd::NO_CMD:                     assert(false); break;
      default: assert(false); break;
   }
   return os;
}

bool CtsCmd::equals(ClientToServerCmd* rhs) const
{
   CtsCmd* the_rhs = dynamic_cast< CtsCmd* > ( rhs );
   if ( !the_rhs ) return false;
   if (api_ != the_rhs->api()) return false;
   return UserCmd::equals(rhs);
}

bool CtsCmd::isWrite() const
{
   switch (api_) {
      case CtsCmd::GET_ZOMBIES:      return false; break; // read only
      case CtsCmd::RESTORE_DEFS_FROM_CHECKPT: return true; break;  // requires write privilege
      case CtsCmd::RESTART_SERVER:   return true; break;  // requires write privilege
      case CtsCmd::SHUTDOWN_SERVER:  return true; break;  // requires write privilege
      case CtsCmd::HALT_SERVER:      return true; break;  // requires write privilege
      case CtsCmd::TERMINATE_SERVER: return true; break;  // requires write privilege
      case CtsCmd::RELOAD_WHITE_LIST_FILE:return true; break;  // requires write privilege
      case CtsCmd::FORCE_DEP_EVAL:   return true; break;       // requires write privilege
      case CtsCmd::PING:             return false; break;      // read only
      case CtsCmd::STATS:            return false; break;      // read only
      case CtsCmd::SUITES:           return false; break;      // read only
      case CtsCmd::DEBUG_SERVER_ON:  return false; break;      // read only
      case CtsCmd::DEBUG_SERVER_OFF: return false; break;      // read only
      case CtsCmd::SERVER_LOAD:      return false; break;      // read only
      case CtsCmd::NO_CMD:           assert(false); break;
      default: assert(false); break;
   }
   assert(false);
   return false;
}

int CtsCmd::timeout() const
{
   if (api_ == CtsCmd::PING) return 10;
   return ClientToServerCmd::timeout();
}

const char* CtsCmd::theArg() const
{
   switch (api_) {
      case CtsCmd::GET_ZOMBIES:      return CtsApi::zombieGetArg(); break;
      case CtsCmd::RESTORE_DEFS_FROM_CHECKPT:  return CtsApi::restoreDefsFromCheckPtArg(); break;
      case CtsCmd::RESTART_SERVER:   return CtsApi::restartServerArg(); break;
      case CtsCmd::SHUTDOWN_SERVER:  return CtsApi::shutdownServerArg(); break;
      case CtsCmd::HALT_SERVER:      return CtsApi::haltServerArg(); break;
      case CtsCmd::TERMINATE_SERVER: return CtsApi::terminateServerArg(); break;
      case CtsCmd::RELOAD_WHITE_LIST_FILE:return CtsApi::reloadwsfileArg(); break;
      case CtsCmd::FORCE_DEP_EVAL:   return CtsApi::forceDependencyEvalArg(); break;
      case CtsCmd::PING:             return CtsApi::pingServerArg(); break;
      case CtsCmd::STATS:            return CtsApi::statsArg(); break;
      case CtsCmd::SUITES:           return CtsApi::suitesArg(); break;
      case CtsCmd::DEBUG_SERVER_ON:  return CtsApi::debug_server_on_arg(); break;
      case CtsCmd::DEBUG_SERVER_OFF: return CtsApi::debug_server_off_arg(); break;
      case CtsCmd::SERVER_LOAD:      return CtsApi::server_load_arg(); break;
      case CtsCmd::NO_CMD:           assert(false); break;
      default: assert(false); break;
   }
   assert(false);
   return NULL;
}

STC_Cmd_ptr CtsCmd::doHandleRequest(AbstractServer* as) const
{
   switch (api_) {
      case CtsCmd::GET_ZOMBIES: {
         as->update_stats().zombie_get_++;
         return PreAllocatedReply::zombie_get_cmd( as );break;
      }

      case CtsCmd::RESTORE_DEFS_FROM_CHECKPT: {
         as->update_stats().restore_defs_from_checkpt_++;
         as->restore_defs_from_checkpt(); // this can throw, i.e. if server not halted, or defs has suites, etc
         break;
      }

      case CtsCmd::RESTART_SERVER: {
         as->update_stats().restart_server_++;
         as->restart();
         return doJobSubmission( as );
         break;
      }
      case CtsCmd::SHUTDOWN_SERVER:  as->update_stats().shutdown_server_++; as->shutdown();  break;
      case CtsCmd::HALT_SERVER:      as->update_stats().halt_server_++;     as->halted();   break;
      case CtsCmd::TERMINATE_SERVER: as->checkPtDefs();  break;
      case CtsCmd::RELOAD_WHITE_LIST_FILE: {
         as->update_stats().reload_white_list_file_++;
         std::string errorMsg;
         if (!as->reloadWhiteListFile(errorMsg)) {
            throw std::runtime_error( errorMsg ) ;
         }
         break;
      }
      case CtsCmd::FORCE_DEP_EVAL: {
         if (!as->defs()) throw std::runtime_error( "No definition in server") ;

         // The Default JobsParam does *not* allow Job creation, & hence => does not submit jobs
         // The default does *not* allow job spawning
         Jobs jobs(as->defs());
         JobsParam jobsParam;  // create jobs =  false, spawn_jobs = false
         if (!jobs.generate(jobsParam)) throw std::runtime_error( jobsParam.getErrorMsg() ) ;
         break;
      }
      case CtsCmd::PING:   as->update_stats().ping_++; break;
      case CtsCmd::STATS:  as->update_stats().stats_++;  return PreAllocatedReply::stats_cmd( as ); break;
      case CtsCmd::SUITES: as->update_stats().suites_++; return PreAllocatedReply::suites_cmd( as ); break;
      case CtsCmd::DEBUG_SERVER_ON:  as->update_stats().debug_server_on_++;  as->debug_server_on(); break;
      case CtsCmd::DEBUG_SERVER_OFF: as->update_stats().debug_server_off_++; as->debug_server_off(); break;
      case CtsCmd::SERVER_LOAD: {    as->update_stats().server_load_cmd_++;
         if (Log::instance()) {
            return PreAllocatedReply::server_load_cmd(  Log::instance()->path() );
         }
         break;
      }
      case CtsCmd::NO_CMD: assert(false); break;
      default: assert(false); break;
   }

   return PreAllocatedReply::ok_cmd();
}

static const char* server_load_desc() {
   /////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
   return
            "Shows the server load graphically, by parsing the log file.\n"
            "If no log file is provided, then the log file path is obtained from\n"
            "the server. *If* this file is accessible from the client, then a\n"
            "graphical view of the server load is shown.\n"
            "This command assumes that gnuplot is available on $PATH. This command\n"
            "produces two files in the CWD 'gnuplot.dat' and 'gnuplot.script'.\n"
            "This generated script can be manually changed, to see different rendering\n"
            "effects. i.e. just run 'gnuplot gnuplot.script'\n\n"
            "  arg1 = <optional> path to log file\n"
            "If the path to log file is known, it is *preferable* to use this,\n"
            "rather than requesting the log path from the server.\n"
            "Usage:\n"
            "   --server_load=/path/to_log_file  # parses log and shows a gnuplot window\n"
            "   --server_load                    # log file path is requested from server\n"
            "                                    # which is then used to produce gnuplot window\n"
            "                                    # *AVOID* if log file path is accessible"
            ;
}

void CtsCmd::addOption(boost::program_options::options_description& desc) const
{
   switch (api_) {
      case CtsCmd::GET_ZOMBIES:{
         desc.add_options()(CtsApi::zombieGetArg(),
                  "Returns the list of zombies from the server.\n"
                  "Results reported to standard output."
         );
         break;
      }
      case CtsCmd::RESTORE_DEFS_FROM_CHECKPT:{
         desc.add_options()(CtsApi::restoreDefsFromCheckPtArg(),
                  "Ask the server to load the definition from an check pt file.\n"
                  "The server must be halted and the definition in the server must be deleted\n"
                  "first, otherwise an error is returned"
         );
         break;
      }
      case CtsCmd::RESTART_SERVER:{
         desc.add_options()( CtsApi::restartServerArg(),
                  "Start job scheduling, communication with jobs, and respond to all requests.\n"
                  "The following table shows server behaviour in the different states.\n"
                  "|----------------------------------------------------------------------------------|\n"
                  "| Server State | User Request | Task Request |Job Scheduling | Auto-Check-pointing |\n"
                  "|--------------|--------------|--------------|---------------|---------------------|\n"
                  "|     RUNNING  |    yes       |      yes     |      yes      |      yes            |\n"
                  "|    SHUTDOWN  |    yes       |      yes     |      no       |      yes            |\n"
                  "|      HALTED  |    yes       |      no      |      no       |      no             |\n"
                  "|--------------|--------------|--------------|---------------|---------------------|"
         );
         break;
      }
      case CtsCmd::SHUTDOWN_SERVER: {
         desc.add_options()( CtsApi::shutdownServerArg(),po::value< string >()->implicit_value( string("") ),
                  "Stop server from scheduling new jobs.\n"
                  "  arg1 = yes(optional) # use to bypass confirmation prompt\n"
                  "The following table shows server behaviour in the different states.\n"
                  "|----------------------------------------------------------------------------------|\n"
                  "| Server State | User Request | Task Request |Job Scheduling | Auto-Check-pointing |\n"
                  "|--------------|--------------|--------------|---------------|---------------------|\n"
                  "|     RUNNING  |    yes       |      yes     |      yes      |      yes            |\n"
                  "|    SHUTDOWN  |    yes       |      yes     |      no       |      yes            |\n"
                  "|      HALTED  |    yes       |      no      |      no       |      no             |\n"
                  "|--------------|--------------|--------------|---------------|---------------------|"
         );
         break;
      }
      case CtsCmd::HALT_SERVER: {
         desc.add_options()( CtsApi::haltServerArg(),po::value< string >()->implicit_value( string("") ),
                  "Stop server communication with jobs, and new job scheduling.\n"
                  "Also stops automatic check pointing\n"
                  "  arg1 = yes(optional) # use to bypass confirmation prompt\n"
                  "The following table shows server behaviour in the different states.\n"
                  "|----------------------------------------------------------------------------------|\n"
                  "| Server State | User Request | Task Request |Job Scheduling | Auto-Check-pointing |\n"
                  "|--------------|--------------|--------------|---------------|---------------------|\n"
                  "|     RUNNING  |    yes       |      yes     |      yes      |      yes            |\n"
                  "|    SHUTDOWN  |    yes       |      yes     |      no       |      yes            |\n"
                  "|      HALTED  |    yes       |      no      |      no       |      no             |\n"
                  "|--------------|--------------|--------------|---------------|---------------------|"
         );
         break;
      }
      case CtsCmd::TERMINATE_SERVER:{
         desc.add_options()( CtsApi::terminateServerArg(),po::value< string >()->implicit_value( string("") ),
                  "Terminate the server.\n"
                  "  arg1 = yes(optional) # use to bypass confirmation prompt"
         );
         break;
      }
      case CtsCmd::RELOAD_WHITE_LIST_FILE:{
         desc.add_options()( CtsApi::reloadwsfileArg(),
                  "Reload the white list file.\n"
                  "The white list file is used to authenticate 'user' commands.\n"
                  "Raises an error if file does not exist, or fails to parse\n"
                  "Expected format for this file is:\n\n"
                  "# comment\n"
                  "4.4.14  # version number\n\n"
                  "# Users with read/write access\n"
                  "user1   # comment\n"
                  "user2   # comment\n\n"
                  "# Users with read  access, must have - before user name\n"
                  "-user3  # comment\n"
                  "-user4"
         );
         break;
      }
      case CtsCmd::FORCE_DEP_EVAL:{
         desc.add_options()( CtsApi::forceDependencyEvalArg(),
                  "Force dependency evaluation. Used for DEBUG only."
         );
         break;
      }
      case CtsCmd::PING:{
         desc.add_options()( CtsApi::pingServerArg(),
                  "Check if server is running on given host/port. Result reported to standard output.\n"
                  "Usage:\n"
                  "  --ping --host=mach --port=3144 # Check if server alive on host mach & port 3144\n"
                  "  --ping --host=fred              # Check if server alive on host fred and port ECF_PORT,\n"
                  "                                  # otherwise default port of 3141\n"
                  "  --ping                          # Check if server alive by using environment variables\n"
                  "                                  # ECF_NODE and ECF_PORT\n"
                  "If ECF_NODE not defined uses 'localhost', if ECF_PORT not defined assumes 3141"
         );
         break;
      }
      case CtsCmd::STATS:{
         desc.add_options()( CtsApi::statsArg(),
                  "Returns the server statistics."
         );
         break;
      }
      case CtsCmd::SUITES:{
         desc.add_options()( CtsApi::suitesArg(),
                  "Returns the list of suites, in the order defined in the server."
         );
         break;
      }
      case CtsCmd::DEBUG_SERVER_ON:   {
         desc.add_options()( CtsApi::debug_server_on_arg(),
                  "Enables debug output from the server"
         );
         break;
      }
      case CtsCmd::DEBUG_SERVER_OFF:  {
         desc.add_options()( CtsApi::debug_server_off_arg(),
                  "Disables debug output from the server"
         );
         break;
      }
      case CtsCmd::SERVER_LOAD:  {
         desc.add_options()( CtsApi::server_load_arg(),po::value< std::string >()->implicit_value( string("") ), server_load_desc() );
         break;
      }
      case CtsCmd::NO_CMD: assert(false); break;
      default: assert(false); break;
   }
}

bool CtsCmd::handleRequestIsTestable() const {
   if (api_ == CtsCmd::TERMINATE_SERVER) return false;
   if (api_ == CtsCmd::RESTORE_DEFS_FROM_CHECKPT) return false;
   return true;
}

void CtsCmd::create( 	Cmd_ptr& cmd,
         boost::program_options::variables_map& vm,
         AbstractClientEnv*  ac ) const
{
   if (ac->debug()) cout << "CtsCmd::create api = '" << api_ << "'.\n";

   assert( api_ != CtsCmd::NO_CMD);

   if (api_ == CtsCmd::HALT_SERVER || api_ == CtsCmd::SHUTDOWN_SERVER || api_ == CtsCmd::TERMINATE_SERVER) {

      std::string do_prompt = vm[ theArg() ].as< std::string > ();
      if (do_prompt.empty()) {
         if (api_ == CtsCmd::HALT_SERVER)          prompt_for_confirmation("Are you sure you want to halt the server ? ");
         else if (api_ == CtsCmd::SHUTDOWN_SERVER) prompt_for_confirmation("Are you sure you want to shut down the server ? ");
         else                                      prompt_for_confirmation("Are you sure you want to terminate the server ? ");
      }
      else if ( do_prompt != "yes" )
         throw std::runtime_error("Halt, shutdown and terminate expected 'yes' as the only argument to bypass the confirmation prompt");
   }
   else if ( api_ == CtsCmd::SERVER_LOAD) {

      std::string log_file = vm[ theArg() ].as< std::string > ();
      if (ac->debug()) std::cout << "   CtsCmd::create CtsCmd::SERVER_LOAD " << log_file << "\n";

      if (!log_file.empty()) {

         // testing client interface
         if (ac->under_test())  return;

         // No need to call server. Parse the log file to create gnu_plot file.
         Gnuplot::show_server_load(log_file);
         return; // Don't create command, since with log file, it is client specific only
      }
   }
   cmd = Cmd_ptr(new CtsCmd( api_ ));
}

std::ostream& operator<<(std::ostream& os, const CtsCmd& c) { return c.print(os); }
