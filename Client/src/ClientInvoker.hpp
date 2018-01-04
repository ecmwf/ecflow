#ifndef CLIENT_INVOKER_HPP_
#define CLIENT_INVOKER_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision$ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "boost/date_time/posix_time/posix_time_types.hpp"

#include "ClientEnvironment.hpp"
#include "ClientOptions.hpp"
#include "Cmd.hpp"
#include "CtsApi.hpp"
#include "TaskApi.hpp"
#include "NodeFwd.hpp"
#include "ServerReply.hpp"
#include "Zombie.hpp"
#include "NOrder.hpp"

/// Invokes the client depending on the arguments
/// This has been separated from main, to allow us to invoke the client
/// from a test suite.
/// Important: We can make *many* calls with the same ClientInvoker.
/// This is more efficient than creating a ClientInvoker for each request
class ClientInvoker : private boost::noncopyable {
public:
	/// Will create the *ClientEnvironment* once on construction
	/// By default will throw exception std::runtime_error for errors
   ClientInvoker();
   ClientInvoker(const std::string& host_port);
   ClientInvoker(const std::string& host, const std::string& port);
   ClientInvoker(const std::string& host, int port);

   /// for debug allow the current client environment to be printed
   std::string to_string() const { return clientEnv_.toString();}

	/// if throw_exception_on_error = false, then
	///    invoke() will return 0 for success and 1 for error.
	///    The error message can be retrieved from errorMsg()
	/// if throw_exception_on_error = true,
	///	  then invoke() for errors will throw std::runtime_error
	///   will still return 0 for success.
	void set_throw_on_error(bool f) { on_error_throw_exception_ = f;}

	/// Return the time it takes to contact server and get a reply.
	const boost::posix_time::time_duration& round_trip_time() const { return rtt_;}

	/// Configure to using command line interface
	/// This affect commands like ping, log & file, so that, output is written to standard out
	/// Can also be used to check argument arguments
	void set_cli(bool f) { clientEnv_.set_cli(f); }
	bool cli() const { return clientEnv_.get_cli(); }

	/// This will override the environment setting.
	/// In particular setting host explicitly will avoid cycling through server list,
	/// if connection fails. hence will bomb out earlier
	/// If applied to child command's will continue attempting this host/port until timeout
   void set_host_port(const std::string& h, const std::string& p);
   void set_hostport(const std::string& host_port);
   const std::string& host() const;
   const std::string& port() const;

	/// Whenever there is a connections failure we wait a number of seconds
	/// before trying again. ( i.e. to get round glitches in the network.)
	/// For the ping command this is set as 1 second
	/// This wait between connection attempts can be configured here.
	/// i.e for the GUI & python interface this can be reduced to increase responsiveness.
	/// Default: In debug this period in set to 1 second and in release mode 10 seconds
	void set_retry_connection_period(unsigned int period) { retry_connection_period_ = period; }

	/// Set the number of times to connect to server, in case of failure
	/// The period between connection attempts is handled by set_retry_connection_period
	/// i.e for the GUI & python interface this can be reduced to increase responsiveness.
	/// Default value is set as 2. Setting a value less than 1 is ignored, will default to 1 in this case
	void set_connection_attempts( unsigned int attempts);

	/// returns 1 on error and 0 on success. The errorMsg can be accessed via errorMsg()
	/// Will attempt to connect to the server a number of times. If this fails it will
	/// try the next server, and so on until a timeout period is reached.
	int invoke( int argc, char* argv[]) const;

   // support for forward compatibility, by changing boost archive version
   // Chosen to change client side only
   void allow_new_client_old_server(int archive_version_of_old_server);
   int allow_new_client_old_server() const;

	/// If testing, overwrite the task path set in the environment, required for
	/// testing the task based commands.
	/// By allowing environment to be changed, we allow smsinit,smscomplete, etc to be replaced
	/// with ECF_CLIENT path executable
	/// The following functions are only used for testing purposes.
	/// Override task paths,env' & job creation at begin time
	void taskPath(const std::string& s);
	void set_jobs_password(const std::string&);
 	void setEnv( const std::vector<std::pair<std::string,std::string> >& e);
 	void testInterface(); // allow cmd construction to be aware thats it under test
   const std::string& process_or_remote_id() const;

 	/// record each request its arguments and the round trip time to and from server
 	/// If the file can not opened for create/append then an runtime error exception is thrown
   void enable_logging(const std::string& log_file_name);
   void disable_logging();

 	/// The timeout feature allow the client to fail gracefully in the case
 	/// where the server has died/crashed. The timeout will ensure the socket is closed.
 	/// allowing the server to be restarted without getting the address is use error.
 	/// Set the timeout for each client->server communication:
   //    connect        : timeout_ second
   //    send request   : timeout_ second
   //    receive reply  : timeout_ second
 	// default is 0 second, which means take the timeout from the command/request
 	// used for test only
 	void set_connect_timeout(int t);

 	/// ServerReply Holds the reply from the server
 	void reset(); // will clear local client definition and handle
 	const ServerReply& server_reply() const { return server_reply_;}
 	defs_ptr defs() const { return server_reply_.client_defs(); }
	const std::string& get_string() const { return server_reply_.get_string(); }
  	bool in_sync() const { return server_reply_.in_sync();}
   bool get_news() const { return (server_reply_.get_news() != ServerReply::NO_NEWS); }
	int client_handle() const { return server_reply_.client_handle(); }

	/// If invoke returns 1, the error message can be retrieved with this function
	const std::string& errorMsg() const { return server_reply_.error_msg();}

	// ***************************************************************************
	// Task/child based api. Only added here for test.
	// Relies on environment for the other args
	int initTask(const std::string& process_id)const
	   { return invoke(TaskApi::init(process_id)); }
	int abortTask(const std::string& reason_why = "") const
	   { return invoke(TaskApi::abort(reason_why)); }
	int eventTask(const std::string& eventName)  const
	   { return invoke(TaskApi::event(eventName)); }
	int meterTask(const std::string& meterName, const std::string& new_meter_value) const
	   { return invoke(TaskApi::meter(meterName,new_meter_value)); }
	int labelTask(const std::string& labelName, const std::vector<std::string>& labels) const
	   { return invoke(TaskApi::label(labelName,labels)); }
   int waitTask(const std::string& on_expression) const
      { return invoke(TaskApi::wait(on_expression)); }
   int queueTask(const std::string& queue_name, const std::string& path_to_node_with_queue = "") const
      { return invoke(TaskApi::queue(queue_name,path_to_node_with_queue)); }
	int completeTask() const
	   { return invoke(TaskApi::complete()); }

	// Support for python child commands, and python jobs
	void set_child_path(const std::string& path) { clientEnv_.set_child_path(path);}
	void set_child_password(const std::string& pass) { clientEnv_.set_child_password(pass);}
	void set_child_pid(const std::string& pid) { clientEnv_.set_child_pid(pid);}
	void set_child_try_no(unsigned int try_no) { clientEnv_.set_child_try_no(try_no);}
	void set_child_timeout(unsigned int seconds) { clientEnv_.set_child_cmd_timeout(seconds);} // ECF_TIMEOUT default is 24 hours allow python jobs to override
   void set_zombie_child_timeout(unsigned int seconds){clientEnv_.set_zombie_child_cmd_timeout(seconds);} // ECF_ZOMBIE_TIMEOUT default is 24 hours allow python jobs to override

	void child_init();
	void child_abort(const std::string& reason  = "");
	void child_event(const std::string& event_name_or_number);
	void child_meter(const std::string& meter_name, int meter_value);
	void child_label(const std::string& label_name, const std::string& label_value);
    void child_wait(const std::string& on_expression);
   std::string child_queue(const std::string& queue_name, const std::string& path_to_node_with_queue = "");
	void child_complete();

	// ********************************************************************************
	// The client api. Mirrors CtsApi on the whole
	int getDefs() const;
 	int loadDefs(const std::string& filePath,
 	               bool force = false,      /* true means overwrite suite of same name */
                  bool check_only = false, /* client side only, true means don't send to server, just check only */
                  bool print = false       /* client side only, print the defs */
 	) const;
 	int load( const defs_ptr& defs, bool force  = false /*true means overwrite suite of same name*/) const
 	       { return load_in_memory_defs(defs,force); }
	int sync(defs_ptr& client_defs) const;
   int sync_local(bool sync_suite_clock = false) const;
	int news(defs_ptr& client_defs) const;
	int news_local() const;

	// find free port on local host. Not 100% accurate, use in test
	static std::string find_free_port(int seed_port_number, bool debug = false);

 	bool wait_for_server_reply(int time_out = 60) const; // wait for server reply, returning false means timed out.
   bool wait_for_server_death(int time_out = 60) const; // wait for server reply, returning true means server died,false means timed out.
	int restartServer() const;
	int haltServer() const;
	int shutdownServer() const ;
	int terminateServer() const;
   int pingServer() const;
   int server_load(const std::string& path_to_log_file = "") const { return invoke(CtsApi::server_load(path_to_log_file)); }
   int debug_server_on() const;
   int debug_server_off() const;
   int stats() const;        // returns stats as string, server does formatting, & hence is free to change ECFLOW-880
   int stats_reset() const;
   int stats_server() const; // for test only, as stats returned may change for each release ECFLOW-880
   int server_version() const;

	int suites() const;
	int ch_register( bool auto_add_new_suites, const std::vector<std::string>& suites) const;
   int ch_suites() const;
   int ch_drop(int client_handle) const;
   int ch_drop_user(const std::string& user = "") const;
	int ch_add(int client_handle, const std::vector<std::string>& suites) const;
	int ch_remove(int client_handle, const std::vector<std::string>& suites) const;
	int ch_auto_add(int client_handle, bool auto_add_new_suites) const;
	int ch1_drop() const;
	int ch1_add(const std::vector<std::string>& suites) const;
	int ch1_remove(const std::vector<std::string>& suites) const;
	int ch1_auto_add(bool auto_add_new_suites) const;

	int begin(const std::string& suiteName,bool force = false) const;
 	int begin_all_suites(bool force = false) const;

 	int zombieGet() const;
 	int zombieFob(const Zombie& z) const;
 	int zombieFail(const Zombie& z) const;
 	int zombieAdopt(const Zombie& z) const;
 	int zombieBlock(const Zombie& z) const;
   int zombieRemove(const Zombie& z) const;
   int zombieKill(const Zombie& z) const;
	int zombieFobCli(const std::string& absNodePath) const;
 	int zombieFailCli(const std::string& absNodePath) const;
 	int zombieAdoptCli(const std::string& absNodePath) const;
 	int zombieBlockCli(const std::string& absNodePath) const;
   int zombieRemoveCli(const std::string& absNodePath) const;
   int zombieKillCli(const std::string& absNodePath) const;
   int zombieFobCliPaths(const std::vector<std::string>& paths) const;
   int zombieFailCliPaths(const std::vector<std::string>& paths) const;
   int zombieAdoptCliPaths(const std::vector<std::string>& paths) const;
   int zombieBlockCliPaths(const std::vector<std::string>& paths) const;
   int zombieRemoveCliPaths(const std::vector<std::string>& paths) const;
   int zombieKillCliPaths(const std::vector<std::string>& paths) const;

   int job_gen(const std::string& absNodePath)  const;

   int edit_history(const std::string& path) const;
   int kill(const std::vector<std::string>& paths) const;
   int kill(const std::string& absNodePath)  const;
   int status(const std::vector<std::string>& paths) const;
 	int status(const std::string& absNodePath)  const;
   int suspend(const std::vector<std::string>& paths) const;
   int suspend(const std::string& absNodePath) const;
   int resume(const std::vector<std::string>& paths) const;
 	int resume(const std::string& absNodePath)  const;
   int check(const std::vector<std::string>& paths) const;
   int check(const std::string& absNodePath) const;
   int delete_nodes(const std::vector<std::string>& paths,bool force = false) const;
   int delete_node(const std::string& absNodePath,bool force = false) const;
 	int delete_all(bool force = false) const;
   int archive(const std::vector<std::string>& paths,bool force = false) const;
   int archive(const std::string& absNodePath,bool force = false) const;
   int restore(const std::vector<std::string>& paths) const;
   int restore(const std::string& absNodePath) const;

	int replace(  const std::string& absNodePath, const std::string& path_to_client_defs,
	                  bool create_parents_as_required = true, bool force = false)  const;
   int replace_1(  const std::string& absNodePath, defs_ptr client_defs, bool create_parents_as_required = true, bool force = false)  const;

   int requeue(const std::vector<std::string>& paths,const std::string& option = "") const;
   int requeue(const std::string& absNodePath,const std::string& option = "") const;
   int run(const std::vector<std::string>& paths,bool force = false) const;
   int run(const std::string& absNodePath,bool force = false) const;
   int order(const std::string& absNodePath,const std::string& order) const; // slow
   int order(const std::string& absNodePath,NOrder::Order) const;             // fast

   int checkPtDefs(ecf::CheckPt::Mode m = ecf::CheckPt::UNDEFINED, int check_pt_interval = 0, int check_pt_save_time_alarm = 0) const;
	int restoreDefsFromCheckPt() const;

	int force(const std::string& absNodePath,const std::string& state_or_event,bool recursive = false,bool set_repeats_to_last_value = false) const;
   int force(const std::vector<std::string>& paths,const std::string& state_or_event,bool recursive = false,bool set_repeats_to_last_value = false) const;

 	int freeDep(const std::vector<std::string>& paths,bool trigger = true, bool all = false, bool date = false, bool time = false) const;
   int freeDep(const std::string& absNodePath,bool trigger = true, bool all = false, bool date = false, bool time = false) const;

	int file(const std::string& absNodePath, const std::string& fileType, const std::string& max_lines = "10000") const;

	int plug(const std::string& sourcePath, const std::string& destPath) const;

	int query(const std::string& query_type, const std::string& path_to_attribute, const std::string& attribute);

	int alter(const std::vector<std::string>& paths,
	          const std::string& alterType, /* one of [ add | change | delete | set_flag | clear_flag ] */
	          const std::string& attrType,
	          const std::string& name = "",
	          const std::string& value = "") const { return invoke(CtsApi::alter(paths,alterType,attrType,name,value)); }
   int alter(const std::string& path,
             const std::string& alterType, /* one of [ add | change | delete | set_flag | clear_flag ] */
             const std::string& attrType,
             const std::string& name = "",
             const std::string& value = "") const { return invoke(CtsApi::alter(path,alterType,attrType,name,value)); }
   int alter_sort(const std::vector<std::string>& paths,
              const std::string& sortable_attribute_name,
              bool recursive = true) const { return invoke(CtsApi::alter_sort(paths,sortable_attribute_name,recursive)); }
   int alter_sort(const std::string& path,
              const std::string& sortable_attribute_name,
              bool recursive = true) const { return invoke(CtsApi::alter_sort(std::vector<std::string>(1,path),sortable_attribute_name,recursive)); }

	int reloadwsfile() const;
	int reloadpasswdfile() const;

	int group(const std::string& groupRequest) const;

	int logMsg(const std::string& msg) const;
   int new_log(const std::string& new_path = "") const;
	int getLog(int lastLines = 0)  const;
	int clearLog() const;
   int flushLog() const;
   int get_log_path() const;

	int forceDependencyEval() const;

	/// The first is for use by CLI(Commend level interface), the other are for ecFlowview/Python
	int edit_script(  const std::string& path_to_task,
	                  const std::string& edit_type,
	                  const std::string& path_to_script = "",
	                  bool create_alias = false,
	                  bool run = true)
		{return invoke(CtsApi::edit_script(path_to_task,edit_type,path_to_script,create_alias,run));}
	int edit_script_edit(const std::string& path_to_task);                                                     // ecFlowview EDIT
 	int edit_script_preprocess(const std::string& path_to_task);                                               // ecFlowview PRE_PROCESS
 	int edit_script_submit(const std::string& path_to_task,const NameValueVec& used_variables );               // ecFlowview SUBMIT
 	int edit_script_preprocess(const std::string& path_to_task,const std::vector<std::string>& file_contents); // ecFlowview PRE_PROCESS USER File
  	int edit_script_submit(const std::string& path_to_task,
	                       const NameValueVec& used_variables,
	                       const std::vector<std::string>& file_contents,
	                       bool alias = false,
	                       bool run = true);                                                                   // ecFlowview SUBMIT_FILE

private:
	/// returns 1 on error and 0 on success. The errorMsg can be accessed via errorMsg()
	int invoke( const std::string& arg ) const;
	int invoke( const std::vector<std::string>& args ) const;
   int invoke(Cmd_ptr) const; // assumes clients of Cmd_ptr constructor has caught exceptions

   int do_invoke_cmd(Cmd_ptr) const;
	int load_in_memory_defs( const defs_ptr& clientDefs, bool force) const; /// For clients that want to load a in memory definition into the server.
 	std::string client_env_host_port() const;
 	void check_child_parameters() const;

private:
   friend class RoundTripRecorder;
   friend class RequestLogger;
private:
	bool on_error_throw_exception_;
	bool test_;                            // used in testing only
	bool testInterface_;                   // used in testing only
	unsigned int connection_attempts_;     // No of attempts to establish connection with the server
	unsigned int retry_connection_period_; // No of seconds to wait before trying to connect in case of failure.

	mutable boost::posix_time::time_duration rtt_;// record latency for each cmd.
	mutable boost::posix_time::ptime start_time_; // Used for time out and measuring latency
	mutable ClientEnvironment clientEnv_;         // Will read the environment *once* on construction. Must be before Client options
	mutable ClientOptions     args_;              // Used for argument parsing & creating client request
	mutable ServerReply server_reply_;            // stores the local defs, client_handle, & all server replies

   /// For use by python interface,
   std::vector<std::string>::const_iterator changed_node_paths_begin() const { return server_reply_.changed_nodes().begin();}
   std::vector<std::string>::const_iterator changed_node_paths_end() const { return server_reply_.changed_nodes().end();}
   friend void export_Client();
};

// Allow logging and debug output of request round trip times
class RequestLogger : private boost::noncopyable  {
public:
   RequestLogger(const ClientInvoker* ci);
   ~RequestLogger();
   void set_cts_cmd(Cmd_ptr cmd) { cmd_ = cmd;}
private:
   const ClientInvoker* ci_;
   Cmd_ptr cmd_;
};

class RoundTripRecorder : private boost::noncopyable  {
public:
   RoundTripRecorder(const ClientInvoker* ci);
   ~RoundTripRecorder();
private:
   const ClientInvoker* ci_;
};

#endif
