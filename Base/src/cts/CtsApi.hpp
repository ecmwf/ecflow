#ifndef CTS_API_HPP_
#define CTS_API_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #74 $ 
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
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>
#include "NodeFwd.hpp"
#include "CheckPt.hpp"

// The two variant api must correspond i.e '--get' and 'get' since this is used by boost program options
// *************************************************************************************
// Note:: if the api requires multiple parameters it must return a std::vector<std::string>
// ***************************************************************************************
class CtsApi : private boost::noncopyable  {
public:
	static std::string to_string(const std::vector<std::string>& );

   static std::string server_version();

	// For python
	static defs_ptr load( defs_ptr defs) { return defs;}
	static std::vector<std::string> loadDefs(const std::string& filePath,bool force,bool check_only);
   static std::string get(const std::string& absNodePath = "");
   static std::string get_state(const std::string& absNodePath = "");
   static std::string migrate(const std::string& absNodePath = "");
   static std::vector<std::string> sync(unsigned int client_handle, unsigned int state_change_no, unsigned int modify_change_no );
   static std::string sync_full(unsigned int client_handle);
	static std::vector<std::string> news(unsigned int client_handle, unsigned int state_change_no, unsigned int modify_change_no );

	static std::vector<std::string> ch_register(  bool auto_add_new_suites, const std::vector<std::string>& suites);
   static std::string ch_suites();
   static std::string ch_drop(int client_handle);
   static std::string ch_drop_user(const std::string& user);
	static std::vector<std::string> ch_add(int client_handle, const std::vector<std::string>& suites);
	static std::vector<std::string> ch_remove(int client_handle, const std::vector<std::string>& suites);
	static std::vector<std::string> ch_auto_add(int client_handle,  bool auto_add_new_suites);
	static std::string suites(); // returns list if all suites, and client handles.

	static std::string restartServer();
	static std::string haltServer(bool auto_confirm = true);
	static std::string shutdownServer(bool auto_confirm = true);
	static std::string terminateServer(bool auto_confirm = true);
   static std::string pingServer();
   static std::string server_load(const std::string& path_to_log_file);
   static std::string debug_server_on();
   static std::string debug_server_off();

	// odd one out, take multi-args but expects string
	static std::string begin(const std::string& suiteName = "", bool force = false); // no suite begins all suites

   static std::string job_gen(const std::string& absNodePath = "");
   static std::string checkJobGenOnly(const std::string& absNodePath = "");

   static std::vector<std::string> check(const std::string& absNodePath = "");
   static std::vector<std::string> check(const std::vector<std::string>& paths);
   static std::vector<std::string> delete_node(const std::vector<std::string>& paths, bool force = false, bool auto_confirm = true); // no paths specified ,delete all suites, for test
   static std::vector<std::string> delete_node(const std::string& absNodePath = "", bool force = false, bool auto_confirm = true); // no node specified ,delete all suites, for test
   static std::vector<std::string> suspend(const std::string& absNodePath);
   static std::vector<std::string> suspend(const std::vector<std::string>& paths);
	static std::vector<std::string> resume(const std::string& absNodePath);
   static std::vector<std::string> resume(const std::vector<std::string>& paths);
	static std::vector<std::string> kill(const std::string& absNodePath);
   static std::vector<std::string> kill(const std::vector<std::string>& paths);
   static std::vector<std::string> status(const std::string& absNodePath);
   static std::vector<std::string> status(const std::vector<std::string>& paths);
   static std::vector<std::string> edit_history(const std::vector<std::string>& paths);
   static std::vector<std::string> edit_history(const std::string& absNodePath);

	static std::string why(const std::string& absNodePath);
	static std::string zombieGet();
	static std::vector<std::string> zombieFob(const std::string& task_path,const std::string& process_id, const std::string& password);
	static std::vector<std::string> zombieFail(const std::string& task_path,const std::string& process_id, const std::string& password);
	static std::vector<std::string> zombieAdopt(const std::string& task_path,const std::string& process_id, const std::string& password);
	static std::vector<std::string> zombieRemove(const std::string& task_path,const std::string& process_id, const std::string& password);
   static std::vector<std::string> zombieBlock(const std::string& task_path,const std::string& process_id, const std::string& password);
   static std::vector<std::string> zombieKill(const std::string& task_path,const std::string& process_id, const std::string& password);
	static std::string zombieFobCli(const std::string& task_path);
	static std::string zombieFailCli(const std::string& task_path);
	static std::string zombieAdoptCli(const std::string& task_path);
	static std::string zombieRemoveCli(const std::string& task_path);
   static std::string zombieBlockCli(const std::string& task_path);
   static std::string zombieKillCli(const std::string& task_path);

	static std::vector<std::string> replace(  const std::string& absNodePath,
	                                              const std::string& path_to_client_defs,
	                                              bool create_parents_as_required = true,
	                                              bool force = false);
   static std::vector<std::string> requeue(const std::vector<std::string>& paths, const std::string& option/* [ "" | "force" | "abort" ] */);
   static std::vector<std::string> requeue(const std::string& absNodePath, const std::string& option/* [ "" | "force" | "abort" ] */);
   static std::vector<std::string> run(const std::vector<std::string>& paths,bool force = false);
   static std::vector<std::string> run(const std::string& absNodePath,bool force = false);
	static std::vector<std::string> order(const std::string& absNodePath,const std::string& orderType);

	static std::string checkPtDefs(ecf::CheckPt::Mode m = ecf::CheckPt::UNDEFINED, int check_pt_interval = 0, int check_pt_save_time_alarm = 0);
	static std::string restoreDefsFromCheckPt();

	static std::vector<std::string> force(const std::vector<std::string>& paths,const std::string& state_or_event,bool recursive = false,bool set_repeats_to_last_value = false);
   static std::vector<std::string> force(const std::string& path,const std::string& state_or_event,bool recursive = false,bool set_repeats_to_last_value = false);

   static std::vector<std::string> freeDep(const std::vector<std::string>& paths,bool trigger = true, bool all = false, bool date = false, bool time = false);
   static std::vector<std::string> freeDep(const std::string& absNodePath,bool trigger = true, bool all = false, bool date = false, bool time = false);

	static std::vector<std::string> file(const std::string& absNodePath, const std::string& fileType, const std::string& max_lines);
	static std::vector<std::string> plug(const std::string& sourcePath, const std::string& destPath);

	static std::vector<std::string> alter(const std::string& path,
	                                      const std::string& alterType, /* one of [ 'add' | 'change' | 'delete' | 'set_flag' | 'clear_flag' ] */
	                                      const std::string& attrType,
	                                      const std::string& name = "",
	                                      const std::string& value = "");
   static std::vector<std::string> alter(const std::vector<std::string>& paths,
                                         const std::string& alterType, /* one of [ 'add' | 'change' | 'delete' | 'set_flag' | 'clear_flag' ] */
                                         const std::string& attrType,
                                         const std::string& name = "",
                                         const std::string& value = "");

	static std::string reloadwsfile();
   static std::string reloadpasswdfile();

	// "expect string of the form "shutdown; get" must be ';' separated
	static std::string group(const std::string& cmds);

	static std::string logMsg(const std::string& theMsgToLog);
	static std::vector<std::string> getLog(int lastLines  = 0);
	static std::vector<std::string> new_log(const std::string& new_path);
   static std::string get_log_path();
   static std::string clearLog();
	static std::string flushLog();
	static std::string forceDependencyEval();

   static std::string stats();
   static std::string stats_server(); // used in test, as serialisation subject to change
   static std::string stats_reset();

 	static std::vector<std::string> edit_script(const std::string& path_to_task,
 	                                            const std::string& edit_type,
 	                                            const std::string& path_to_script = "",
 	                                            bool create_alias = false,
 	                                            bool run = true);


 	// Only to be used in Cmd
   static const char* server_version_arg();
   static const char* statsArg();
   static const char* stats_server_arg();
   static const char* stats_reset_arg();
	static const char* suitesArg();
	static const char* ch_register_arg();
   static const char* ch_drop_arg();
   static const char* ch_suites_arg();
   static const char* ch_drop_user_arg();
	static const char* ch_add_arg();
	static const char* ch_remove_arg();
	static const char* ch_auto_add_arg();

	static const char* terminateServerArg();
	static const char* restartServerArg();
	static const char* haltServerArg();
	static const char* shutdownServerArg();
   static const char* pingServerArg();
   static const char* server_load_arg();
   static const char* debug_server_on_arg();
   static const char* debug_server_off_arg();

   static const char* getArg();
   static const char* get_state_arg();
   static const char* migrate_arg();
   static const char* syncArg();
   static const char* sync_full_arg();
	static const char* newsArg();
	static const char* loadDefsArg();
	static const char* beginArg();
   static const char* job_genArg();
   static const char* check_arg();
   static const char* checkJobGenOnlyArg();
	static const char* delete_node_arg();
	static const char* suspend_arg();
	static const char* resume_arg();
	static const char* kill_arg();
   static const char* statusArg();
   static const char* edit_history_arg();
	static const char* whyArg();
	static const char* zombieGetArg();
	static const char* zombieFobArg();
	static const char* zombieFailArg();
	static const char* zombieAdoptArg();
	static const char* zombieRemoveArg();
   static const char* zombieBlockArg();
   static const char* zombieKillArg();
	static const char* replace_arg();
	static const char* requeueArg();
	static const char* runArg();
	static const char* orderArg();
	static const char* checkPtDefsArg();
	static const char* restoreDefsFromCheckPtArg();
	static const char* logMsgArg();
	static const char* forceArg();
	static const char* freeDepArg();
	static const char* fileArg();
	static const char* plugArg();
	static const char* reloadwsfileArg();
   static const char* reloadpasswdfile_arg();
	static const char* groupArg();
	static const char* forceDependencyEvalArg();
	static const char* alterArg();
	static const char* edit_script_arg();
};
#endif
