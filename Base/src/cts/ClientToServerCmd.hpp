#ifndef CLIENT_TO_SERVER_CMD_HPP_
#define CLIENT_TO_SERVER_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Cmd
// Author      : Avi
// Revision    : $Revision: #143 $ 
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

#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/serialization/base_object.hpp>      // base class serialization
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/export.hpp>   // explicit code for exports (place last) , needed for BOOST_CLASS_EXPORT

#include "PrintStyle.hpp"
#include "Cmd.hpp"
#include "NodeFwd.hpp"
#include "NOrder.hpp"
#include "Zombie.hpp"
#include "Flag.hpp"
#include "Child.hpp"
#include "CheckPt.hpp"
#include "PreAllocatedReply.hpp"

#if defined(_AIX) && !defined(DEBUG)
// Required for MoveCmd for release mode of v11.1 compiler
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#endif

class AbstractServer;
class AbstractClientEnv;

///////////////////////////////////////////////////////////////////////////////////
// Client->Server cmd's
///////////////////////////////////////////////////////////////////////////////////
class ClientToServerCmd  {
public:
   virtual ~ClientToServerCmd();

   virtual std::ostream& print(std::ostream& os) const = 0;
   virtual bool equals(ClientToServerCmd* rhs) const = 0;

   /// Called by the _server_ to service the client depending on the Command
   /// The server will pass itself down via the AbstractServer
   /// The returned Server to Client  command is sent back to the client
   /// Uses template pattern, it first authenticates request and then calls
   /// doHandleRequest. This function can throw exceptions. std::runtime_error
   STC_Cmd_ptr handleRequest(AbstractServer*) const;

   /// Returns true if handleRequest is testable. Only used in TEST
   virtual bool handleRequestIsTestable() const { return true ;}

   /// How long in seconds the client to attempt to send request and get a reply back
   /// before the request fails. This timeout affects the wait for the outward request
   /// and inward reply.
   ///
   /// The timeout feature allow the client to fail gracefully in the case
   /// where the server has died/crashed. The timeout will ensure the socket is closed.
   /// allowing the server to be restarted without getting the address is use error.
   ///
   /// NOTE: We also have a timeout in ClientInvoker/ClientEnvironment, *that* is different,
   ///       as that applies to CHILD/task commands, and control how long we continue
   ///       to iterate over the hosts files
   virtual int timeout() const { return 60; }

   /// A command can be read only command or write only command
   /// A read only command will not change the state of the suites in the server
   /// A write only command can modify the statue of suite in the server
   /// Used by the server for authentication since only write only users are allowed to edit.
   virtual bool isWrite() const { return false; /* returning false means read only */ }

   /// This Must be called for client->server commands.As this is required
   /// for authentication. *However* task based commands have their own authentication
   /// mechanism, and don't need setup_user_authentification().
   virtual void setup_user_authentification() = 0;

   /// Allow control over connection to different servers/hosts if the main server is down
   /// i.e for a getCmd, we do not want to wait 24 hours, trying all the servers
   /// However for Task based commands like , init,abort,event, meter,complete we
   /// want this behaviour as it can alter Node tree state and thus affect dependent nodes
   virtual bool connect_to_different_servers() const { return false; }

   /// The show occurs on the client side
   virtual PrintStyle::Type_t show_style() const { return PrintStyle::NOTHING;}

   // Other commands
   virtual bool get_cmd() const { return false; }
   virtual bool task_cmd() const { return false; }
   virtual bool terminate_cmd() const { return false; }
   virtual bool group_cmd() const { return false; }
   virtual bool ping_cmd() const { return false;}
   virtual bool why_cmd( std::string& ) const { return false;}
   virtual bool show_cmd() const { return false ;}
   virtual bool delete_all_cmd() const { return false ;}

   // CLIENT side Parse and command construction, create can throw std::runtime_error for errors
   virtual const char* theArg() const = 0;  // used for argument parsing
   virtual void addOption(boost::program_options::options_description& desc) const = 0;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv) const = 0;
protected:
   ClientToServerCmd() {}

   /// called by handleRequest, part of the template pattern
   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const = 0;

   /// return true if authentication succeeds, false and STC_Cmd_ptr to return otherwise
   /// This function is called from doHandleRequest and hence is called
   /// from within the server. The default implementation will get the current
   /// user and authenticate with reference to the white list file
   virtual bool authenticate(AbstractServer*, STC_Cmd_ptr&) const = 0;

   /// Log the command. Must typically be done before call doHandleRequest(), in case of crash/exception
   /// In rare case allow override. (i.e for additional debug)
   /// called by handleRequest, part of the template pattern
   virtual void do_log() const;

   /// Some commands which cause a change in state, should force an immediate job submission.
   /// Providing the server is *NOT* shutdown
   static STC_Cmd_ptr doJobSubmission(AbstractServer* as);

   static void dumpVecArgs(const char* argOption, const std::vector<std::string>& args);

   /// Find the node otherwise throw std:::runtime_error
   node_ptr find_node(AbstractServer* as, const std::string& absNodepath) const;

   /// Find The node for edit, otherwise throw std:::runtime_error
   /// Will add the node edit history collection
   node_ptr find_node_for_edit(AbstractServer* as, const std::string& absNodepath) const;

   /// Find The node for edit, otherwise return a NULL pointer
   /// Will add the node edit history collection
   node_ptr find_node_for_edit_no_throw(AbstractServer* as, const std::string& absNodepath) const;

   /// finds the associated node and adds to edit history nodes
   void add_node_for_edit_history(AbstractServer* as, const std::string& absNodepath) const;
   void add_node_for_edit_history(node_ptr) const;

private:
   friend class GroupCTSCmd;
   friend class EditHistoryMgr;
   mutable std::vector<weak_node_ptr> edit_history_nodes_;  // NOT persisted

private:
   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive &ar, const unsigned int /*version*/) {}
};
BOOST_SERIALIZATION_ASSUME_ABSTRACT(ClientToServerCmd)

//=================================================================================
// Task Commands
// ================================================================================
class TaskCmd : public ClientToServerCmd {
protected:
   TaskCmd( const std::string& pathToSubmittable,
            const std::string& jobsPassword,
            const std::string& process_or_remote_id,
            int try_no)
   : submittable_(0),path_to_submittable_(pathToSubmittable),
     jobs_password_(jobsPassword),process_or_remote_id_(process_or_remote_id), try_no_(try_no){}
   TaskCmd() : submittable_(0),try_no_(0) {}
public:

   virtual bool isWrite() const { return true; }
   virtual int timeout() const { return 180; }  // ECFLOW-157 80 -> 180

   const std::string& path_to_node() const { return path_to_submittable_;}
   const std::string& jobs_password() const { return jobs_password_;}
   const std::string& process_or_remote_id() const { return process_or_remote_id_;}
   int try_no() const { return try_no_;}
   virtual ecf::Child::CmdType child_type() const = 0;

   virtual bool equals(ClientToServerCmd*) const;
   virtual bool task_cmd() const { return true; }
   virtual bool connect_to_different_servers() const { return true; }

protected:
   /// Overridden to do nothing since Task based commands don't need _user_
   /// based authentification
   virtual void setup_user_authentification(){}
   virtual bool authenticate(AbstractServer*, STC_Cmd_ptr&) const; /// Task have their own mechanism,can throw std::runtime_error
   Submittable* get_submittable(AbstractServer* as) const ; // can throw std::runtime_error

protected:
   mutable Submittable* submittable_; // stored during authentication and re-used handle request, not persisted, server side only

private:
   std::string path_to_submittable_;
   std::string jobs_password_;
   std::string process_or_remote_id_;
   int try_no_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< ClientToServerCmd >( *this );
      ar & path_to_submittable_;
      ar & jobs_password_;
      ar & process_or_remote_id_;
      ar & try_no_;
   }
};

class InitCmd : public TaskCmd {
public:
   InitCmd(const std::string& pathToTask,
            const std::string& jobsPassword,
            const std::string& process_or_remote_id,
            int try_no )
   : TaskCmd(pathToTask,jobsPassword,process_or_remote_id,try_no) {}

   InitCmd() : TaskCmd()  {}

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;
   virtual ecf::Child::CmdType child_type() const { return ecf::Child::INIT; }

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< TaskCmd >( *this );
   }
};

class CompleteCmd : public TaskCmd {
public:
   CompleteCmd(const std::string& pathToTask,
            const std::string& jobsPassword,
            const std::string& process_or_remote_id = "",
            int try_no  = 1)
   : TaskCmd(pathToTask,jobsPassword,process_or_remote_id,try_no) {}
   CompleteCmd() : TaskCmd() {}

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;
   virtual ecf::Child::CmdType child_type() const { return ecf::Child::COMPLETE; }

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< TaskCmd >( *this );
   }
};

/// A child command that evaluates a expression. If the expression is false.
/// Then client invoker will block.
class CtsWaitCmd : public TaskCmd {
public:
   CtsWaitCmd(const std::string& pathToTask,
            const std::string& jobsPassword,
            const std::string& process_or_remote_id,
            int try_no,
            const std::string& expression);
   CtsWaitCmd() : TaskCmd()  {}

   const std::string& expression() const { return expression_;}

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:

   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;
   virtual ecf::Child::CmdType child_type() const { return ecf::Child::WAIT; }

   std::string expression_;
   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< TaskCmd >( *this );
      ar & expression_;
   }
};

class AbortCmd : public TaskCmd {
public:
   AbortCmd(const std::string& pathToTask,
            const std::string& jobsPassword,
            const std::string& process_or_remote_id,
            int try_no = 1,
            const std::string& reason = "");
   AbortCmd() : TaskCmd() {}

   const std::string& reason() const {return reason_; }

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;
   virtual ecf::Child::CmdType child_type() const { return ecf::Child::ABORT; }

   std::string reason_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< TaskCmd >( *this );
      ar & reason_;
   }
};

class EventCmd : public TaskCmd {
public:
   EventCmd(const std::string& pathToTask,
            const std::string& jobsPassword,
            const std::string& process_or_remote_id,
            int try_no,
            const std::string& eventName )
   : TaskCmd(pathToTask,jobsPassword,process_or_remote_id,try_no), name_(eventName) {}
   EventCmd() : TaskCmd() {}

   const std::string& name() const { return name_; }

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;
   virtual ecf::Child::CmdType child_type() const { return ecf::Child::EVENT; }

private:
   std::string name_; // the events name

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< TaskCmd >( *this );
      ar & name_;
   }
};

class MeterCmd : public TaskCmd {
public:
   MeterCmd(const std::string& pathToTask,
            const std::string& jobsPassword,
            const std::string& process_or_remote_id,
            int try_no,
            const std::string& meterName,
            int meterValue)
   : TaskCmd(pathToTask,jobsPassword,process_or_remote_id,try_no), name_(meterName), value_(meterValue) {}
   MeterCmd() : TaskCmd(), value_(0) {}

   const std::string& name() const { return name_; }
   int value() const { return value_; }

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;
   virtual ecf::Child::CmdType child_type() const { return ecf::Child::METER; }

private:
   std::string name_;  // the meters name
   int value_;         // the meters value

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< TaskCmd >( *this );
      ar & name_;
      ar & value_;
   }
};

class LabelCmd : public TaskCmd {
public:
   LabelCmd(const std::string& pathToTask,
            const std::string& jobsPassword,
            const std::string& process_or_remote_id,
            int try_no,
            const std::string& name,
            const std::string& label)
   : TaskCmd(pathToTask,jobsPassword,process_or_remote_id,try_no), name_(name), label_(label) {}
   LabelCmd() : TaskCmd() {}

   const std::string& name() const { return name_; }
   const std::string& label() const { return label_;}

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void  create(   Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;
   virtual ecf::Child::CmdType child_type() const { return ecf::Child::LABEL; }

private:
   std::string name_;   // the label name
   std::string label_;  // a single label, or multi-line label

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< TaskCmd >( *this );
      ar & name_;
      ar & label_;
   }
};

//=================================================================================
// User Commands
// ================================================================================
class UserCmd : public ClientToServerCmd {
public:
   UserCmd(){}

   const std::string& user() const { return user_;}
   virtual void setup_user_authentification();

protected:

   virtual bool equals(ClientToServerCmd*) const;
   virtual bool authenticate(AbstractServer*, STC_Cmd_ptr&) const;

   /// Prompt the user for confirmation: If user responds with no, will exit client
   static void prompt_for_confirmation(const std::string& prompt);

   /// All user commands will be pre_fixed with "--" and post_fixed with :user.
   std::ostream& user_cmd(std::ostream& os, const std::string& the_cmd) const;

   static int time_out_for_load_sync_and_get();

   // The order is preserved during the split. Paths assumed to start with '/' char
   static void split_args_to_options_and_paths(
            const std::vector<std::string>& args,
            std::vector<std::string>& options,
            std::vector<std::string>& paths);


private:
   std::string user_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< ClientToServerCmd >( *this );
      ar & user_;
   }
};


// ========================================================================
// This Command should NEVER be changed
// This will allow new client to ask OLD server about its version
// ========================================================================
class ServerVersionCmd : public UserCmd {
public:
   ServerVersionCmd(){}

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;
   virtual const char* theArg() const;
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create(    Cmd_ptr& cmd,
                           boost::program_options::variables_map& vm,
                           AbstractClientEnv* clientEnv ) const;
private:
   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
   }
};


// This command is used to encapsulate all commands that are
// simple signals to the server. This helps to cut down on the
// number of global symbols used by boost serialisation.
// =========================================================================
// *** IMPORTANT ***: If any of these commands in the future need arguments,
// *** then ensure to place a DUMMY enum in its place.
// *** This will allow a *newer* development client to still send message to a older server.
// *** i.e like terminating the server
// *** IMPORTANT: For any new commands, must be added to the end.
// *** - STATS_RESET was introduced in release 4.0.5
// =========================================================================
class CtsCmd : public UserCmd {
public:
   enum Api { NO_CMD, RESTORE_DEFS_FROM_CHECKPT,
      RESTART_SERVER, SHUTDOWN_SERVER, HALT_SERVER, TERMINATE_SERVER,
      RELOAD_WHITE_LIST_FILE,
      FORCE_DEP_EVAL,
      PING, GET_ZOMBIES, STATS, SUITES,
      DEBUG_SERVER_ON, DEBUG_SERVER_OFF,
      SERVER_LOAD, STATS_RESET
     };

   CtsCmd(Api a) : api_(a) {}
   CtsCmd() : api_(NO_CMD) {}

   Api api() const { return api_;}

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual bool isWrite() const;
   virtual bool terminate_cmd() const { return api_ == TERMINATE_SERVER; }
   virtual bool ping_cmd() const { return api_ == PING; }
   virtual int timeout() const;

   virtual bool handleRequestIsTestable() const;

   virtual const char* theArg() const;
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

   Api api_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & api_;
   }
};

class CheckPtCmd : public UserCmd {
public:
   CheckPtCmd(ecf::CheckPt::Mode m, int interval,int checkpt_save_time_alarm)
   :  mode_(m), check_pt_interval_(interval),check_pt_save_time_alarm_(checkpt_save_time_alarm) {}
   CheckPtCmd() : mode_(ecf::CheckPt::UNDEFINED), check_pt_interval_(0),check_pt_save_time_alarm_(0) {}

   ecf::CheckPt::Mode mode() const { return mode_;}
   int check_pt_interval() const { return check_pt_interval_;}
   int check_pt_save_time_alarm() const { return check_pt_save_time_alarm_;}

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;
   virtual bool isWrite() const;
   virtual const char* theArg() const;
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

   ecf::CheckPt::Mode mode_;
   int check_pt_interval_;
   int check_pt_save_time_alarm_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & mode_;
      ar & check_pt_interval_;
      ar & check_pt_save_time_alarm_;
   }
};


// Client---(CSyncCmd::SYNC_FULL)---->Server-----(SSyncCmd)--->client:
// Client---(CSyncCmd::SYNC)--------->Server-----(SSyncCmd)--->client:
// Client---(CSyncCmd::NEWS)--------->Server-----(SNewsCmd)--->client:
class CSyncCmd : public UserCmd {
public:
   enum Api { NEWS, SYNC , SYNC_FULL};

   CSyncCmd(Api a, unsigned int client_handle,unsigned int client_state_change_no, unsigned int client_modify_change_no)
   : api_(a),
     client_handle_(client_handle),
     client_state_change_no_(client_state_change_no),
     client_modify_change_no_(client_modify_change_no) {}
   CSyncCmd(unsigned int client_handle)
   : api_(SYNC_FULL),
     client_handle_(client_handle),
     client_state_change_no_(0),
     client_modify_change_no_(0) {}
   CSyncCmd()
   : api_(SYNC),
     client_handle_(0),
     client_state_change_no_(0),
     client_modify_change_no_(0) {}

   Api api() const { return api_;}
   int client_state_change_no() const { return client_state_change_no_;}
   int client_modify_change_no() const { return client_modify_change_no_;}
   int client_handle() const { return client_handle_;}

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;
   virtual int timeout() const;

   virtual const char* theArg() const;
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:

   /// Custom handling of command logging to add additional debug on same line
   /// makes it easier to debug errors in syncing.
   virtual void do_log() const;

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

   Api api_;
   int client_handle_;
   int client_state_change_no_;
   int client_modify_change_no_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & api_;
      ar & client_handle_;
      ar & client_state_change_no_;
      ar & client_modify_change_no_;
   }
};

class ClientHandleCmd : public UserCmd {
public:
   enum Api { REGISTER, DROP, DROP_USER, ADD, REMOVE, AUTO_ADD , SUITES };

   ClientHandleCmd(Api api = AUTO_ADD)
   : api_(api),
     client_handle_(0),
     auto_add_new_suites_(false) {}

   ClientHandleCmd(const std::vector<std::string>& suites, bool add_add_new_suites)
   : api_(REGISTER),
     client_handle_(0),
     auto_add_new_suites_(add_add_new_suites),
     suites_(suites)  {}

   ClientHandleCmd(int client_handle)
   : api_(DROP),
     client_handle_(client_handle),
     auto_add_new_suites_(false) {}

   ClientHandleCmd(const std::string& drop_user)
    : api_(DROP_USER),
      client_handle_(0),
      auto_add_new_suites_(false),
      drop_user_(drop_user){}

   ClientHandleCmd(int client_handle, const std::vector<std::string>& suites, Api api)
   : api_(api),  // Must be ADD or REMOVE
     client_handle_(client_handle),
     auto_add_new_suites_(false),
     suites_(suites){}

   ClientHandleCmd(int client_handle, bool add_add_new_suites)
   : api_(AUTO_ADD),
     client_handle_(client_handle),
     auto_add_new_suites_(add_add_new_suites) {}

   Api api() const { return api_;}
   const std::string& drop_user() const { return drop_user_;}

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;
   virtual bool isWrite() const;

   virtual const char* theArg() const;
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

   Api api_;
   int client_handle_;
   bool auto_add_new_suites_;
   std::string drop_user_;
   std::vector<std::string> suites_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & api_;
      ar & client_handle_;
      ar & auto_add_new_suites_;
      ar & drop_user_;
      ar & suites_;
   }
};

// Collection of commands, that all take a abs node path as their only arg
// Reduce number of global symbols caused by boost serialisation
// Previously they were all separate commands
//
// Client---(CtsNodeCmd(GET))---->Server-----(DefsCmd | SNodeCmd )--->client:
// When doHandleRequest is called in the server it will return DefsCmd
// The DefsCmd is used to transport the node tree hierarchy to/from the server
//
// CHECK_JOB_GEN_ONLY command will traverse hierarchically from the given node path
// and force generation of jobs. (i.e independently of dependencies).
// This is used in *testing* only, so that we can compare/test/verify
// job generation with the old version.
// if absNodepath is empty we will generate jobs for all tasks
class CtsNodeCmd : public UserCmd {
public:
   enum Api { NO_CMD, JOB_GEN, CHECK_JOB_GEN_ONLY, GET, WHY, GET_STATE, MIGRATE };
   CtsNodeCmd(Api a, const std::string& absNodePath) : api_(a),absNodePath_(absNodePath) {}
   CtsNodeCmd(Api a) : api_(a) { assert(a != NO_CMD); }
   CtsNodeCmd() : api_(NO_CMD) {}

   Api api() const { return api_;}
   const std::string& absNodePath() const { return absNodePath_;}

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual PrintStyle::Type_t show_style() const;

   virtual int timeout() const;
   virtual bool isWrite() const;
   virtual bool handleRequestIsTestable() const { return !terminate_cmd();}
   virtual bool why_cmd( std::string& nodePath) const;
   virtual bool get_cmd() const { return api_ ==  GET; }

   virtual const char* theArg() const;
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

private:
   Api api_;
   std::string absNodePath_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & api_;
      ar & absNodePath_;
   }
};

// DELETE If paths_ empty will delete all suites (beware) else will delete the chosen nodes.
class PathsCmd : public UserCmd {
public:
   enum Api { NO_CMD,  DELETE, SUSPEND, RESUME, KILL, STATUS, CHECK, EDIT_HISTORY };

   PathsCmd(Api api,const std::vector<std::string>& paths, bool force = false)
   : api_(api),force_(force),paths_(paths){}
   PathsCmd(Api api,const std::string& absNodePath, bool force = false);
   PathsCmd(Api api)
   : api_(api), force_(false) { assert(api != NO_CMD); }
   PathsCmd()
   : api_(NO_CMD),force_(false) {}

   Api api() const { return api_; }
   const std::vector<std::string>& paths() const { return paths_;}
   bool force() const { return force_;}

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;
   virtual bool isWrite() const;
   virtual bool delete_all_cmd() const;

   virtual const char* theArg() const;
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

private:
   Api api_;
   bool force_;
   mutable std::vector<std::string> paths_; // mutable to allow swap to clear & reclaim memory, as soon as possible

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & api_;
      ar & force_;
      ar & paths_;
   }
};


/// The LogCmd is paired with SStringCmd
/// Client---(LogCmd)---->Server-----(SStringCmd)--->client:
/// When doHandleRequest is called in the server it will return SStringCmd
/// The SStringCmd is used to transport the log file contents to the client
class LogCmd : public UserCmd {
public:
   enum LogApi { GET, CLEAR, FLUSH, NEW , PATH};
   LogCmd(LogApi a, int get_last_n_lines = 0) : api_(a),get_last_n_lines_(get_last_n_lines) {}
   LogCmd(const std::string& path); // NEW
   LogCmd() : api_(LogCmd::GET),get_last_n_lines_(0) {}

   LogApi api() const { return api_;}
   int get_last_n_lines() const { return get_last_n_lines_;}
   const std::string& new_path() const { return new_path_;}

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

   LogApi api_;
   int get_last_n_lines_;
   std::string new_path_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & api_;
      ar & get_last_n_lines_;
      ar & new_path_;
   }
};

/// Simply writes the message to the log file
class LogMessageCmd : public UserCmd {
public:
   LogMessageCmd(const std::string& msg) : msg_(msg) {}
   LogMessageCmd() {}

   const std::string& msg() const { return msg_;}

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   std::string msg_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & msg_;
   }
};


// class Begin:  if suiteName is empty we will begin all suites
class BeginCmd : public UserCmd {
public:
   BeginCmd(const std::string& suiteName, bool force = false);
   BeginCmd() : force_(false) {}

   const std::string& suiteName() const { return suiteName_;}
   bool force() const { return force_;}

   virtual int timeout() const { return 180; }

   virtual bool isWrite() const { return true; }
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

   std::string suiteName_;
   bool        force_;      // reset begin status on suites & bypass checks, can create zombies, used in test only

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & suiteName_;
      ar & force_;
   }
};

class ZombieCmd : public UserCmd {
public:
   ZombieCmd(ecf::User::Action uc, const std::string& path, const std::string& process_id, const std::string& password)
   : user_action_(uc), path_(path), process_id_(process_id), password_(password) {}
   ZombieCmd(ecf::User::Action uc = ecf::User::BLOCK) : user_action_(uc) {}

   const std::string& path_to_task() const { return path_;}
   const std::string& process_or_remote_id() const { return process_id_;}
   const std::string& password() const { return password_;}

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const;
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

   ecf::User::Action user_action_;
   std::string path_;
   std::string process_id_;
   std::string password_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & user_action_;
      ar & path_;
      ar & process_id_;
      ar & password_;
   }
};

class RequeueNodeCmd : public UserCmd {
public:
   enum Option { NO_OPTION, ABORT, FORCE };

   RequeueNodeCmd(const std::vector<std::string>& paths, Option op = NO_OPTION)
   : paths_(paths), option_(op) {}

   RequeueNodeCmd(const std::string& absNodepath, Option op = NO_OPTION)
   : paths_(std::vector<std::string>(1,absNodepath)), option_(op) {}

   RequeueNodeCmd() : option_(NO_OPTION) {}

   const std::vector<std::string>& paths() const { return paths_;}
   Option option() const { return option_;}

   virtual bool isWrite() const { return true; }
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

private:
   mutable std::vector<std::string>  paths_;  // mutable to allow swap to clear & reclaim memory, as soon as possible
   Option                    option_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & paths_;
      ar & option_;
   }
};

class OrderNodeCmd : public UserCmd {
public:
   OrderNodeCmd(const std::string& absNodepath, NOrder::Order op)
   : absNodepath_(absNodepath), option_(op) {}
   OrderNodeCmd() : option_(NOrder::TOP) {}

   const std::string& absNodepath() const { return absNodepath_;}
   NOrder::Order option() const { return option_;}

   virtual bool isWrite() const { return true; }
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

   std::string   absNodepath_;
   NOrder::Order      option_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & absNodepath_;
      ar & option_;
   }
};


// The absNodepath must be provided
class RunNodeCmd : public UserCmd {
public:
   RunNodeCmd(const std::string& absNodepath, bool force, bool test = false)
   : paths_(std::vector<std::string>(1,absNodepath)), force_(force), test_(test) {}

   RunNodeCmd(const std::vector<std::string>& paths, bool force, bool test = false)
   : paths_(paths), force_(force), test_(test) {}

   RunNodeCmd() : force_(false), test_(false) {}

   const std::vector<std::string>& paths() const { return paths_;}
   bool force() const { return force_;}

   virtual bool isWrite() const { return true; }
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

private:
   mutable std::vector<std::string> paths_; // mutable to allow swap to clear & reclaim memory, as soon as possible
   bool        force_;
   bool        test_;   // only for test, hence we don't serialise this

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & paths_;
      ar & force_;
   }
};


// Does Nothing in the server, however allows client code to display the
// returned Defs in different showStyles
// This class has no need for persistence, i.e client side only
class ShowCmd : public UserCmd {
public:
   ShowCmd(PrintStyle::Type_t s = PrintStyle::DEFS) : style_(s) {}

   // returns the showStyle
   virtual bool show_cmd() const { return true ;}
   virtual PrintStyle::Type_t show_style() const { return style_;}

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   // The Show Cmd is processed on the client side,
   // Likewise the doHandleRequest does nothing,
   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

   PrintStyle::Type_t style_;

   // Persistence is still required since show command can be *USED* in a *GROUP* command
   // However its ONLY used on the client side, hence no need to serialise data members
   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
   }
};

// Will *load* the suites, into the server.
// Additionally the server will try to resolve extern's. The extern are references
// to Node, events, meters, limits, variables defined on another suite.
class LoadDefsCmd : public UserCmd {
public:
   LoadDefsCmd(const std::string& defs_filename, bool force = false);

   LoadDefsCmd(const defs_ptr& defs, bool force = false)
   : force_(force), defs_(defs) {}

   LoadDefsCmd() : force_(false) {}

   // Uses by equals only
   const defs_ptr& theDefs() const { return defs_; }

   virtual bool isWrite() const { return true; }
   virtual int timeout() const { return time_out_for_load_sync_and_get(); }
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
   static Cmd_ptr create(const std::string& defs_filename, bool force, bool check_only, AbstractClientEnv* clientEnv);

private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the command as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

   bool        force_;
   defs_ptr    defs_;
   std::string defs_filename_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & force_;
      ar & defs_;
      ar & defs_filename_;
   }
};

class ReplaceNodeCmd : public UserCmd {
public:
   ReplaceNodeCmd(const std::string& node_path, bool createNodesAsNeeded, defs_ptr defs, bool force );
   ReplaceNodeCmd(const std::string& node_path, bool createNodesAsNeeded, const std::string& path_to_defs, bool force );
   ReplaceNodeCmd() : createNodesAsNeeded_(false), force_(false) {}

   defs_ptr theDefs() const  { return clientDefs_; }
   const std::string& pathToNode() const { return pathToNode_; }
   const std::string& path_to_defs() const { return path_to_defs_;}
   bool createNodesAsNeeded() const { return createNodesAsNeeded_;}
   bool force() const { return force_;}

   virtual bool isWrite() const { return true; }
   virtual int timeout() const { return 300; }
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

   bool        createNodesAsNeeded_;
   bool        force_;
   std::string pathToNode_;
   std::string path_to_defs_; // Can be empty if defs loaded in memory via python api
   defs_ptr    clientDefs_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & createNodesAsNeeded_;
      ar & force_;
      ar & pathToNode_;
      ar & path_to_defs_;
      ar & clientDefs_;
   }
};

// Set the state on the affected node ONLY.
// If recursive is used set the state, on node and  _ALL_ nodes _beneath
// setRepeatToLastValue set, only make sense when used with recursive.
// stateOrEvent, string is one of:
// < unknown | suspended | complete | queued | submitted | active | aborted | clear | set >
class ForceCmd : public UserCmd {
public:
   ForceCmd(const std::vector<std::string>& paths,
            const std::string& stateOrEvent,
            bool recursive,
            bool setRepeatToLastValue)
   : paths_(paths), stateOrEvent_(stateOrEvent),
     recursive_(recursive), setRepeatToLastValue_(setRepeatToLastValue) {}
   ForceCmd(const std::string& path,
            const std::string& stateOrEvent,
            bool recursive,
            bool setRepeatToLastValue)
   : paths_(std::vector<std::string>(1,path)), stateOrEvent_(stateOrEvent),
     recursive_(recursive), setRepeatToLastValue_(setRepeatToLastValue) {}
   ForceCmd() : recursive_(false), setRepeatToLastValue_(false) {}

   // Uses by equals only
   const std::vector<std::string> paths() const { return paths_; }
   const std::string& stateOrEvent() const { return stateOrEvent_;}
   bool recursive() const { return recursive_;}
   bool setRepeatToLastValue() const { return setRepeatToLastValue_;}

   virtual bool isWrite() const { return true; }
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

private:
   mutable std::vector<std::string> paths_; // mutable to allow swap to clear & reclaim memory, as soon as possible
   std::string              stateOrEvent_;
   bool                     recursive_;
   bool                     setRepeatToLastValue_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & paths_;
      ar & stateOrEvent_;
      ar & recursive_;
      ar & setRepeatToLastValue_;
   }
};

// Free Dependencies
class FreeDepCmd : public UserCmd {
public:
   FreeDepCmd(const std::vector<std::string>& paths,
            bool trigger = true,
            bool all = false, // day, date, time, today, trigger, cron
            bool date = false,
            bool time =  false // includes time, day, date, today, cron
   )
   : paths_(paths), trigger_(trigger), all_(all), date_(date), time_(time) {}

   FreeDepCmd(const std::string& path,
            bool trigger = true,
            bool all = false, // day, date, time, today, trigger, cron
            bool date = false,
            bool time =  false // includes time, day, date, today, cron
   )
   : paths_(std::vector<std::string>(1,path)), trigger_(trigger), all_(all), date_(date), time_(time) {}

   FreeDepCmd() : trigger_(true), all_(false), date_(false), time_(false){}

   // Uses by equals only
   const std::vector<std::string>& paths() const { return paths_; }
   bool trigger() const { return trigger_;}
   bool all() const     { return all_;}
   bool date() const    { return date_;}
   bool time() const    { return time_;}

   virtual bool isWrite() const { return true; }
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

private:
   mutable std::vector<std::string> paths_; // mutable to allow swap to clear & reclaim memory, as soon as possible
   bool          trigger_;
   bool          all_;
   bool          date_;
   bool          time_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & paths_;
      ar & trigger_;
      ar & all_;
      ar & date_;
      ar & time_;
   }
};

class AlterCmd : public UserCmd {
public:
   enum Delete_attr_type  { DEL_VARIABLE, DEL_TIME, DEL_TODAY, DEL_DATE, DEL_DAY,
      DEL_CRON, DEL_EVENT, DEL_METER, DEL_LABEL,
      DEL_TRIGGER, DEL_COMPLETE, DEL_REPEAT, DEL_LIMIT, DEL_LIMIT_PATH,
      DEL_INLIMIT, DEL_ZOMBIE, DELETE_ATTR_ND };

   enum Change_attr_type  { VARIABLE, CLOCK_TYPE, CLOCK_DATE, CLOCK_GAIN,  EVENT, METER, LABEL,
      TRIGGER, COMPLETE, REPEAT, LIMIT_MAX, LIMIT_VAL, DEFSTATUS, CHANGE_ATTR_ND, CLOCK_SYNC };

   enum Add_attr_type  {  ADD_TIME, ADD_TODAY, ADD_DATE, ADD_DAY, ADD_ZOMBIE, ADD_VARIABLE, ADD_ATTR_ND };

   AlterCmd(const std::string& path, Add_attr_type  attr,  const std::string& name, const std::string& value = "" )
   : paths_(std::vector<std::string>(1,path)), name_(name), value_(value), add_attr_type_(attr),
     del_attr_type_(DELETE_ATTR_ND), change_attr_type_(CHANGE_ATTR_ND),flag_type_(ecf::Flag::NOT_SET), flag_(false) {}
   AlterCmd(const std::vector<std::string>& paths, Add_attr_type  attr,  const std::string& name, const std::string& value = "" )
   : paths_(paths), name_(name), value_(value), add_attr_type_(attr),
     del_attr_type_(DELETE_ATTR_ND), change_attr_type_(CHANGE_ATTR_ND),flag_type_(ecf::Flag::NOT_SET), flag_(false) {}

   AlterCmd(const std::string& path,  Delete_attr_type  del, const std::string& name = "" , const std::string& value = "")
   : paths_(std::vector<std::string>(1,path)), name_(name), value_(value), add_attr_type_(ADD_ATTR_ND),
     del_attr_type_(del), change_attr_type_(CHANGE_ATTR_ND),flag_type_(ecf::Flag::NOT_SET), flag_(false) {}
   AlterCmd(const std::vector<std::string>& paths,  Delete_attr_type  del, const std::string& name = "" , const std::string& value = "")
   : paths_(paths), name_(name), value_(value), add_attr_type_(ADD_ATTR_ND),
     del_attr_type_(del), change_attr_type_(CHANGE_ATTR_ND),flag_type_(ecf::Flag::NOT_SET), flag_(false) {}

   AlterCmd(const std::string& path, Change_attr_type  attr, const std::string& name, const std::string& value = "")
   : paths_(std::vector<std::string>(1,path)), name_(name), value_(value), add_attr_type_(ADD_ATTR_ND),
     del_attr_type_(DELETE_ATTR_ND), change_attr_type_(attr),flag_type_(ecf::Flag::NOT_SET), flag_(false) {}
   AlterCmd(const std::vector<std::string>& paths, Change_attr_type  attr, const std::string& name, const std::string& value = "")
   : paths_(paths), name_(name), value_(value), add_attr_type_(ADD_ATTR_ND),
     del_attr_type_(DELETE_ATTR_ND), change_attr_type_(attr),flag_type_(ecf::Flag::NOT_SET), flag_(false) {}

   AlterCmd(const std::string& path, ecf::Flag::Type ft,  bool flag)
   : paths_(std::vector<std::string>(1,path)), add_attr_type_(ADD_ATTR_ND),
     del_attr_type_(DELETE_ATTR_ND), change_attr_type_(CHANGE_ATTR_ND),flag_type_(ft), flag_(flag) {}
   AlterCmd(const std::vector<std::string>& paths, ecf::Flag::Type ft,  bool flag)
   : paths_(paths), add_attr_type_(ADD_ATTR_ND),
     del_attr_type_(DELETE_ATTR_ND), change_attr_type_(CHANGE_ATTR_ND),flag_type_(ft), flag_(flag) {}

   AlterCmd()
   : add_attr_type_(ADD_ATTR_ND), del_attr_type_(DELETE_ATTR_ND),
     change_attr_type_(CHANGE_ATTR_ND),flag_type_(ecf::Flag::NOT_SET), flag_(false) {}

   // Uses by equals only
   const std::vector<std::string>& paths() const { return paths_; }
   const std::string& name() const { return name_; }
   const std::string& value() const { return value_; }
   Delete_attr_type delete_attr_type() const { return del_attr_type_;}
   Change_attr_type change_attr_type() const { return change_attr_type_;}
   Add_attr_type add_attr_type() const { return add_attr_type_;}
   ecf::Flag::Type flag_type() const { return flag_type_;}
   bool flag() const { return flag_;}

   virtual bool isWrite() const { return true; }
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;
   STC_Cmd_ptr alter_server_state(AbstractServer*) const;

   void createAdd(    Cmd_ptr& cmd,       std::vector<std::string>& options,       std::vector<std::string>& paths) const;
   void createDelete( Cmd_ptr& cmd, const std::vector<std::string>& options, const std::vector<std::string>& paths) const;
   void createChange( Cmd_ptr& cmd,       std::vector<std::string>& options,       std::vector<std::string>& paths) const;
   void create_flag(  Cmd_ptr& cmd, const std::vector<std::string>& options, const std::vector<std::string>& paths, bool flag) const;

private:
   mutable std::vector<std::string> paths_; // mutable to allow swap to clear & reclaim memory, as soon as possible
   std::string              name_;
   std::string              value_;
   Add_attr_type            add_attr_type_;
   Delete_attr_type         del_attr_type_;
   Change_attr_type         change_attr_type_;
   ecf::Flag::Type          flag_type_;
   bool                     flag_; // true means set false means clear

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & paths_;
      ar & name_;
      ar & value_;
      ar & add_attr_type_;
      ar & del_attr_type_;
      ar & change_attr_type_;
      ar & flag_type_;
      ar & flag_;
   }
};

//================================================================================
// Paired with SStringCmd
// Client---(CFileCmd)---->Server-----(SStringCmd)--->client:
//================================================================================
class CFileCmd : public UserCmd {
public:
   enum File_t { ECF, JOB, JOBOUT, MANUAL, KILL, STAT };
   CFileCmd(const std::string& pathToNode, File_t file, size_t max_lines)
   : file_(file),  pathToNode_(pathToNode), max_lines_(max_lines) {}
   CFileCmd(const std::string& pathToNode, const std::string& file_type, const std::string& max_lines);

   CFileCmd() : file_(ECF),max_lines_(0) {}

   // Uses by equals only
   const std::string& pathToNode() const { return pathToNode_; }
   File_t fileType() const { return file_;}
   size_t max_lines() const { return max_lines_;}

   static std::vector<CFileCmd::File_t>  fileTypesVec();
   static std::string toString(File_t);

   virtual bool handleRequestIsTestable() const { return false ;}
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

   File_t        file_;
   std::string   pathToNode_;
   size_t        max_lines_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & file_;
      ar & pathToNode_;
      ar & max_lines_;
   }
};

//================================================================================
// Paired with SStringCmd
// Client---(EditScriptCmd)---->Server-----(SStringCmd)--->client:
//================================================================================
class EditScriptCmd : public UserCmd {
public:
   enum EditType { EDIT, PREPROCESS, SUBMIT,  PREPROCESS_USER_FILE, SUBMIT_USER_FILE  };
   EditScriptCmd(const std::string& path_to_node,EditType et) // EDIT or PREPROCESS
   :  edit_type_(et), path_to_node_(path_to_node),
      alias_(false),run_(false)
   {}

   EditScriptCmd(const std::string& path_to_node, const NameValueVec& user_variables)
   :  edit_type_(SUBMIT), path_to_node_(path_to_node), user_variables_(user_variables),
      alias_(false),run_(false)
   {}

   EditScriptCmd(const std::string& path_to_node, const std::vector<std::string>& user_file_contents)
   :  edit_type_(PREPROCESS_USER_FILE), path_to_node_(path_to_node), user_file_contents_(user_file_contents),
      alias_(false),run_(false)
   {}

   EditScriptCmd( const std::string& path_to_node,
                  const NameValueVec& user_variables,
                  const std::vector<std::string>& user_file_contents,
                  bool create_alias,
                  bool run_alias
   )
   :  edit_type_(SUBMIT_USER_FILE), path_to_node_(path_to_node), user_file_contents_(user_file_contents),user_variables_(user_variables),
      alias_(create_alias),run_(run_alias)
   {}

   EditScriptCmd() : edit_type_(EDIT),alias_(false),run_(false) {}

   // Uses by equals only
   const std::string& path_to_node() const { return path_to_node_; }
   EditType edit_type() const { return edit_type_;}
   bool alias() const { return alias_;}
   bool run() const { return run_;}

   virtual bool handleRequestIsTestable() const { return false ;}
   virtual bool isWrite() const;
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

   EditType      edit_type_;
   std::string   path_to_node_;
   mutable std::vector<std::string>  user_file_contents_;
   NameValueVec user_variables_;
   bool alias_;
   bool run_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & edit_type_;
      ar & path_to_node_;
      ar & user_file_contents_;
      ar & user_variables_;
      ar & alias_;
      ar & run_;
   }
};

class PlugCmd : public UserCmd {
public:
   PlugCmd(const std::string& source, const std::string& dest) : source_(source), dest_(dest) {}
   PlugCmd() {}

   // Uses by equals only
   const std::string& source() const { return source_; }
   const std::string& dest() const { return dest_; }

   virtual int timeout() const { return 120; }
   virtual bool handleRequestIsTestable() const { return false ;}
   virtual bool isWrite() const { return true; }
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

private:
   std::string source_;
   std::string dest_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & source_;
      ar & dest_;
   }
};

class MoveCmd : public UserCmd {
public:
   MoveCmd(const std::pair<std::string,std::string>& host_port, Node* src, const std::string& dest);
   MoveCmd();
   virtual ~MoveCmd();

   Node* source() const;
   const std::string& dest() const { return dest_; }

   virtual bool handleRequestIsTestable() const { return false ;}
   virtual bool isWrite() const { return true; }
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

   bool check_source() const;

private:
   mutable Suite*  sourceSuite_; // only one is set, gets round un-registered class exception
   mutable Family* sourceFamily_;// only one is set, gets round un-registered class exception
   mutable Task*   sourceTask_;  // only one is set, gets round un-registered class exception
   std::string src_host_;
   std::string src_port_;
   std::string src_path_;
   std::string dest_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & sourceSuite_;  // only one is serialised
      ar & sourceFamily_; // only one is serialised
      ar & sourceTask_;   // only one is serialised
      ar & src_host_;
      ar & src_port_;
      ar & src_path_;
      ar & dest_;
   }
};

// The group command allows a series of commands to be be executed:
//
// Client---(GroupCTSCmd)---->Server-----(GroupSTCCmd | StcCmd(OK) | Error )--->client:
//
// If client->server contains GetDefs cmd and log file commands, then a group
// command will be created for returning to the client
//
// If group command contains multiple [ CtsNodeCmd(GET) | LogCmd --get ] commands then
// all the results are returned back to the client, HOWEVER when client calls
// Cmd::defs() | Cmd::get_string() only the first data is returned.
//
class GroupCTSCmd : public UserCmd {
public:
   GroupCTSCmd(const std::string& list_of_commands,AbstractClientEnv* clientEnv);
   GroupCTSCmd(){}

   virtual bool isWrite() const;
   virtual PrintStyle::Type_t show_style() const;
   virtual bool get_cmd() const;
   virtual bool task_cmd() const;
   virtual bool terminate_cmd() const;
   virtual bool why_cmd( std::string& ) const;
   virtual bool group_cmd() const { return true; }

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ClientToServerCmd*) const;

   void addChild(Cmd_ptr childCmd);
   const std::vector<Cmd_ptr>& cmdVec() const { return cmdVec_;}

   virtual const char* theArg() const { return arg();}
   virtual void addOption(boost::program_options::options_description& desc) const;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   virtual void setup_user_authentification();
   virtual bool authenticate(AbstractServer*, STC_Cmd_ptr&) const;
   virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const;

   std::vector<Cmd_ptr> cmdVec_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< UserCmd >( *this );
      ar & cmdVec_;
   }
};

std::ostream& operator<<(std::ostream& os, const ServerVersionCmd&);
std::ostream& operator<<(std::ostream& os, const CtsCmd&);
std::ostream& operator<<(std::ostream& os, const CtsNodeCmd&);
std::ostream& operator<<(std::ostream& os, const PathsCmd&);
std::ostream& operator<<(std::ostream& os, const CheckPtCmd&);
std::ostream& operator<<(std::ostream& os, const LoadDefsCmd&);
std::ostream& operator<<(std::ostream& os, const LogCmd&);
std::ostream& operator<<(std::ostream& os, const LogMessageCmd&);
std::ostream& operator<<(std::ostream& os, const BeginCmd&);
std::ostream& operator<<(std::ostream& os, const ZombieCmd&);
std::ostream& operator<<(std::ostream& os, const InitCmd&);
std::ostream& operator<<(std::ostream& os, const EventCmd&);
std::ostream& operator<<(std::ostream& os, const MeterCmd&);
std::ostream& operator<<(std::ostream& os, const LabelCmd&);
std::ostream& operator<<(std::ostream& os, const CompleteCmd&);
std::ostream& operator<<(std::ostream& os, const CtsWaitCmd&);
std::ostream& operator<<(std::ostream& os, const AbortCmd&);
std::ostream& operator<<(std::ostream& os, const RequeueNodeCmd&);
std::ostream& operator<<(std::ostream& os, const OrderNodeCmd&);
std::ostream& operator<<(std::ostream& os, const RunNodeCmd&);
std::ostream& operator<<(std::ostream& os, const ReplaceNodeCmd&);
std::ostream& operator<<(std::ostream& os, const ForceCmd&);
std::ostream& operator<<(std::ostream& os, const FreeDepCmd&);
std::ostream& operator<<(std::ostream& os, const CFileCmd&);
std::ostream& operator<<(std::ostream& os, const PlugCmd&);
std::ostream& operator<<(std::ostream& os, const AlterCmd&);
std::ostream& operator<<(std::ostream& os, const MoveCmd&);
std::ostream& operator<<(std::ostream& os, const GroupCTSCmd&);

BOOST_CLASS_EXPORT_KEY(ServerVersionCmd)
BOOST_CLASS_EXPORT_KEY(CtsCmd)
BOOST_CLASS_EXPORT_KEY(CSyncCmd)
BOOST_CLASS_EXPORT_KEY(ClientHandleCmd)
BOOST_CLASS_EXPORT_KEY(CtsNodeCmd)
BOOST_CLASS_EXPORT_KEY(PathsCmd)
BOOST_CLASS_EXPORT_KEY(CheckPtCmd)
BOOST_CLASS_EXPORT_KEY(LoadDefsCmd)
BOOST_CLASS_EXPORT_KEY(LogCmd)
BOOST_CLASS_EXPORT_KEY(LogMessageCmd)
BOOST_CLASS_EXPORT_KEY(BeginCmd)
BOOST_CLASS_EXPORT_KEY(ZombieCmd)
BOOST_CLASS_EXPORT_KEY(InitCmd)
BOOST_CLASS_EXPORT_KEY(EventCmd)
BOOST_CLASS_EXPORT_KEY(MeterCmd)
BOOST_CLASS_EXPORT_KEY(LabelCmd)
BOOST_CLASS_EXPORT_KEY(AbortCmd)
BOOST_CLASS_EXPORT_KEY(CtsWaitCmd)
BOOST_CLASS_EXPORT_KEY(CompleteCmd)
BOOST_CLASS_EXPORT_KEY(RequeueNodeCmd)
BOOST_CLASS_EXPORT_KEY(OrderNodeCmd)
BOOST_CLASS_EXPORT_KEY(RunNodeCmd)
BOOST_CLASS_EXPORT_KEY(ReplaceNodeCmd)
BOOST_CLASS_EXPORT_KEY(ForceCmd)
BOOST_CLASS_EXPORT_KEY(FreeDepCmd)
BOOST_CLASS_EXPORT_KEY(CFileCmd)
BOOST_CLASS_EXPORT_KEY(EditScriptCmd)
BOOST_CLASS_EXPORT_KEY(PlugCmd)
BOOST_CLASS_EXPORT_KEY(AlterCmd)
BOOST_CLASS_EXPORT_KEY(MoveCmd)
BOOST_CLASS_EXPORT_KEY(GroupCTSCmd)
BOOST_CLASS_EXPORT_KEY(ShowCmd)
#endif
