//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #76 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : (C)lient (t)o (s)erver API
//============================================================================

#include <sstream>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>

#include "CtsApi.hpp"

std::string CtsApi::to_string(const std::vector<std::string>& vec) {
   std::string ret;
   size_t vec_size = vec.size();
   for(size_t i = 0; i < vec_size; i++) {
      if (vec[i].empty()) continue;
      ret += vec[i];
      if (i != vec_size-1) ret += " ";  // don't add space at the end
   }
   return ret;
}

std::string CtsApi::server_version() { return std::string("--server_version");}
const char* CtsApi::server_version_arg() { return "server_version"; }

std::string CtsApi::get(const std::string& absNodePath) {
   std::string ret = "--get";
   if (!absNodePath.empty()) {
      ret += "=";
      ret += absNodePath;
   }
   return ret;
}
const char* CtsApi::getArg() { return "get"; }

std::string CtsApi::get_state(const std::string& absNodePath) {
   std::string ret = "--get_state";
   if (!absNodePath.empty()) {
      ret += "=";
      ret += absNodePath;
   }
   return ret;
}
const char* CtsApi::get_state_arg() { return "get_state"; }

std::string CtsApi::migrate(const std::string& absNodePath) {
   std::string ret = "--migrate";
   if (!absNodePath.empty()) {
      ret += "=";
      ret += absNodePath;
   }
   return ret;
}
const char* CtsApi::migrate_arg() { return "migrate"; }

std::string CtsApi::stats()        { return "--stats"; }
const char* CtsApi::statsArg()     { return "stats"; }
std::string CtsApi::stats_server() { return "--stats_server"; }
const char* CtsApi::stats_server_arg() { return "stats_server"; }
std::string CtsApi::stats_reset()     { return "--stats_reset"; }
const char* CtsApi::stats_reset_arg()  { return "stats_reset"; }


std::string CtsApi::suites()    { return "--suites"; }
const char* CtsApi::suitesArg() { return "suites"; }

std::vector<std::string> CtsApi::ch_register( int client_handle, bool auto_add_new_suites , const std::vector<std::string>& suites )
{
   std::vector<std::string> retVec; retVec.reserve(suites.size() +1);
   std::string ret = "--ch_register=";
   if (client_handle != 0) {
      ret += boost::lexical_cast<std::string>(client_handle);
      retVec.push_back(ret);
      if ( auto_add_new_suites ) retVec.push_back("true");
      else                       retVec.push_back("false");
   }
   else {
      if ( auto_add_new_suites ) ret += "true";
      else                       ret += "false";
      retVec.push_back(ret);
   }
   for(const auto & suite : suites) { retVec.push_back(suite);}
   return retVec;
}
const char* CtsApi::ch_register_arg() { return "ch_register"; }

std::string CtsApi::ch_suites() { return "--ch_suites";}
const char* CtsApi::ch_suites_arg() { return "ch_suites"; }

std::string CtsApi::ch_drop(int client_handle)
{
   std::string ret = "--ch_drop=";
   ret += boost::lexical_cast<std::string>(client_handle);
   return ret;
}
const char* CtsApi::ch_drop_arg() { return "ch_drop"; }

std::string CtsApi::ch_drop_user(const std::string& user)
{
   std::string ret = "--ch_drop_user";
   if (!user.empty()) {
      ret += "=";
      ret += user;
   }
   return ret;
}
const char* CtsApi::ch_drop_user_arg() { return "ch_drop_user"; }

std::vector<std::string> CtsApi::ch_add( int client_handle, const std::vector<std::string>& suites )
{
   std::vector<std::string> retVec; retVec.reserve(suites.size() +1);
   std::string ret = "--ch_add=";
   ret += boost::lexical_cast<std::string>(client_handle);
   retVec.push_back(ret);
   for(const auto & suite : suites) { retVec.push_back(suite);}
   return retVec;
}
const char* CtsApi::ch_add_arg() { return "ch_add"; }

std::vector<std::string> CtsApi::ch_remove( int client_handle, const std::vector<std::string>& suites )
{
   std::vector<std::string> retVec; retVec.reserve(suites.size() +1);
   std::string ret = "--ch_rem=";
   ret += boost::lexical_cast<std::string>(client_handle);
   retVec.push_back(ret);
   for(const auto & suite : suites) { retVec.push_back(suite); }
   return retVec;
}
const char* CtsApi::ch_remove_arg() { return "ch_rem"; }

std::vector<std::string> CtsApi::ch_auto_add( int client_handle, bool auto_add_new_suites )
{
   std::vector<std::string> retVec; retVec.reserve(2);
   std::string ret = "--ch_auto_add=";
   ret += boost::lexical_cast<std::string>(client_handle);
   retVec.push_back(ret);
   if ( auto_add_new_suites ) retVec.emplace_back("true");
   else                       retVec.emplace_back("false");
   return retVec;
}
const char* CtsApi::ch_auto_add_arg() { return "ch_auto_add"; }


std::vector<std::string> CtsApi::sync(unsigned int client_handle,unsigned int state_change_no, unsigned int modify_change_no )
{
   std::vector<std::string> retVec; retVec.reserve(3);
   std::string ret = "--sync=";
   ret += boost::lexical_cast<std::string>( client_handle );
   retVec.push_back(ret);
   retVec.push_back(boost::lexical_cast<std::string>( state_change_no ));
   retVec.push_back(boost::lexical_cast<std::string>( modify_change_no ));
   return retVec;
}
const char* CtsApi::syncArg()  { return "sync"; }

std::vector<std::string> CtsApi::sync_clock(unsigned int client_handle,unsigned int state_change_no, unsigned int modify_change_no )
{
   std::vector<std::string> retVec; retVec.reserve(3);
   std::string ret = "--sync_clock=";
   ret += boost::lexical_cast<std::string>( client_handle );
   retVec.push_back(ret);
   retVec.push_back(boost::lexical_cast<std::string>( state_change_no ));
   retVec.push_back(boost::lexical_cast<std::string>( modify_change_no ));
   return retVec;
}
const char* CtsApi::sync_clock_arg()  { return "sync_clock"; }

std::string CtsApi::sync_full(unsigned int client_handle) {
   std::string ret = "--sync_full=";
   ret += boost::lexical_cast<std::string>( client_handle );
   return ret;
}
const char* CtsApi::sync_full_arg() { return "sync_full";}

std::vector<std::string> CtsApi::news(unsigned int client_handle, unsigned int state_change_no, unsigned int modify_change_no )
{
   std::vector<std::string> retVec; retVec.reserve(3);
   std::string ret = "--news=";
   ret += boost::lexical_cast<std::string>( client_handle );
   retVec.push_back(ret);
   retVec.push_back(boost::lexical_cast<std::string>( state_change_no ));
   retVec.push_back(boost::lexical_cast<std::string>( modify_change_no ));
   return retVec;
}
const char* CtsApi::newsArg()  { return "news"; }

std::vector<std::string> CtsApi::loadDefs(const std::string& filePath,bool force,bool check_only,bool print){

   std::string ret = "--load="; ret += filePath;

   std::vector<std::string> retVec; retVec.reserve(3);
   retVec.push_back(ret);
   if (force) retVec.emplace_back("force");
   if (check_only) retVec.emplace_back("check_only");
   if (print) retVec.emplace_back("print");
   return retVec;
}
const char* CtsApi::loadDefsArg() { return "load"; }

std::string CtsApi::restartServer()       { return "--restart";}
const char* CtsApi::restartServerArg()    { return "restart";}

std::string CtsApi::haltServer(bool auto_confirm) { return (auto_confirm) ? "--halt=yes" : "--halt"; }
const char* CtsApi::haltServerArg()       { return "halt"; }

std::string CtsApi::shutdownServer(bool auto_confirm) { return (auto_confirm) ? "--shutdown=yes" : "--shutdown"; }
const char* CtsApi::shutdownServerArg()   { return "shutdown"; }

std::string CtsApi::terminateServer(bool auto_confirm) { return (auto_confirm) ? "--terminate=yes" : "--terminate";}
const char* CtsApi::terminateServerArg() { return "terminate";}

std::string CtsApi::pingServer()          { return "--ping"; }
const char* CtsApi::pingServerArg()       { return "ping"; }

std::string CtsApi::server_load(const std::string& path_to_log_file)
{
   std::string ret = "--server_load";
   if (!path_to_log_file.empty()) {
      ret += "=";
      ret += path_to_log_file;
   }
   return ret;
}
const char* CtsApi::server_load_arg()       { return "server_load"; }


std::string CtsApi::debug_server_on()      { return "--debug_server_on"; }
const char* CtsApi::debug_server_on_arg()  { return "debug_server_on"; }
std::string CtsApi::debug_server_off()     { return "--debug_server_off"; }
const char* CtsApi::debug_server_off_arg() { return "debug_server_off"; }


std::string CtsApi::begin(const std::string& suiteName, bool force) {

   // *both* are optional
   std::string ret = "--begin";
   if (suiteName.empty() && !force) {
      return ret;
   }
   if (!suiteName.empty()) {
      ret += "=";
      ret += suiteName;
   }
   if (force) {
      if (suiteName.empty()) {
         ret += "=--force";
      }
      else {
         ret += " --force"; // note the space separator
      }
   }
   return ret;
}
const char* CtsApi::beginArg() { return "begin"; }


std::string CtsApi::checkJobGenOnly(const std::string& absNodePath)
{
   std::string ret = "--checkJobGenOnly";
   if (!absNodePath.empty()) {
      ret += "=";
      ret += absNodePath;
   }
   return ret;
}
const char* CtsApi::checkJobGenOnlyArg() { return "checkJobGenOnly";}


std::string CtsApi::job_gen(const std::string& absNodePath)
{
   std::string ret = "--job_gen";
   if (!absNodePath.empty()) {
      ret += "=";
      ret += absNodePath;
   }
   return ret;
}
const char* CtsApi::job_genArg() { return "job_gen";}


std::vector<std::string> CtsApi::check(const std::vector<std::string>& paths)
{
   std::vector<std::string> retVec; retVec.reserve(2 + paths.size());
   retVec.emplace_back("--check");
   if (paths.empty()) retVec.emplace_back("_all_");
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
std::vector<std::string> CtsApi::check(const std::string& path)
{
   if (path.empty()) {
      return CtsApi::check(std::vector<std::string>());
   }
   return CtsApi::check(std::vector<std::string>(1,path));
}
const char* CtsApi::check_arg() { return "check";}

std::vector<std::string> CtsApi::delete_node(const std::vector<std::string>& paths, bool force, bool auto_confirm )
{
   std::vector<std::string> retVec; retVec.reserve(4 + paths.size());
   retVec.emplace_back("--delete");
   if (paths.empty()) {
      retVec.emplace_back("_all_");
   }
   if (force)        {
      retVec.emplace_back("force");
   }
   if (auto_confirm) {          // By default delete prompts
      retVec.emplace_back("yes");  // yes means we don't prompt, and have automatically confirmed the delete
   }
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
std::vector<std::string> CtsApi::delete_node(const std::string& absNodePath, bool force, bool auto_confirm )
{
   if (absNodePath.empty()) return CtsApi::delete_node(std::vector<std::string>(),force,auto_confirm);
   return CtsApi::delete_node(std::vector<std::string>(1,absNodePath),force,auto_confirm);
}
const char* CtsApi::delete_node_arg() { return "delete";}


std::vector<std::string> CtsApi::suspend(const std::vector<std::string>& paths)
{
   std::vector<std::string> retVec; retVec.reserve(1 + paths.size());
   retVec.emplace_back("--suspend");
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
std::vector<std::string> CtsApi::suspend(const std::string& path)
{
   return CtsApi::suspend(std::vector<std::string>(1,path));
}
const char* CtsApi::suspend_arg() { return "suspend";}


std::vector<std::string> CtsApi::resume(const std::vector<std::string>& paths)
{
   std::vector<std::string> retVec; retVec.reserve(1 + paths.size());
   retVec.emplace_back("--resume");
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
std::vector<std::string> CtsApi::resume(const std::string& path)
{
   return CtsApi::resume(std::vector<std::string>(1,path));
}
const char* CtsApi::resume_arg() { return "resume";}


std::vector<std::string> CtsApi::kill(const std::vector<std::string>& paths)
{
   std::vector<std::string> retVec; retVec.reserve(1 + paths.size());
   retVec.emplace_back("--kill");
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
std::vector<std::string> CtsApi::kill(const std::string& path)
{
   return CtsApi::kill(std::vector<std::string>(1,path));
}
const char* CtsApi::kill_arg() { return "kill";}


std::vector<std::string> CtsApi::status(const std::vector<std::string>& paths)
{
   std::vector<std::string> retVec; retVec.reserve(1 + paths.size());
   retVec.emplace_back("--status");
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
std::vector<std::string> CtsApi::status(const std::string& path)
{
   return CtsApi::status(std::vector<std::string>(1,path));
}
const char* CtsApi::statusArg() { return "status";}


std::vector<std::string> CtsApi::edit_history(const std::vector<std::string>& paths)
{
   std::vector<std::string> retVec; retVec.reserve(1 + paths.size());
   retVec.emplace_back("--edit_history");
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
std::vector<std::string> CtsApi::edit_history(const std::string& path)
{
   return CtsApi::edit_history(std::vector<std::string>(1,path));
}
const char* CtsApi::edit_history_arg() { return "edit_history";}

std::vector<std::string> CtsApi::archive(const std::vector<std::string>& paths, bool force){
   std::vector<std::string> retVec; retVec.reserve(2 + paths.size());
   retVec.emplace_back("--archive");
   if (force) retVec.emplace_back("force");
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
std::vector<std::string> CtsApi::archive(const std::string& path,bool force){
   return CtsApi::archive(std::vector<std::string>(1,path),force);
}
const char* CtsApi::archive_arg() { return "archive";}

std::vector<std::string> CtsApi::restore(const std::vector<std::string>& paths){
   std::vector<std::string> retVec; retVec.reserve(1 + paths.size());
   retVec.emplace_back("--restore");
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
std::vector<std::string> CtsApi::restore(const std::string& path){
   return CtsApi::restore(std::vector<std::string>(1,path));
}
const char* CtsApi::restore_arg() { return "restore";}


std::string CtsApi::why(const std::string& absNodePath)
{
   if ( absNodePath.empty()) return "--why";
   std::string ret = "--why=";
   ret += absNodePath;
   return ret;
}
const char* CtsApi::whyArg() { return "why";}

std::string CtsApi::zombieGet()    { return "--zombie_get"; }
const char* CtsApi::zombieGetArg() { return "zombie_get"; }

std::vector<std::string> CtsApi::zombieFob(const std::vector<std::string>& paths,const std::string& process_id, const std::string& password)
{
   std::string ret = "--zombie_fob=";
   std::vector<std::string> retVec; retVec.reserve(paths.size()+3);
   if (!paths.empty()) ret += paths[0];
   retVec.push_back(ret);
   for(size_t i=1; i < paths.size(); i++)  retVec.push_back(paths[i]);
   if (!process_id.empty()) retVec.push_back(process_id); // Note: order is important, even if empty
   if (!password.empty()) retVec.push_back(password);     // Note: order is important, even if empty
   return retVec;
}
std::string CtsApi::zombieFobCli(const std::string& path) { std::string ret = "--zombie_fob="; ret += path; return ret;}
std::vector<std::string> CtsApi::zombieFobCli(const std::vector<std::string>& paths) {
   std::string ret = "--zombie_fob";
   std::vector<std::string> retVec; retVec.reserve(paths.size()+ 1);
   retVec.push_back(ret);
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
const char* CtsApi::zombieFobArg() { return "zombie_fob";}

std::vector<std::string> CtsApi::zombieFail(const std::vector<std::string>& paths,const std::string& process_id, const std::string& password)
{
   std::string ret = "--zombie_fail=";
   std::vector<std::string> retVec; retVec.reserve(paths.size()+3);
   if (!paths.empty()) ret += paths[0];
   retVec.push_back(ret);
   for(size_t i=1; i < paths.size(); i++)  retVec.push_back(paths[i]);
   if (!process_id.empty()) retVec.push_back(process_id); // Note: order is important, even if empty
   return retVec;
}
std::string CtsApi::zombieFailCli(const std::string& path) { std::string ret = "--zombie_fail="; ret += path; return ret;}
std::vector<std::string> CtsApi::zombieFailCli(const std::vector<std::string>& paths) {
   std::string ret = "--zombie_fail";
   std::vector<std::string> retVec; retVec.reserve(paths.size()+ 1);
   retVec.push_back(ret);
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
const char* CtsApi::zombieFailArg() { return "zombie_fail";}

std::vector<std::string> CtsApi::zombieAdopt(const std::vector<std::string>& paths,const std::string& process_id, const std::string& password)
{
   std::string ret = "--zombie_adopt=";
   std::vector<std::string> retVec; retVec.reserve(paths.size()+3);
   if (!paths.empty()) ret += paths[0];
   retVec.push_back(ret);
   for(size_t i=1; i < paths.size(); i++)  retVec.push_back(paths[i]);
   if (!process_id.empty()) retVec.push_back(process_id); // Note: order is important, even if empty
   if (!password.empty()) retVec.push_back(password);     // Note: order is important, even if empty
   return retVec;
}
std::string CtsApi::zombieAdoptCli(const std::string& path) { std::string ret = "--zombie_adopt="; ret += path; return ret;}
std::vector<std::string> CtsApi::zombieAdoptCli(const std::vector<std::string>& paths) {
   std::string ret = "--zombie_adopt";
   std::vector<std::string> retVec; retVec.reserve(paths.size()+ 1);
   retVec.push_back(ret);
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
const char* CtsApi::zombieAdoptArg() { return "zombie_adopt";}

std::vector<std::string> CtsApi::zombieRemove(const std::vector<std::string>& paths,const std::string& process_id, const std::string& password)
{
   std::string ret = "--zombie_remove=";
   std::vector<std::string> retVec; retVec.reserve(paths.size()+3);
   if (!paths.empty()) ret += paths[0];
   retVec.push_back(ret);
   for(size_t i=1; i < paths.size(); i++)  retVec.push_back(paths[i]);
   if (!process_id.empty()) retVec.push_back(process_id); // Note: order is important, even if empty
   if (!password.empty()) retVec.push_back(password);     // Note: order is important, even if empty
   return retVec;
}
std::string CtsApi::zombieRemoveCli(const std::string& path) { std::string ret = "--zombie_remove="; ret += path; return ret;}
std::vector<std::string> CtsApi::zombieRemoveCli(const std::vector<std::string>& paths) {
   std::string ret = "--zombie_remove";
   std::vector<std::string> retVec; retVec.reserve(paths.size()+ 1);
   retVec.push_back(ret);
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
const char* CtsApi::zombieRemoveArg() { return "zombie_remove";}

std::vector<std::string> CtsApi::zombieBlock(const std::vector<std::string>& paths,const std::string& process_id, const std::string& password)
{
   std::string ret = "--zombie_block=";
   std::vector<std::string> retVec; retVec.reserve(paths.size()+3);
   if (!paths.empty()) ret += paths[0];
   retVec.push_back(ret);
   for(size_t i=1; i < paths.size(); i++)  retVec.push_back(paths[i]);
   if (!process_id.empty()) retVec.push_back(process_id); // Note: order is important, even if empty
   if (!password.empty()) retVec.push_back(password);     // Note: order is important, even if empty
   return retVec;
}
std::string CtsApi::zombieBlockCli(const std::string& path) { std::string ret = "--zombie_block="; ret += path; return ret;}
std::vector<std::string> CtsApi::zombieBlockCli(const std::vector<std::string>& paths) {
   std::string ret = "--zombie_block";
   std::vector<std::string> retVec; retVec.reserve(paths.size()+ 1);
   retVec.push_back(ret);
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
const char* CtsApi::zombieBlockArg() { return "zombie_block";}

std::vector<std::string> CtsApi::zombieKill(const std::vector<std::string>& paths,const std::string& process_id, const std::string& password)
{
   std::string ret = "--zombie_kill=";
   std::vector<std::string> retVec; retVec.reserve(paths.size()+3);
   if (!paths.empty()) ret += paths[0];
   retVec.push_back(ret);
   for(size_t i=1; i < paths.size(); i++)  retVec.push_back(paths[i]);
   if (!process_id.empty()) retVec.push_back(process_id); // Note: order is important, even if empty
   if (!password.empty()) retVec.push_back(password);     // Note: order is important, even if empty
   return retVec;
}
std::string CtsApi::zombieKillCli(const std::string& path) { std::string ret = "--zombie_kill="; ret += path; return ret;}
std::vector<std::string> CtsApi::zombieKillCli(const std::vector<std::string>& paths) {
   std::string ret = "--zombie_kill";
   std::vector<std::string> retVec; retVec.reserve(paths.size()+ 1);
   retVec.push_back(ret);
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
const char* CtsApi::zombieKillArg() { return "zombie_kill";}


std::vector<std::string> CtsApi::requeue(const std::vector<std::string>& paths, const std::string& option)
{
   std::vector<std::string> retVec; retVec.reserve(2 + paths.size());
   retVec.emplace_back("--requeue");
   if (!option.empty()) retVec.push_back(option);
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
std::vector<std::string> CtsApi::requeue(const std::string& absNodePath, const std::string& option)
{
   return CtsApi::requeue(std::vector<std::string>(1,absNodePath),option);
}
const char* CtsApi::requeueArg() { return "requeue"; }

std::vector<std::string> CtsApi::run(const std::vector<std::string>& paths, bool force)
{
   std::vector<std::string> retVec; retVec.reserve(paths.size()+2);
   retVec.emplace_back("--run");
   if (force) retVec.emplace_back("force");
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
std::vector<std::string> CtsApi::run(const std::string& absNodePath, bool force)
{
   return CtsApi::run(std::vector<std::string>(1,absNodePath),force);
}
const char* CtsApi::runArg() { return "run"; }


std::vector<std::string> CtsApi::order(const std::string& absNodePath,const std::string& orderType)
{
   std::vector<std::string> retVec; retVec.reserve(2);
   std::string ret = "--order="; ret += absNodePath;
   retVec.push_back(ret);
   retVec.push_back(orderType);
   return retVec;
}
const char* CtsApi::orderArg() { return "order"; }


std::vector<std::string> CtsApi::replace(  const std::string& absNodePath,
         const std::string& path_to_client_defs,
         bool create_parents_as_required,
         bool force)
{
   std::vector<std::string> retVec; retVec.reserve(3);

   std::string ret = "--replace="; ret += absNodePath;
   retVec.push_back(ret);
   retVec.push_back(path_to_client_defs);
   if (create_parents_as_required) retVec.emplace_back("parent");
   if (force) retVec.emplace_back("force");

   return retVec;
}
const char* CtsApi::replace_arg()     { return "replace"; }


std::string CtsApi::checkPtDefs(ecf::CheckPt::Mode m, int check_pt_interval, int check_pt_save_time_alarm)
{
    std::string ret = "--check_pt";
    if (m != ecf::CheckPt::UNDEFINED || check_pt_interval != 0 || check_pt_save_time_alarm != 0) ret += "=";

    switch (m) {
       case ecf::CheckPt::NEVER:   ret += "never"; break;
       case ecf::CheckPt::ON_TIME: ret += "on_time"; break;
       case ecf::CheckPt::ALWAYS:  ret += "always"; break;
       case ecf::CheckPt::UNDEFINED:   break; // leave empty
       default: assert(false); break;
    }

    if (check_pt_interval != 0) {
       if (m != ecf::CheckPt::UNDEFINED) ret += ":";
       ret += boost::lexical_cast<std::string>(check_pt_interval);
    }
    else {
       if (m == ecf::CheckPt::UNDEFINED && check_pt_save_time_alarm != 0) {
          ret += "alarm:";
          ret += boost::lexical_cast<std::string>(check_pt_save_time_alarm);
       }
    }
    return ret;
}
const char* CtsApi::checkPtDefsArg()     { return "check_pt"; }

std::string CtsApi::restoreDefsFromCheckPt()   { return "--restore_from_checkpt"; }
const char* CtsApi::restoreDefsFromCheckPtArg(){ return "restore_from_checkpt"; }


std::string CtsApi::logMsg(const std::string& theMsgToLog) {
   std::string ret =  "--msg=";
   ret += theMsgToLog;
   return ret;
}
const char* CtsApi::logMsgArg() { return "msg"; }


std::vector<std::string> CtsApi::force( const std::vector<std::string>& paths,
         const std::string& state_or_event,
         bool recursive,
         bool set_repeats_to_last_value)
{
   std::vector<std::string> retVec; retVec.reserve(paths.size() + 3);

   std::string ret = "--force="; ret += state_or_event;
   retVec.push_back(ret);
   if (recursive)                 retVec.emplace_back("recursive");
   if (set_repeats_to_last_value) retVec.emplace_back("full");
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}

std::vector<std::string> CtsApi::force( const std::string& path,
         const std::string& state_or_event,
         bool recursive,
         bool set_repeats_to_last_value)
{
   return CtsApi::force(std::vector<std::string>(1,path),state_or_event,recursive,set_repeats_to_last_value);
}
const char* CtsApi::forceArg()  { return "force"; }


std::vector<std::string> CtsApi::freeDep(const std::vector<std::string>& paths,bool trigger, bool all, bool date, bool time) {

   std::vector<std::string> retVec; retVec.reserve(paths.size() + 4);

   retVec.emplace_back("--free-dep");
   if (all)        retVec.emplace_back("all");
   else {
      if (trigger) retVec.emplace_back("trigger");
      if (date)    retVec.emplace_back("date");
      if (time)    retVec.emplace_back("time");
   }
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
std::vector<std::string> CtsApi::freeDep(const std::string& path,bool trigger, bool all, bool date, bool time) {

   return CtsApi::freeDep(std::vector<std::string>(1,path),trigger,all,date,time);
}
const char* CtsApi::freeDepArg()  { return "free-dep"; }


std::vector<std::string> CtsApi::file(const std::string& absNodePath, const std::string& fileType, const std::string& max_lines)
{
   std::vector<std::string> retVec; retVec.reserve(3);
   std::string ret = "--file="; ret += absNodePath;
   retVec.push_back(ret);
   retVec.push_back(fileType);
   retVec.push_back(max_lines);
   return retVec;
}
const char* CtsApi::fileArg()  { return "file"; }


std::vector<std::string> CtsApi::plug(const std::string& sourcePath, const std::string& destPath)
{
   std::vector<std::string> retVec; retVec.reserve(2);

   std::string ret = "--plug="; ret += sourcePath;
   retVec.push_back(ret);
   retVec.push_back(destPath);

   return retVec;
}
const char* CtsApi::plugArg()     { return "plug"; }

std::vector<std::string> CtsApi::alter(
         const std::vector<std::string>& paths,
         const std::string& alterType,
         const std::string& attrType,
         const std::string& name,
         const std::string& value)
{
   std::vector<std::string> retVec; retVec.reserve(5 + paths.size());

   retVec.emplace_back("--alter");
   retVec.push_back(alterType);
   retVec.push_back(attrType);
   if ( !name.empty() )  retVec.push_back(name);
   if ( !value.empty() ) retVec.push_back(value);
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
std::vector<std::string> CtsApi::alter(
         const std::string& path,
         const std::string& alterType,
         const std::string& attrType,
         const std::string& name,
         const std::string& value)
{
   return CtsApi::alter(std::vector<std::string>(1,path),alterType,attrType,name,value);
}

std::vector<std::string> CtsApi::alter_sort(
       const std::vector<std::string>& paths,
       const std::string& sortable_attribute_name,
       bool recursive)
{
   std::vector<std::string> retVec; retVec.reserve(2 + paths.size());

   retVec.emplace_back("--alter");
   retVec.emplace_back("sort");
   retVec.push_back(sortable_attribute_name);
   if ( recursive ) retVec.emplace_back("recursive");
   std::copy(paths.begin(),paths.end(),std::back_inserter(retVec));
   return retVec;
}
const char* CtsApi::alterArg() { return "alter"; }


std::string CtsApi::reloadwsfile()     { return "--reloadwsfile"; }
const char* CtsApi::reloadwsfileArg()  { return "reloadwsfile"; }

std::string CtsApi::reloadpasswdfile()     { return "--reloadpasswdfile"; }
const char* CtsApi::reloadpasswdfile_arg() { return "reloadpasswdfile";}

std::string CtsApi::group(const std::string& cmds) {
   std::string ret = "--group=";
   ret += cmds;
   return ret;
}
const char* CtsApi::groupArg()  { return "group"; }

std::vector<std::string> CtsApi::getLog(int lastLine) {
   std::vector<std::string> retVec; retVec.reserve(2);
   retVec.emplace_back("--log=get");
   if (lastLine != 0) {
      std::stringstream ss; ss << lastLine;
      retVec.push_back(ss.str());
   }
   return retVec;
}
std::vector<std::string> CtsApi::new_log(const std::string& new_path) {
   std::vector<std::string> retVec; retVec.reserve(2);
   retVec.emplace_back("--log=new");
   if (!new_path.empty()) retVec.push_back(new_path);
   return retVec;
}
std::string CtsApi::clearLog()     { return "--log=clear"; }
std::string CtsApi::flushLog()     { return "--log=flush"; }
std::string CtsApi::get_log_path() { return "--log=path"; }

std::string CtsApi::forceDependencyEval() { return "--force-dep-eval";}
const char* CtsApi::forceDependencyEvalArg() { return "force-dep-eval";}


std::vector<std::string> CtsApi::edit_script(
         const std::string& path_to_task,
         const std::string& edit_type,
         const std::string& path_to_script,
         bool create_alias,
         bool run
         )
{
   std::vector<std::string> retVec;
   std::string ret =  "--edit_script=";
   ret += path_to_task;
   retVec.push_back(ret);
   retVec.push_back(edit_type);
   if (!path_to_script.empty()) retVec.push_back(path_to_script);
   if (create_alias) retVec.emplace_back("create_alias");
   if (!run) retVec.emplace_back("no_run");
   return retVec;
}
const char* CtsApi::edit_script_arg() { return "edit_script";}

std::vector<std::string> CtsApi::query(
         const std::string& query_type, const std::string& path_to_attribute,
         const std::string& attribute)
{
   std::vector<std::string> retVec;  retVec.reserve(3);
   std::string ret = "--query="; ret += query_type ;
   retVec.push_back(ret);
   if (query_type == "event" || query_type == "meter" || query_type == "variable") {
      retVec.push_back(path_to_attribute + ":" + attribute);
   }
   else if (query_type == "trigger") {
      retVec.push_back(path_to_attribute);
      retVec.push_back(attribute);
   }
   else if (query_type == "state" || query_type == "dstate") {
      retVec.push_back(path_to_attribute);
   }
   return retVec;
}
const char* CtsApi::queryArg() { return "query";}

