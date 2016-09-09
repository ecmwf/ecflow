#ifndef SERVER_ENVIRONMENT_HPP_
#define SERVER_ENVIRONMENT_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : ServerEnvironment
// Author      : Avi
// Revision    : $Revision: #52 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//
// The server environment is read from the configuration file. This defines
// the defaults for the server,and is configurable by the user, at run time
// Alternatively some variable can be set via environment. See environment.cfg
//
// The server must be created before the log file. Since the log is initialised
// with the log file name from the server environment
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <string>
#include <vector>
#include <map>
#include <exception>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>

#include "Host.hpp"
#include "WhiteListFile.hpp"
#include "CheckPt.hpp"

// Added ServerEvinronmentException so that it can be in the same scope as server
// in ServerMain. Previously we had a separate try block
// Also distinguish between server and environment errors
class ServerEnvironmentException : public std::runtime_error {
public:
   ServerEnvironmentException(const std::string& msg) : std::runtime_error(msg) {}
};


class ServerEnvironment : private boost::noncopyable {
public:
   ServerEnvironment(int argc, char* argv[]);
   ServerEnvironment(int argc, char* argv[], const std::string& path_to_config_file); // *only used in test*
	~ServerEnvironment();

	/// return true if option are valid false and error message otherwise
	bool valid(std::string& errorMsg) const;

	/// return the directory path associated with smshome
	/// but can be overridden by the environment variable ECF_HOME
	std::string ecf_home() const { return ecfHome_; }

	/// returns the server host and port
	std::pair<std::string,std::string> hostPort() const;

	/// returns the server port. This has a default value defined in server_environment.cfg
	/// but can be overridden by the environment variable ECF_PORT
	int port() const { return serverPort_;}
	std::string the_port() const;

	/// returns the TCP protocol. default is TCPv4. Can be changed via command line to TCPv6
	boost::asio::ip::tcp tcp_protocol() const { return tcp_protocol_;}

	/// Return true if job generation is enabled. By default job generation is enabled however
	/// for test/debug we can elect to disable, it.
	bool jobGeneration() const { return jobGeneration_;}

   /// Forward compatibility allow old clients to talk to new server
	/// Temp: This allows ecflow  to be built. i.e we have old clients(3.0.x), installed on different machines
	///       they need to talk to new server. (i.e since we can't change the old clients)
   /// This is controlled with ECF_ALLOW_OLD_CLIENT_NEW_SERVER
   int allow_old_client_new_server() const { return allow_old_client_new_server_;}


	/// Whenever we save the checkpt, we time how long this takes.
	/// For very large definition the time can be significant and start to interfere with
	/// the scheduling. (i.e since write to disk is blocking).
	/// The default is 30 seconds. When the save time exceeds this, the late flag is
	/// set on the definition. This needs to be manually reset.
	size_t checkpt_save_time_alarm() const { return checkpt_save_time_alarm_;}
	void set_checkpt_save_time_alarm(size_t t) { checkpt_save_time_alarm_ = t ;}

	/// returns the path of the checkpoint file. This has a default name set in server_environment.cfg
	/// but can be overridden by the environment variable ECF_CHECK
   /// if the check point file starts with an absolute path return as is:
   /// otherwise returns ECF_HOME/ecf_checkpt_file_
	const std::string& checkPtFilename() const { return ecf_checkpt_file_; }

	/// returns the path of the old checkPointFile. This has a default name set in server_environment.cfg
	/// but can be overridden by the environment variable ECF_CHECKOLD
   /// if the check point file starts with an absolute path return as is:
   /// otherwise returns ECF_HOME/ecf_backup_checkpt_file_
	const std::string& oldCheckPtFilename() const { return ecf_backup_checkpt_file_; }

	/// returns the checkPt interval. This is the time in seconds, at which point the server
	/// serializes the defs node tree. This is called the check point file.
	/// This has a default value set in environment.cfg
	/// but can be overridden by the environment variable ECF_CHECKINTERVAL
	/// if set via environment variable it must be convertible to an integer
	int checkPtInterval() const { return checkPtInterval_;}

	/// set the check point interval. Typically set via client interface
	/// value should > 0 for valid values.
	void set_checkpt_interval(int v) { if (v > 0) checkPtInterval_ = v;}

	/// returns the check mode. This specifies options for saving of the checkPoint file:
   ecf::CheckPt::Mode checkMode() const { return checkMode_;}
   void set_check_mode(ecf::CheckPt::Mode m) { checkMode_ = m; }
   std::string check_mode_str() const;

	/// returns the number of seconds at which we should check time dependencies
	/// this includes evaluating trigger dependencies and submit the corresponding jobs.
	/// This is set at 60 seconds. But will vary for debug/test purposes only.
	/// For Testing the state change queued->submitted->active duration < submitJobsInterval
	/// If this state change happens at the job submission boundary then
	/// time series can get a skew.
	int submitJobsInterval() const { return submitJobsInterval_;}

	/// returns server variables, as vector of pairs.
	/// Some of these variables hold environment variables
	/// Note:: additional variable are created for use by clients, i.e like
	///        ECF_PORT(jobs, server, client)
	///        ECF_NODE(jobs).   ECF_NODE is the server machine host name
  	void variables(std::vector<std::pair<std::string,std::string> >&) const;

	/// Ask the server to reload file the hold list of users and their access rights
	/// The white list file is specified by the environment variable ECF_LISTS
	/// This allows/disallows user access on the live server
	/// Return true if file is reloaded ok, else false and error message if:
	/// 	a/ File does not exist
	///  	b/ File is empty
	///  	c/ Errors in parsing file
	/// If errors arise the exist user still stay in affect
  	bool reloadWhiteListFile(std::string& errorMsg);

#ifdef ECFLOW_MT
  	// returns the numbers threads to be used by the server.
   size_t threads() const { return threads_; };
#endif

	/// There are several kinds of authentification:
	///     a/ None
	///     b/ List mode.   ASCII file based on ECF_LISTS is defined
	///     c/ Secure mode. binary file based ECF_PASSWD is defined
	/// At the moment we will only implement options a/ and b/
	//
	/// Returns true if the given user has access to the server, false otherwise
   bool authenticateReadAccess(const std::string& user)const;
   bool authenticateReadAccess(const std::string& user,const std::string& path)const;
   bool authenticateReadAccess(const std::string& user,const std::vector<std::string>& paths)const;
 	bool authenticateWriteAccess(const std::string& user) const;
   bool authenticateWriteAccess(const std::string& user,const std::string& path)const;
   bool authenticateWriteAccess(const std::string& user,const std::vector<std::string>& paths)const;

	/// return true if help option was selected
   bool help_option() const  { return help_option_; }
   bool version_option() const  { return version_option_; }

 	/// debug is enabled via --debug or server invocation
 	bool debug() const { return debug_;}
   void set_debug(bool f) { debug_ = f;}

	std::string dump() const;

private:
   void init(int argc, char* argv[], const std::string& path_to_config_file);

	///  defaults are read from a config file
	void read_config_file(std::string& log_file_name,const std::string& path_to_config_file);

	/// Get the standard environment variables, overwrite any settings from config file
	void read_environment_variables(std::string& log_file_name);

	void change_dir_to_ecf_home_and_check_accesibility();

private:
	ecf::Host host_name_;
	std::string serverHost_; // must be after host_name_, since used in init
	int  serverPort_;
	int  checkPtInterval_;
	int  checkpt_save_time_alarm_;
	int  submitJobsInterval_;
#ifdef ECFLOW_MT
	size_t threads_;
#endif
	bool jobGeneration_;   // used in debug/test mode only
	bool debug_;
   bool help_option_;
   bool version_option_;
	int allow_old_client_new_server_;
 	ecf::CheckPt::Mode checkMode_;
	std::string ecfHome_;
	std::string ecf_checkpt_file_;
	std::string ecf_backup_checkpt_file_;
	std::string ecf_pid_;
	std::string killCmd_;
	std::string statusCmd_;
	std::string urlCmd_;
	std::string urlBase_;
	std::string url_;
	std::string ecf_cmd_;
	std::string ecf_micro_;
   std::string ecf_white_list_file_;
   mutable WhiteListFile white_list_file_;
	boost::asio::ip::tcp tcp_protocol_;      // defaults to IPv4 TCP protocol
	friend class ServerOptions;
};

#endif
