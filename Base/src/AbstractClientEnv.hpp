#ifndef ABSTRACT_CLIENT_ENV__HPP_
#define ABSTRACT_CLIENT_ENV__HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This class is used to represent the client environment to the cmds
//
// This abstraction ONLY covers aspects that are required, to _CREATE_
// the client command/request that is to be sent to the server
// The key theme is that we can define a new client to server command
// in one place. i.e it allows the functionality, command line arguments,
// parsing of the arguments, can all be done in the command class itself
// If the command requires any additional arguments from the client environment
// they should be added to this class, and the implementation in the derived class
//============================================================================

#include <string>
#include <vector>

class AbstractClientEnv {
protected:
 	AbstractClientEnv() = default;
public:
 	AbstractClientEnv(const  AbstractClientEnv&) = delete;
   const AbstractClientEnv& operator=(const AbstractClientEnv&) = delete;

 	virtual ~AbstractClientEnv() = default;

 	void set_cli(bool f) { cli_ = f ;}
 	bool get_cli() const { return cli_;}

 	/// For all tasks/child based commands we require taskPath and password and optional Remote ID
 	/// When the jobs use a queueing system the remote id (ECF_RID) is used to
 	/// aid killing of jobs.(typically zombies)
 	virtual bool checkTaskPathAndPassword(std::string& errorMsg) const = 0;
	virtual const std::string& task_path() const  = 0;
 	virtual int task_try_no() const = 0;
	virtual const std::string& jobs_password() const = 0;
	virtual const std::string& process_or_remote_id() const = 0;

	/// For test allow env variable to be set on defs, i.e. allow us to inject ECF_CLIENT to defs
	virtual const std::vector<std::pair<std::string,std::string> >& env() const = 0;

	/// Allow ping to set the host:port
	virtual void set_host_port(const std::string& host, const std::string& port) = 0;

   /// return the current host
	virtual const std::string& host() const = 0;

   /// returns the port number
	virtual const std::string& port() const = 0;

	/// debug client, when environment variable ECF_CLIENT_DEBUG is set.
	virtual bool debug() const = 0 ;

   /// Returns the user password read from the password file.
	/// This value is cached, so we only read passwd file once
	/// When the password is read in we crypt the password, using user name as a salt.
   virtual const std::string& get_custom_user_password(const std::string& user) const = 0;
   virtual const std::string& get_user_password(const std::string& user) const = 0;
   virtual void clear_user_password() = 0; // force password check again

   // returns a user specified user name. When this is used a password must be provided
   virtual const std::string& get_user_name() const = 0;
   virtual void set_user_name(const std::string&) = 0;

	/// Some commands work on construction. to avoid this under test. Call set_test
	/// i.e Command like CtsCmd::SERVER_LOAD can be client side only, in which case
	/// when testing the client interface we want to avoid opening the log file.
	virtual void set_test() = 0;
	virtual bool under_test() const = 0;

private:
   bool cli_{false};  // Command Line Interface. Controls whether output written to standard out, and argument checking
};
#endif
