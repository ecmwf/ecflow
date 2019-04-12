#ifndef CLIENT_ENVIRONMENT_HPP_
#define CLIENT_ENVIRONMENT_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : ClientEnvironment
// Author      : Avi
// Revision    : $Revision$ 
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

#include "AbstractClientEnv.hpp"
#ifdef ECF_OPENSSL
#include "Openssl.hpp"
#endif

class ClientEnvironment : public AbstractClientEnv {
public:
	/// The constructor will load the environment
	ClientEnvironment();

	/// This constructor is only used in Test environment, as it allow the host file to be set
	ClientEnvironment(const std::string& hostFile, const std::string& host = "", const std::string& port = "");

   /// This controls for how long child commands continue trying to connect to Server before failing.
	/// Maximum time in seconds for client to deliver message to server/servers. This is
	/// typically 24 hours in a real environment. It is this long to allow operators to
	/// recover from any crashes.
	/// for the CHILD/task commands *ONLY*
	/// Can be overridden by  changing environment variable ECF_TIMEOUT
	long max_child_cmd_timeout() const  { return timeout_; }

   /// This controls for how long child zombie commands continue trying to connect to Server before failing.
   /// Maximum time in seconds for client to deliver message to server/servers. This is
   /// typically 24 hours in a real environment.
   /// for the CHILD/task commands *ONLY*
   /// Can be overridden by  changing environment variable ECF_ZOMBIE_TIMEOUT
   long max_zombie_child_cmd_timeout() const  { return zombie_timeout_; }

	/// Allow pure python jobs to override the ECF_TIMEOUT & ECF_ZOMBIE_TIMEOUT
   void set_child_cmd_timeout(unsigned int t) { timeout_ = t; }
   void set_zombie_child_cmd_timeout(unsigned int t) { zombie_timeout_ = t; }

   /// The timeout feature allow the client to fail gracefully in the case
   /// where the server has died/crashed. The timeout will ensure the socket is closed.
   /// allowing the server to be restarted without getting the address is use error.
   /// Set the timeout for each client->server communication:
   //    connect        : timeout_ second
   //    send request   : timeout_ second
   //    receive reply  : timeout_ second
   // default is 0 second, which means take the timeout from the command/request
	// The default can be overridden with ECF_CONNECT_TIMEOUT.
	// This is useful when running valgrind --tool=callgrind which leads to
	// massive slow down and disconnection from the client, when the server is still
	// running.
   // used for **test only**
   int connect_timeout() const { return connect_timeout_;}

	/// When called will demand load the ecf host file, and read the hosts.
	/// Will then iterate over the host, when the end is reached will start over
	/// If host can not be opened or errors occur returns false;
	bool get_next_host(std::string& errorMsg);

	/// If server denies client communication and this flag is set, exit with an error
	/// Avoids 24hr hour connection attempt to server. Aids zombie control.
	bool denied() const { return denied_;}

	/// if NO_ECF is set then abort client immediately.  Client should return success
	/// useful when we want to test jobs stand-alone
	bool no_ecf() const { return no_ecf_;}

	/// for debug
	std::string toString() const;

	/// Return contents of ECF_HOST environment variable, Otherwise an EMPTY string
	static std::string hostSpecified();

	/// Returns of ECF_PORT,environment variable, otherwise returns Str::DEFAULT_PORT_NUMBER
	static std::string portSpecified();

   // Set the rid. Only needed to override in regression test, to avoid interference
   // May be need a better name, is really the process id of running job.
   // Needs to be the same value as supplied to child command init
   void set_remote_id(const std::string& rid) { remote_id_ = rid; }

   // Used by python to enable debug of client api
   void set_debug(bool flag);

#ifdef ECF_OPENSSL
   /// return true if this is a ssl enabled server
   ecf::Openssl& openssl() { return ssl_;}
   bool ssl() const { return ssl_.enabled();}
   void enable_ssl(const std::string& ssl) { ssl_.enable(ssl);}
#endif

// AbstractClientEnv functions:
 	bool checkTaskPathAndPassword(std::string& errorMsg) const override;
 	const std::string& task_path() const override { return task_path_; }
 	int task_try_no() const override { return task_try_num_; }
	const std::string& jobs_password() const override { return jobs_password_ ;}
	const std::string& process_or_remote_id() const override { return remote_id_; }
   void set_host_port(const std::string& host, const std::string& port) override;
   const std::string& host() const override;
   const std::string& port() const override;
	const std::vector<std::pair<std::string,std::string> >& env() const override { return env_;}
	const std::string& get_user_password() const override;
	bool debug() const override { return debug_;} //enabled if ECF_DEBUG_CLIENT set
   void set_test() override { under_test_ = true; }
   bool under_test() const override { return under_test_; }

   // Support for python child commands, and python jobs
   void set_child_path(const std::string& path) { task_path_ = path;}
   void set_child_password(const std::string& pass) { jobs_password_ = pass;}
   void set_child_pid(const std::string& pid) { remote_id_ = pid;}
   void set_child_try_no(unsigned int try_no) { task_try_num_ = try_no;}

private:
 	std::string task_path_;             // ECF_NAME = /aSuit/aFam/aTask
	std::string jobs_password_;         // ECF_PASS jobs password
	std::string remote_id_;             // ECF_RID process id of running job
 	unsigned int task_try_num_{1};         // ECF_TRYNO. The task try number. The number of times the job has been run
	std::string host_file_;             // ECF_HOSTFILE. File that lists the backup hosts, port numbers must match
	long timeout_;                      // ECF_TIMEOUT. Host file iteration time out
   long zombie_timeout_;               // ECF_ZOMBIE_TIMEOUT. Host file iteration time out for zombies, default same as ECF_TIMEOUT
	int  connect_timeout_{0};              // default 0, ECF_CONNECT_TIMEOUT, connection timeout
	bool denied_{false};                       // ECF_DENIED.If the server denies the communication, then the child command can be set to fail immediately
 	bool no_ecf_{false};                       // NO_ECF. if defined then abort cmd immediately. useful when test jobs stand-alone

	bool debug_{false};                        // For live debug, enabled by env variable ECF_CLIENT_DEBUG
	bool under_test_{false};                   // Used in testing client interface
	std::vector<std::pair<std::string,std::string> > env_; // For test allow env variable to be set on defs

	bool host_file_read_{false};               // to ensure we read host file only once
	std::vector<std::pair<std::string, std::string> > host_vec_; // The list of host:port pairs
	int  host_vec_index_{0};               // index into host_vec;

   mutable std::string passwd_;

#ifdef ECF_OPENSSL
 	ecf::Openssl ssl_;
#endif

	/// The option read from the command line.
 	friend class ClientOptions;

	// Allow testing to override the task path set in the environment. In this constructor
	friend class ClientInvoker;

private:

   void init();

   void read_environment_variables();  /// Get the standard environment variables

   void taskPath(const std::string& s) { task_path_ = s;}
   void set_jobs_password(const std::string& s) { jobs_password_ = s;}
   void set_connect_timeout(int t) { connect_timeout_ = t;}

   // Allow testing to add or update the environment in the Defs file
   // Each pair is ( variable name, variable value )
   void setEnv( const std::vector<std::pair<std::string,std::string> >& e) { env_= e;}

	bool parseHostsFile(std::string& errorMsg);
};

#endif
