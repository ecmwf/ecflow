#ifndef CLIENT_TO_SERVER_CMD_HPP_
#define CLIENT_TO_SERVER_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Cmd
// Author      : Avi
// Revision    : $Revision: #143 $ 
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

#include <boost/program_options.hpp>

#include "PrintStyle.hpp"
#include "Cmd.hpp"
#include "NodeFwd.hpp"
#include "NOrder.hpp"
#include "Zombie.hpp"
#include "Flag.hpp"
#include "Child.hpp"
#include "CheckPt.hpp"
#include "PreAllocatedReply.hpp"
#include "Serialization.hpp"

class AbstractServer;
class AbstractClientEnv;
class GroupCTSCmd;

///////////////////////////////////////////////////////////////////////////////////
// Client->Server cmd's
///////////////////////////////////////////////////////////////////////////////////
class ClientToServerCmd  {
public:
   virtual ~ClientToServerCmd();

   /// The host where the client was called
   const std::string& hostname() const { return cl_host_;}

   /// The second print is for use by EditHistoryMgr when we have commands that take multiple paths
   /// The EditHistoryMgr records what command was applied to each node. However when logging the
   /// the command we do not want logs all the paths, for each node.(performance bottleneck
   /// when dealing with thousands of paths)
   virtual std::ostream& print(std::ostream& os) const = 0;
   virtual std::ostream& print(std::ostream& os, const std::string& path) const { return print(os); }

   /// Print the command without trailing <user>@<host>. Used by Group command, avoids duplicate user@host for each child command
   virtual std::ostream& print_only(std::ostream& os ) const { return print(os); }

   virtual bool equals(ClientToServerCmd* rhs) const;

   /// Called by the _server_ to service the client depending on the Command
   /// The server will pass itself down via the AbstractServer
   /// The returned Server to Client  command is sent back to the client
   /// Uses template pattern, it first authenticates request and then calls
   /// doHandleRequest. This function can throw exceptions. std::runtime_error
   STC_Cmd_ptr handleRequest(AbstractServer*) const;

   /// After handleRequest() has run, this function can be used reclaim memory
   virtual void cleanup() {}

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
   /// A write only command can modify the state of suite in the server
   /// Used by the server for authentication since only write only users are allowed to edit.
   virtual bool isWrite() const { return false; /* returning false means read only */ }

   /// Some commands modify the server but do not affect defs. i.e reload white list file,password file
   /// Other like ClientHandleCmd make edits to defs(well kind off) but are read only.(i.e anyone can call them)
   /// This return true for those commands that affect the defs, that we need sync on the client side.
   virtual bool cmd_updates_defs() const { return isWrite(); }

   /// This Must be called for client->server commands.As this is required
   /// for authentication. *However* task based commands have their own authentication
   /// mechanism, and don't need setup_user_authentification().
   virtual void setup_user_authentification(const std::string& user, const std::string& passwd) = 0; // Used by PlugCmd
   virtual void setup_user_authentification(AbstractClientEnv&) = 0; // set user and passwd
   virtual void setup_user_authentification() = 0;                   // if user empty setup.

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

   // used by group_cmd to postfix syncCmd on all user commands that modify defs
   virtual void set_client_handle(int client_handle) {} // used by group_cmd
   virtual void set_group_cmd(const GroupCTSCmd*) {}

   // CLIENT side Parse and command construction, create can throw std::runtime_error for errors
   virtual const char* theArg() const = 0;  // used for argument parsing
   virtual void addOption(boost::program_options::options_description& desc) const = 0;
   virtual void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv) const = 0;
protected:
   ClientToServerCmd();

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
   /// If logging fails set late flag to warn users ECFLOW-536
   virtual void do_log(AbstractServer*) const;

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
   void add_node_path_for_edit_history(const std::string& absNodepath) const;

private:
   friend class GroupCTSCmd;
   friend class EditHistoryMgr;
   mutable std::vector<weak_node_ptr> edit_history_nodes_;       // NOT persisted
   mutable std::vector<std::string>   edit_history_node_paths_;  // NOT persisted, used when deleting

   std::string cl_host_; // The host where the client was called

private:
   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar( CEREAL_NVP(cl_host_) );
   }
};

//=================================================================================
// Task Commands
// ================================================================================
class TaskCmd : public ClientToServerCmd {
protected:
   TaskCmd( const std::string& pathToSubmittable,
            const std::string& jobsPassword,
            const std::string& process_or_remote_id,
            int try_no)
   : submittable_(nullptr),
     path_to_submittable_(pathToSubmittable),
     jobs_password_(jobsPassword),process_or_remote_id_(process_or_remote_id), try_no_(try_no){assert(!hostname().empty());}

   TaskCmd() = default;

public:

   bool isWrite() const override { return true; }
   int timeout() const override { return 190; }  // ECFLOW-157 80 -> 190

   const std::string& path_to_node() const { return path_to_submittable_;}
   const std::string& jobs_password() const { return jobs_password_;}
   const std::string& process_or_remote_id() const { return process_or_remote_id_;}
   int try_no() const { return try_no_;}
   virtual ecf::Child::CmdType child_type() const = 0;

   bool equals(ClientToServerCmd*) const override;
   bool task_cmd() const override { return true; }
   bool connect_to_different_servers() const override { return true; }

   bool password_missmatch() const { return password_missmatch_;}
   bool pid_missmatch() const { return pid_missmatch_;}

protected:
   /// Overridden to do nothing since Task based commands don't need _user_ based authentication
   void setup_user_authentification(const std::string& user, const std::string& passwd) override{}
   void setup_user_authentification(AbstractClientEnv&) override{}
   void setup_user_authentification() override{}

   bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override; /// Task have their own mechanism,can throw std::runtime_error
   Submittable* get_submittable(AbstractServer* as) const ; // can throw std::runtime_error

protected:
   mutable Submittable* submittable_{nullptr}; // stored during authentication and re-used handle request, not persisted, server side only

private:
   mutable bool password_missmatch_{false}; // stored during authentication and re-used handle request, not persisted, server side only
   mutable bool pid_missmatch_{false};      // stored during authentication and re-used handle request, not persisted, server side only

private:
   std::string path_to_submittable_;
   std::string jobs_password_;
   std::string process_or_remote_id_;
   int try_no_{0};

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< ClientToServerCmd >( this ),
          CEREAL_NVP(path_to_submittable_),
          CEREAL_NVP(jobs_password_),
          CEREAL_NVP(process_or_remote_id_),
          CEREAL_NVP(try_no_));
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

   std::ostream& print(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   ecf::Child::CmdType child_type() const override { return ecf::Child::INIT; }

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< TaskCmd >( this ));
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

   std::ostream& print(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   ecf::Child::CmdType child_type() const override { return ecf::Child::COMPLETE; }

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< TaskCmd >( this ));
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

   std::ostream& print(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:

   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   ecf::Child::CmdType child_type() const override { return ecf::Child::WAIT; }

   std::string expression_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< TaskCmd >( this ),
         CEREAL_NVP(expression_));
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

   std::ostream& print(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   ecf::Child::CmdType child_type() const override { return ecf::Child::ABORT; }

   std::string reason_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< TaskCmd >( this ),
      CEREAL_NVP(reason_));
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

   std::ostream& print(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   ecf::Child::CmdType child_type() const override { return ecf::Child::EVENT; }

private:
   std::string name_; // the events name

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< TaskCmd >( this ),
      CEREAL_NVP(name_));
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
   MeterCmd() : TaskCmd() {}

   const std::string& name() const { return name_; }
   int value() const { return value_; }

   std::ostream& print(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   ecf::Child::CmdType child_type() const override { return ecf::Child::METER; }

private:
   std::string name_;  // the meters name
   int value_{0};         // the meters value

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< TaskCmd >( this ),
      CEREAL_NVP(name_),
      CEREAL_NVP(value_));
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

   std::ostream& print(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void  create(   Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   ecf::Child::CmdType child_type() const override { return ecf::Child::LABEL; }

private:
   std::string name_;   // the label name
   std::string label_;  // a single label, or multi-line label

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< TaskCmd >( this ),
      CEREAL_NVP(name_),
      CEREAL_NVP(label_));
   }
};

class QueueAttr;
class QueueCmd : public TaskCmd {
public:
   QueueCmd(const std::string& pathToTask,
            const std::string& jobsPassword,
            const std::string& process_or_remote_id,
            int try_no,
            const std::string& queueName,
            const std::string& action,
            const std::string& step = "",
            const std::string& path_to_node_with_queue = "") // if empty search for queue up node tree
   : TaskCmd(pathToTask,jobsPassword,process_or_remote_id,try_no),
     name_(queueName),action_(action),step_(step),path_to_node_with_queue_(path_to_node_with_queue) {}
   QueueCmd() : TaskCmd() {}

   const std::string& name() const { return name_; }
   const std::string& action() const { return action_; }
   const std::string& step() const { return step_; }
   const std::string& path_to_node_with_queue() const { return path_to_node_with_queue_; }

   std::ostream& print(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   ecf::Child::CmdType child_type() const override { return ecf::Child::QUEUE; }

   std::string handle_queue(QueueAttr& queue_attr) const;

private:
   std::string name_;                     // the queue name
   std::string action_;                   // [ active | aborted | complete | no_of_aborted ]
   std::string step_;                     // will be empty when action is [ active | no_of_aborted]
   std::string path_to_node_with_queue_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< TaskCmd >( this ),
         CEREAL_NVP(name_),
         CEREAL_NVP(action_),
         CEREAL_NVP(step_),
         CEREAL_NVP(path_to_node_with_queue_));
   }
};

//=================================================================================
// User Commands
// ================================================================================
class UserCmd : public ClientToServerCmd {
public:
   UserCmd()= default;

   const std::string& user() const { return user_;}
   const std::string& passwd() const { return pswd_;}

   void setup_user_authentification(const std::string& user, const std::string& passwd) override;
   void setup_user_authentification(AbstractClientEnv&) override;
   void setup_user_authentification() override;

protected:

   bool equals(ClientToServerCmd*) const override;
   bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
   bool do_authenticate(AbstractServer* as, STC_Cmd_ptr&, const std::string& path) const;
   bool do_authenticate(AbstractServer* as, STC_Cmd_ptr&, const std::vector<std::string>& paths) const;

   /// Prompt the user for confirmation: If user responds with no, will exit client
   static void prompt_for_confirmation(const std::string& prompt);

   /// All user commands will be pre_fixed with "--" and post_fixed with :user@host
   std::ostream& user_cmd(std::ostream& os, const std::string& the_cmd) const;


   static int time_out_for_load_sync_and_get();

   // The order is preserved during the split. Paths assumed to start with '/' char
   static void split_args_to_options_and_paths(
            const std::vector<std::string>& args,
            std::vector<std::string>& options,
            std::vector<std::string>& paths,
            bool treat_colon_in_path_as_path = false);

private:
   std::string user_;
   std::string pswd_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< ClientToServerCmd >( this ),
         CEREAL_NVP(user_));
      CEREAL_OPTIONAL_NVP(ar, pswd_, [this](){return !pswd_.empty(); }); // conditionally save
   }
};

// ========================================================================
// This Command should NEVER be changed
// This will allow new client to ask OLD server about its version
// ========================================================================
class ServerVersionCmd : public UserCmd {
public:
   ServerVersionCmd()= default;

   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;
   const char* theArg() const override;
   void addOption(boost::program_options::options_description& desc) const override;
   void create(    Cmd_ptr& cmd,
                           boost::program_options::variables_map& vm,
                           AbstractClientEnv* clientEnv ) const override;
private:
   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ));
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
// *** IMPORTANT: For any new commands, must be added to the end, for each major release
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
      SERVER_LOAD, STATS_RESET,
      RELOAD_PASSWD_FILE,
      STATS_SERVER
     };

   explicit CtsCmd(Api a) : api_(a) {}
   CtsCmd()= default;

   Api api() const { return api_;}

   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   bool isWrite() const override;
   bool cmd_updates_defs() const  override;
   bool terminate_cmd() const override { return api_ == TERMINATE_SERVER; }
   bool ping_cmd() const override { return api_ == PING; }
   int timeout() const override;

   bool handleRequestIsTestable() const override;

   const char* theArg() const override;
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

   Api api_{NO_CMD};

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(api_));
   }
};

class CheckPtCmd : public UserCmd {
public:
   CheckPtCmd(ecf::CheckPt::Mode m, int interval,int checkpt_save_time_alarm)
   :  mode_(m), check_pt_interval_(interval),check_pt_save_time_alarm_(checkpt_save_time_alarm) {}
   CheckPtCmd() = default;

   ecf::CheckPt::Mode mode() const { return mode_;}
   int check_pt_interval() const { return check_pt_interval_;}
   int check_pt_save_time_alarm() const { return check_pt_save_time_alarm_;}

   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;
   bool isWrite() const override;
   const char* theArg() const override;
   void addOption(boost::program_options::options_description& desc) const override;
   void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

private:
   ecf::CheckPt::Mode mode_{ecf::CheckPt::UNDEFINED};
   int check_pt_interval_{0};
   int check_pt_save_time_alarm_{0};

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(mode_),
         CEREAL_NVP(check_pt_interval_),
         CEREAL_NVP(check_pt_save_time_alarm_));
   }
};


// Client---(CSyncCmd::SYNC_FULL)---->Server-----(SSyncCmd)--->client:
// Client---(CSyncCmd::SYNC)--------->Server-----(SSyncCmd)--->client:
// Client---(CSyncCmd::SYNC_CLOCK)--->Server-----(SSyncCmd)--->client:
// Client---(CSyncCmd::NEWS)--------->Server-----(SNewsCmd)--->client:
class CSyncCmd : public UserCmd {
public:
   enum Api { NEWS, SYNC, SYNC_FULL, SYNC_CLOCK};

   CSyncCmd(Api a, unsigned int client_handle,unsigned int client_state_change_no, unsigned int client_modify_change_no)
   : api_(a),
     client_handle_(client_handle),
     client_state_change_no_(client_state_change_no),
     client_modify_change_no_(client_modify_change_no) {}
   explicit CSyncCmd(unsigned int client_handle)
   : api_(SYNC_FULL),
     client_handle_(client_handle)
     {}
   CSyncCmd()= default;

   Api api() const { return api_;}
   int client_state_change_no() const { return client_state_change_no_;}
   int client_modify_change_no() const { return client_modify_change_no_;}
   int client_handle() const { return client_handle_;}

   void set_client_handle(int client_handle) override { client_handle_ = client_handle;} // used by group_cmd
   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;
   int timeout() const override;

   const char* theArg() const override;
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:

   /// Custom handling of command logging to add additional debug on same line
   /// makes it easier to debug errors in syncing.
   void do_log(AbstractServer*) const override;

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

   Api api_{SYNC};
   int client_handle_{0};
   int client_state_change_no_{0};
   int client_modify_change_no_{0};

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(api_),
         CEREAL_NVP(client_handle_),
         CEREAL_NVP(client_state_change_no_),
         CEREAL_NVP(client_modify_change_no_));
   }
};

class ClientHandleCmd : public UserCmd {
public:
   enum Api { REGISTER, DROP, DROP_USER, ADD, REMOVE, AUTO_ADD , SUITES };

   explicit ClientHandleCmd(Api api = AUTO_ADD)
   : api_(api) {}

   ClientHandleCmd(int client_handle,const std::vector<std::string>& suites, bool add_add_new_suites)
   : api_(REGISTER),
     client_handle_(client_handle),
     auto_add_new_suites_(add_add_new_suites),
     suites_(suites)
      {}

   explicit ClientHandleCmd(int client_handle)
   : api_(DROP),
     client_handle_(client_handle)
     {}

   explicit ClientHandleCmd(const std::string& drop_user)
    : api_(DROP_USER),
      drop_user_(drop_user)
      {}

   ClientHandleCmd(int client_handle, const std::vector<std::string>& suites, Api api)
   : api_(api),  // Must be ADD or REMOVE
     client_handle_(client_handle),
     suites_(suites)
     {}

   ClientHandleCmd(int client_handle, bool add_add_new_suites)
   : api_(AUTO_ADD),
     client_handle_(client_handle),
     auto_add_new_suites_(add_add_new_suites)
     {}

   Api api() const { return api_;}
   const std::string& drop_user() const { return drop_user_;}

   bool cmd_updates_defs() const override;
   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override;
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;

   // called in the server
   virtual void set_group_cmd(const GroupCTSCmd* cmd) { group_cmd_ = cmd;}

private:
   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

   Api api_;
   int client_handle_{0};
   bool auto_add_new_suites_{false};
   std::string drop_user_;
   std::vector<std::string> suites_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(api_),
         CEREAL_NVP(client_handle_),
         CEREAL_NVP(auto_add_new_suites_),
         CEREAL_NVP(drop_user_),
         CEREAL_NVP(suites_));
   }

private:
   const GroupCTSCmd* group_cmd_ = nullptr; // not persisted only used in server
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
   explicit CtsNodeCmd(Api a) : api_(a) { assert(a != NO_CMD); }
   CtsNodeCmd()= default;

   Api api() const { return api_;}
   const std::string& absNodePath() const { return absNodePath_;}

   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   PrintStyle::Type_t show_style() const override;

   int timeout() const override;
   bool isWrite() const override;
   bool handleRequestIsTestable() const override { return !terminate_cmd();}
   bool why_cmd( std::string& nodePath) const override;
   bool get_cmd() const override { return api_ ==  GET; }

   const char* theArg() const override;
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;

private:
   Api api_{NO_CMD};
   std::string absNodePath_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(api_),
         CEREAL_NVP(absNodePath_));
   }
};

// DELETE If paths_ empty will delete all suites (beware) else will delete the chosen nodes.
class DeleteCmd : public UserCmd {
public:
   DeleteCmd(const std::vector<std::string>& paths, bool force = false)
      : force_(force),paths_(paths),group_cmd_(nullptr){}
   DeleteCmd(const std::string& absNodePath, bool force = false);
   DeleteCmd()  {};

   const std::vector<std::string>& paths() const { return paths_;}
   bool force() const { return force_;}

   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   std::ostream& print(std::ostream& os, const std::string& path) const override;

   bool equals(ClientToServerCmd*) const override;
   bool isWrite() const override { return true;}

   const char* theArg() const override;
   void addOption(boost::program_options::options_description& desc) const override;
   void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;

   // called in the server
   virtual void set_group_cmd(const GroupCTSCmd* cmd) { group_cmd_ = cmd;}

   static void check_for_active_or_submitted_tasks(AbstractServer* as,node_ptr theNodeToDelete);

private:
   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
   void cleanup() override { std::vector<std::string>().swap(paths_);} /// run in the server, after handlerequest

private:
   bool force_{false};
   std::vector<std::string> paths_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(force_),
         CEREAL_NVP(paths_));
   }
private:
   const GroupCTSCmd* group_cmd_{nullptr}; // not persisted only used in server
};

// DELETE If paths_ empty will delete all suites (beware) else will delete the chosen nodes.
class PathsCmd : public UserCmd {
public:
   enum Api { NO_CMD, SUSPEND, RESUME, KILL, STATUS, CHECK, EDIT_HISTORY, ARCHIVE, RESTORE };

   PathsCmd(Api api,const std::vector<std::string>& paths, bool force = false)
      : api_(api),force_(force),paths_(paths){}
   PathsCmd(Api api,const std::string& absNodePath, bool force = false);
   explicit PathsCmd(Api api)
      : api_(api) { assert(api != NO_CMD); }
   PathsCmd() = default;

   Api api() const { return api_; }
   const std::vector<std::string>& paths() const { return paths_;}
   bool force() const { return force_;}

   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   std::ostream& print(std::ostream& os, const std::string& path) const override;

   bool equals(ClientToServerCmd*) const override;
   bool isWrite() const override;

   const char* theArg() const override;
   void addOption(boost::program_options::options_description& desc) const override;
   void create(    Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
   void cleanup() override { std::vector<std::string>().swap(paths_);} /// run in the server, after handlerequest

   std::ostream& my_print(std::ostream& os, const std::vector<std::string>& paths) const;
   std::ostream& my_print_only(std::ostream& os, const std::vector<std::string>& paths) const;

private:
   Api api_{NO_CMD};
   bool force_{false};
   std::vector<std::string> paths_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(api_),
         CEREAL_NVP(force_),
         CEREAL_NVP(paths_));
   }
};


/// The LogCmd is paired with SStringCmd
/// Client---(LogCmd)---->Server-----(SStringCmd)--->client:
/// When doHandleRequest is called in the server it will return SStringCmd
/// The SStringCmd is used to transport the log file contents to the client
class LogCmd : public UserCmd {
public:
   enum LogApi { GET, CLEAR, FLUSH, NEW , PATH };
   LogCmd(LogApi a, int get_last_n_lines = 0); // for zero we take default from log. Avoid adding dependency on log.hpp
   explicit LogCmd(const std::string& path); // NEW
   LogCmd();

   LogApi api() const { return api_;}
   int get_last_n_lines() const { return get_last_n_lines_;}
   const std::string& new_path() const { return new_path_;}

   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   bool isWrite() const override;
   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

   LogApi api_{LogCmd::GET};
   int get_last_n_lines_; // default to 100 -> ECFLOW-174
   std::string new_path_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(api_),
         CEREAL_NVP(get_last_n_lines_),
         CEREAL_NVP(new_path_));
   }
};

/// Simply writes the message to the log file
class LogMessageCmd : public UserCmd {
public:
   explicit LogMessageCmd(const std::string& msg) : msg_(msg) {}
   LogMessageCmd() = default;

   const std::string& msg() const { return msg_;}

   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   std::string msg_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(msg_));
   }
};


// class Begin:  if suiteName is empty we will begin all suites
class BeginCmd : public UserCmd {
public:
   BeginCmd(const std::string& suiteName, bool force = false);
   BeginCmd()= default;

   const std::string& suiteName() const { return suiteName_;}
   bool force() const { return force_;}

   int timeout() const override { return 80; }

   bool isWrite() const override { return true; }
   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

private:
   std::string suiteName_;
   bool        force_{false};      // reset begin status on suites & bypass checks, can create zombies, used in test only

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(suiteName_),
         CEREAL_NVP(force_));
   }
};

class ZombieCmd : public UserCmd {
public:
   ZombieCmd(ecf::User::Action uc, const std::vector<std::string>& paths, const std::string& process_id, const std::string& password)
   : user_action_(uc), process_id_(process_id), password_(password),paths_(paths) {}
   explicit ZombieCmd(ecf::User::Action uc = ecf::User::BLOCK) : user_action_(uc) {}

   const std::vector<std::string>& paths() const { return paths_;}
   const std::string& process_or_remote_id() const { return process_id_;}
   const std::string& password() const { return password_;}

   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override;
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;

private:
   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   void cleanup() override { std::vector<std::string>().swap(paths_);} /// run in the server, after handlerequest

   ecf::User::Action user_action_;
   std::string process_id_;         // should be empty for multiple paths and when using CLI
   std::string password_;           // should be empty for multiple paths and when using CLI
   std::vector<std::string> paths_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(user_action_),
         CEREAL_NVP(process_id_),
         CEREAL_NVP(password_),
         CEREAL_NVP(paths_));
   }
};

class RequeueNodeCmd : public UserCmd {
public:
   enum Option { NO_OPTION, ABORT, FORCE };

   RequeueNodeCmd(const std::vector<std::string>& paths, Option op = NO_OPTION)
   : paths_(paths), option_(op) {}

   RequeueNodeCmd(const std::string& absNodepath, Option op = NO_OPTION)
   : paths_(std::vector<std::string>(1,absNodepath)), option_(op) {}

   RequeueNodeCmd()= default;

   const std::vector<std::string>& paths() const { return paths_;}
   Option option() const { return option_;}

   bool isWrite() const override { return true; }
   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   std::ostream& print(std::ostream& os, const std::string& path) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
   void cleanup() override { std::vector<std::string>().swap(paths_);} /// run in the server, after doHandleRequest

private:
   mutable std::vector<std::string>  paths_;  // mutable to allow swap to clear & reclaim memory, as soon as possible
   Option                    option_{NO_OPTION};

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(paths_),
         CEREAL_NVP(option_));
   }
};

class OrderNodeCmd : public UserCmd {
public:
   OrderNodeCmd(const std::string& absNodepath, NOrder::Order op)
   : absNodepath_(absNodepath), option_(op) {}
   OrderNodeCmd()= default;

   const std::string& absNodepath() const { return absNodepath_;}
   NOrder::Order option() const { return option_;}

   bool isWrite() const override { return true; }
   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;

private:
   std::string   absNodepath_;
   NOrder::Order      option_{NOrder::TOP};

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(absNodepath_),
         CEREAL_NVP(option_));
   }
};


// The absNodepath must be provided
class RunNodeCmd : public UserCmd {
public:
   RunNodeCmd(const std::string& absNodepath, bool force, bool test = false)
   : paths_(std::vector<std::string>(1,absNodepath)), force_(force), test_(test) {}

   RunNodeCmd(const std::vector<std::string>& paths, bool force, bool test = false)
   : paths_(paths), force_(force), test_(test) {}

   RunNodeCmd()= default;

   const std::vector<std::string>& paths() const { return paths_;}
   bool force() const { return force_;}

   bool isWrite() const override { return true; }
   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   std::ostream& print(std::ostream& os, const std::string& path) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
   void cleanup() override { std::vector<std::string>().swap(paths_);} /// run in the server, after doHandleRequest

private:
   std::vector<std::string> paths_;
   bool        force_{false};
   bool        test_{false};   // only for test, hence we don't serialise this

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(paths_),
         CEREAL_NVP(force_));
   }
};


// Does Nothing in the server, however allows client code to display the
// returned Defs in different showStyles
// This class has no need for persistence, i.e client side only
class ShowCmd : public UserCmd {
public:
   explicit ShowCmd(PrintStyle::Type_t s = PrintStyle::DEFS) : style_(s) {}

   // returns the showStyle
   bool show_cmd() const override { return true ;}
   PrintStyle::Type_t show_style() const override { return style_;}

   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   // The Show Cmd is processed on the client side,
   // Likewise the doHandleRequest does nothing,
   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

   PrintStyle::Type_t style_;

   // Persistence is still required since show command can be *USED* in a *GROUP* command
   // However its ONLY used on the client side, hence no need to serialise data members
   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ));
   }
};

// Will *load* the suites, into the server.
// Additionally the server will try to resolve extern's. The extern are references
// to Node, events, meters, limits, variables defined on another suite.
class LoadDefsCmd : public UserCmd {
public:
   LoadDefsCmd(const defs_ptr& defs, bool force = false);
   LoadDefsCmd(const std::string& defs_filename,bool force = false,bool check_only = false/* not persisted */,bool print = false/* not persisted */,
               const std::vector<std::pair<std::string,std::string> >& client_env = std::vector<std::pair<std::string,std::string> >());
   LoadDefsCmd()= default;

   // Uses by equals only
   const std::string& defs_as_string() const { return defs_; }

   bool isWrite() const override { return true; }
   int timeout() const override { return time_out_for_load_sync_and_get(); }
   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
   static Cmd_ptr create(const std::string& defs_filename,bool force,bool check_only,bool print,AbstractClientEnv* clientEnv);

private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the command as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

   bool        force_{false};
   std::string defs_;
   std::string defs_filename_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(force_),
         CEREAL_NVP(defs_),
         CEREAL_NVP(defs_filename_));
   }
};

class ReplaceNodeCmd : public UserCmd {
public:
   ReplaceNodeCmd(const std::string& node_path, bool createNodesAsNeeded, defs_ptr client_defs, bool force );
   ReplaceNodeCmd(const std::string& node_path, bool createNodesAsNeeded, const std::string& path_to_defs, bool force);
   ReplaceNodeCmd()= default;

   const std::string& the_client_defs() const  { return clientDefs_; }
   const std::string& pathToNode() const { return pathToNode_; }
   const std::string& path_to_defs() const { return path_to_defs_;}
   bool createNodesAsNeeded() const { return createNodesAsNeeded_;}
   bool force() const { return force_;}

   bool isWrite() const override { return true; }
   int timeout() const override { return 300; }
   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;

   //void set_client_env(const std::vector<std::pair<std::string,std::string> >& env ) { client_env_ = env;} // only used in test

private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
   void cleanup() override { std::string().swap(clientDefs_);} /// run in the server, after command send to client

   bool        createNodesAsNeeded_{false};
   bool        force_{false};
   std::string pathToNode_;
   std::string path_to_defs_; // Can be empty if defs loaded in memory via python api
   std::string clientDefs_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(createNodesAsNeeded_),
         CEREAL_NVP(force_),
         CEREAL_NVP(pathToNode_),
         CEREAL_NVP(path_to_defs_),
         CEREAL_NVP(clientDefs_));
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
   ForceCmd()= default;

   // Uses by equals only
   const std::vector<std::string> paths() const { return paths_; }
   const std::string& stateOrEvent() const { return stateOrEvent_;}
   bool recursive() const { return recursive_;}
   bool setRepeatToLastValue() const { return setRepeatToLastValue_;}

   bool isWrite() const override { return true; }
   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   std::ostream& print(std::ostream& os, const std::string& path) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
   void cleanup() override { std::vector<std::string>().swap(paths_);} /// run in the server, after doHandleRequest

private:
   std::vector<std::string> paths_;
   std::string              stateOrEvent_;
   bool                     recursive_{false};
   bool                     setRepeatToLastValue_{false};

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(paths_),
         CEREAL_NVP(stateOrEvent_),
         CEREAL_NVP(recursive_),
         CEREAL_NVP(setRepeatToLastValue_));
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

   FreeDepCmd() = default;

   // Uses by equals only
   const std::vector<std::string>& paths() const { return paths_; }
   bool trigger() const { return trigger_;}
   bool all() const     { return all_;}
   bool date() const    { return date_;}
   bool time() const    { return time_;}

   bool isWrite() const override { return true; }
   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   std::ostream& print(std::ostream& os, const std::string& path) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
   void cleanup() override { std::vector<std::string>().swap(paths_);} /// run in the server, after doHandleRequest

private:
   std::vector<std::string> paths_;
   bool          trigger_{true};
   bool          all_{false};
   bool          date_{false};
   bool          time_{false};

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(paths_),
         CEREAL_NVP(trigger_),
         CEREAL_NVP(all_),
         CEREAL_NVP(date_),
         CEREAL_NVP(time_));
   }
};

class AlterCmd : public UserCmd {
public:
   enum Delete_attr_type  { DEL_VARIABLE, DEL_TIME, DEL_TODAY, DEL_DATE, DEL_DAY,
      DEL_CRON, DEL_EVENT, DEL_METER, DEL_LABEL,
      DEL_TRIGGER, DEL_COMPLETE, DEL_REPEAT, DEL_LIMIT, DEL_LIMIT_PATH,
      DEL_INLIMIT, DEL_ZOMBIE, DELETE_ATTR_ND, DEL_LATE, DEL_QUEUE, DEL_GENERIC };

   enum Change_attr_type  { VARIABLE, CLOCK_TYPE, CLOCK_DATE, CLOCK_GAIN,  EVENT, METER, LABEL,
      TRIGGER, COMPLETE, REPEAT, LIMIT_MAX, LIMIT_VAL, DEFSTATUS, CHANGE_ATTR_ND, CLOCK_SYNC, LATE };

   enum Add_attr_type  {  ADD_TIME, ADD_TODAY, ADD_DATE, ADD_DAY, ADD_ZOMBIE, ADD_VARIABLE, ADD_ATTR_ND, ADD_LATE, ADD_LIMIT, ADD_INLIMIT, ADD_LABEL };

   // Python
   AlterCmd(const std::vector<std::string>& paths,
             const std::string& alterType, /* one of [ add | change | delete | set_flag | clear_flag ] */
             const std::string& attrType,
             const std::string& name,
             const std::string& value);
   // add
   AlterCmd(const std::string& path, Add_attr_type  attr,  const std::string& name, const std::string& value = "" )
   : paths_(std::vector<std::string>(1,path)), name_(name), value_(value), add_attr_type_(attr) {}
   AlterCmd(const std::vector<std::string>& paths, Add_attr_type  attr,  const std::string& name, const std::string& value = "" )
   : paths_(paths), name_(name), value_(value), add_attr_type_(attr) {}
   // delete
   AlterCmd(const std::string& path,  Delete_attr_type  del, const std::string& name = "" , const std::string& value = "")
   : paths_(std::vector<std::string>(1,path)), name_(name), value_(value), 
     del_attr_type_(del) {}
   AlterCmd(const std::vector<std::string>& paths,  Delete_attr_type  del, const std::string& name = "" , const std::string& value = "")
   : paths_(paths), name_(name), value_(value), 
     del_attr_type_(del) {}
   // change
   AlterCmd(const std::string& path, Change_attr_type  attr, const std::string& name, const std::string& value = "")
   : paths_(std::vector<std::string>(1,path)), name_(name), value_(value),  change_attr_type_(attr) {}
   AlterCmd(const std::vector<std::string>& paths, Change_attr_type  attr, const std::string& name, const std::string& value = "")
   : paths_(paths), name_(name), value_(value),  change_attr_type_(attr) {}
   // flag
   AlterCmd(const std::string& path, ecf::Flag::Type ft,  bool flag)
   : paths_(std::vector<std::string>(1,path)), flag_type_(ft), flag_(flag) {}
   AlterCmd(const std::vector<std::string>& paths, ecf::Flag::Type ft,  bool flag)
   : paths_(paths), flag_type_(ft), flag_(flag) {}
   // sort
   AlterCmd(const std::string& path, const std::string& name,const std::string& value)
    : paths_(std::vector<std::string>(1,path)), name_(name),value_(value)  {}
   AlterCmd(const std::vector<std::string>& paths, const std::string& name,const std::string& value)
    : paths_(paths), name_(name),value_(value)  {}

   AlterCmd()= default;

   // Uses by equals only
   const std::vector<std::string>& paths() const { return paths_; }
   const std::string& name() const { return name_; }
   const std::string& value() const { return value_; }
   Delete_attr_type delete_attr_type() const { return del_attr_type_;}
   Change_attr_type change_attr_type() const { return change_attr_type_;}
   Add_attr_type add_attr_type() const { return add_attr_type_;}
   ecf::Flag::Type flag_type() const { return flag_type_;}
   bool flag() const { return flag_;}

   bool isWrite() const override { return true; }
   std::ostream& print(std::ostream& os) const override;
   std::ostream& print(std::ostream& os, const std::string& path) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   STC_Cmd_ptr alter_server_state(AbstractServer*) const;
   bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
   void cleanup() override { std::vector<std::string>().swap(paths_);} /// run in the server, after doHandleRequest

   std::ostream& my_print(std::ostream& os, const std::vector<std::string>& paths) const;

   Add_attr_type get_add_attr_type(const std::string&) const;
   void createAdd(    Cmd_ptr& cmd,       std::vector<std::string>& options,       std::vector<std::string>& paths) const;
   void extract_name_and_value_for_add(Add_attr_type ,std::string& name,std::string& value, std::vector<std::string>& options, std::vector<std::string>& paths) const;
   void check_for_add(Add_attr_type ,const std::string& name, const std::string& value) const;

   Delete_attr_type get_delete_attr_type(const std::string&) const;
   void createDelete( Cmd_ptr& cmd, const std::vector<std::string>& options, const std::vector<std::string>& paths) const;
   void extract_name_and_value_for_delete(Delete_attr_type,std::string& name,std::string& value,const std::vector<std::string>& options,const std::vector<std::string>& paths) const;
   void check_for_delete(Delete_attr_type,const std::string& name, const std::string& value) const;

   Change_attr_type get_change_attr_type(const std::string&) const;
   void createChange( Cmd_ptr& cmd,       std::vector<std::string>& options,       std::vector<std::string>& paths) const;
   void extract_name_and_value_for_change(Change_attr_type,std::string& name,std::string& value,std::vector<std::string>& options,std::vector<std::string>& paths) const;
   void check_for_change(Change_attr_type,const std::string& name, const std::string& value) const;

   ecf::Flag::Type  get_flag_type(const std::string&) const;
   void create_flag(  Cmd_ptr& cmd, const std::vector<std::string>& options, const std::vector<std::string>& paths, bool flag) const;

   void check_sort_attr_type(const std::string&) const;
   void create_sort_attributes(Cmd_ptr& cmd,const std::vector<std::string>& options,const std::vector<std::string>& paths ) const;

   void alter_and_attr_type(std::string& alter_type,std::string& attr_type ) const;

private:
   std::vector<std::string> paths_;
   std::string              name_;
   std::string              value_;
   Add_attr_type            add_attr_type_{ADD_ATTR_ND};
   Delete_attr_type         del_attr_type_{DELETE_ATTR_ND};
   Change_attr_type         change_attr_type_{CHANGE_ATTR_ND};
   ecf::Flag::Type          flag_type_{ecf::Flag::NOT_SET};
   bool                     flag_{false}; // true means set false means clear

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(paths_),
         CEREAL_NVP(name_),
         CEREAL_NVP(value_),
         CEREAL_NVP(add_attr_type_),
         CEREAL_NVP(del_attr_type_),
         CEREAL_NVP(change_attr_type_),
         CEREAL_NVP(flag_type_),
         CEREAL_NVP(flag_));
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
   CFileCmd() = default;

   // Uses by equals only
   const std::string& pathToNode() const { return pathToNode_; }
   File_t fileType() const { return file_;}
   size_t max_lines() const { return max_lines_;}

   static std::vector<CFileCmd::File_t>  fileTypesVec();
   static std::string toString(File_t);

   bool handleRequestIsTestable() const override { return false ;}
   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;

   File_t        file_{ECF};
   std::string   pathToNode_;
   size_t        max_lines_{0};

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(file_),
         CEREAL_NVP(pathToNode_),
         CEREAL_NVP(max_lines_));
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
   :  edit_type_(et), path_to_node_(path_to_node)
   {}

   EditScriptCmd(const std::string& path_to_node, const NameValueVec& user_variables)
   :  edit_type_(SUBMIT), path_to_node_(path_to_node), user_variables_(user_variables)
   {}

   EditScriptCmd(const std::string& path_to_node, const std::vector<std::string>& user_file_contents)
   :  edit_type_(PREPROCESS_USER_FILE), path_to_node_(path_to_node), user_file_contents_(user_file_contents)
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

   EditScriptCmd()= default;

   // Uses by equals only
   const std::string& path_to_node() const { return path_to_node_; }
   EditType edit_type() const { return edit_type_;}
   bool alias() const { return alias_;}
   bool run() const { return run_;}

   bool handleRequestIsTestable() const override { return false ;}
   bool isWrite() const override;
   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
   bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
   void cleanup() override{ std::vector<std::string>().swap(user_file_contents_);} /// run in the server, after doHandleRequest

private:
   EditType      edit_type_{EDIT};
   std::string   path_to_node_;
   mutable std::vector<std::string>  user_file_contents_;
   NameValueVec user_variables_;
   bool alias_{false};
   bool run_{false};

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(edit_type_),
         CEREAL_NVP(path_to_node_),
         CEREAL_NVP(user_file_contents_),
         CEREAL_NVP(user_variables_),
         CEREAL_NVP(alias_),
         CEREAL_NVP(run_));
   }
};

class PlugCmd : public UserCmd {
public:
   PlugCmd(const std::string& source, const std::string& dest) : source_(source), dest_(dest) {}
   PlugCmd() = default;

   // Uses by equals only
   const std::string& source() const { return source_; }
   const std::string& dest() const { return dest_; }

   int timeout() const override { return 120; }
   bool handleRequestIsTestable() const override { return false ;}
   bool isWrite() const override { return true; }
   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

private:
   std::string source_;
   std::string dest_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(source_),
         CEREAL_NVP(dest_));
   }
};

class MoveCmd : public UserCmd {
public:
   MoveCmd(const std::pair<std::string,std::string>& host_port, Node* src, const std::string& dest);
   MoveCmd();
   ~MoveCmd() override;

   Node* source() const;
   const std::string& src_node() const { return src_node_;}
   const std::string& dest() const { return dest_; }

   bool handleRequestIsTestable() const override { return false ;}
   bool isWrite() const override { return true; }

   std::ostream& print(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

   bool check_source() const;

private:
   std::string src_node_;
   std::string src_host_;
   std::string src_port_;
   std::string src_path_;
   std::string dest_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(src_node_),
         CEREAL_NVP(src_host_),
         CEREAL_NVP(src_port_),
         CEREAL_NVP(src_path_),
         CEREAL_NVP(dest_));
   }
};

class QueryCmd : public UserCmd {
public:
   QueryCmd(const std::string& query_type,
            const std::string& path_to_attribute,
            const std::string& attribute,
            const std::string& path_to_task )
   : query_type_(query_type),path_to_attribute_(path_to_attribute),attribute_(attribute),path_to_task_(path_to_task){}
   QueryCmd() : UserCmd(){}
   ~QueryCmd() override;

   const std::string& query_type() const { return query_type_; }
   const std::string& path_to_attribute() const { return path_to_attribute_; }
   const std::string& attribute() const { return attribute_; }
   const std::string& path_to_task() const { return  path_to_task_;}

   std::ostream& print(std::ostream& os) const override;
   std::ostream& print_only(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void  create(   Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   bool handleRequestIsTestable() const override { return false;}
   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

private:
   std::string query_type_;        // [ state | dstate | event | meter | trigger ]
   std::string path_to_attribute_;
   std::string attribute_;         // [ event_name | meter_name | variable_name | trigger expression] empty for state and dstate
   std::string path_to_task_;      // The task the invoked this command, needed for logging

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(query_type_),
         CEREAL_NVP(path_to_attribute_),
         CEREAL_NVP(attribute_),
         CEREAL_NVP(path_to_task_));
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
   GroupCTSCmd(Cmd_ptr cmd) : cli_(false) { addChild(cmd);}
   GroupCTSCmd()= default;

   bool isWrite() const override;
   bool cmd_updates_defs() const override;

   PrintStyle::Type_t show_style() const override;
   bool get_cmd() const override;
   bool task_cmd() const override;
   bool terminate_cmd() const override;
   bool why_cmd( std::string& ) const override;
   bool group_cmd() const override { return true; }

   void set_client_handle(int client_handle) const; // used in group sync with client register

   std::ostream& print(std::ostream& os) const override;
   bool equals(ClientToServerCmd*) const override;

   void addChild(Cmd_ptr childCmd);
   const std::vector<Cmd_ptr>& cmdVec() const { return cmdVec_;}

   const char* theArg() const override { return arg();}
   void addOption(boost::program_options::options_description& desc) const override;
   void create( 	Cmd_ptr& cmd,
            boost::program_options::variables_map& vm,
            AbstractClientEnv* clientEnv ) const override;
private:
   static const char* arg();  // used for argument parsing
   static const char* desc(); // The description of the argument as provided to user

   void setup_user_authentification(const std::string& user, const std::string& passwd) override;
   void setup_user_authentification(AbstractClientEnv&) override;
   void setup_user_authentification() override;

   bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
   STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

private:
   bool cli_ = true;
   std::vector<Cmd_ptr> cmdVec_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class< UserCmd >( this ),
         CEREAL_NVP(cli_),
         CEREAL_NVP(cmdVec_));
   }
};

std::ostream& operator<<(std::ostream& os, const ServerVersionCmd&);
std::ostream& operator<<(std::ostream& os, const CtsCmd&);
std::ostream& operator<<(std::ostream& os, const CtsNodeCmd&);
std::ostream& operator<<(std::ostream& os, const DeleteCmd&);
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
std::ostream& operator<<(std::ostream& os, const QueryCmd&);

#endif
