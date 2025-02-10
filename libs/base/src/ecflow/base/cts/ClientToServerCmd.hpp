/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_ClientToServerCmd_HPP
#define ecflow_base_cts_ClientToServerCmd_HPP

#include <boost/program_options.hpp>

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/base/Authentication.hpp"
#include "ecflow/base/Authorisation.hpp"
#include "ecflow/base/Cmd.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/NodeFwd.hpp"

class AbstractServer;
class AbstractClientEnv;
class GroupCTSCmd;

///////////////////////////////////////////////////////////////////////////////////
// Client->Server cmd's
///////////////////////////////////////////////////////////////////////////////////
class ClientToServerCmd {
public:
    virtual ~ClientToServerCmd();

    /// The host where the client was called
    const std::string& hostname() const { return cl_host_; }

    /// The second print is for use by EditHistoryMgr when we have commands that take multiple paths
    /// The EditHistoryMgr records what command was applied to each node. However when logging the
    /// the command we do not want logs all the paths, for each node.(performance bottleneck
    /// when dealing with thousands of paths)
    virtual void print(std::string& os) const = 0;
    virtual void print(std::string& os, const std::string& /*path*/) const { print(os); }

    /// Print the command without trailing <user>@<host>. Used by Group command, avoids duplicate user@host for each
    /// child command
    virtual void print_only(std::string& os) const { return print(os); }

    /// Print with minimal information. Deals with errors report where cmd have thousands of paths. truncate to one
    /// path. This should *NOT* include trailing <user>@<host>
    virtual std::string print_short() const {
        std::string ret;
        print_only(ret);
        return ret;
    }

    virtual bool equals(ClientToServerCmd* rhs) const;

    virtual void set_identity(ecf::Identity identity) { identity_ = std::move(identity); }
    [[nodiscard]] ecf::Identity& identity() const { return identity_; }

    [[nodiscard]] virtual ecf::authorisation_t authenticate(AbstractServer& server) const = 0;
    [[nodiscard]] virtual ecf::authorisation_t authorise(AbstractServer& server) const    = 0;

    /**
     *  Evaluates the preconditions of the command, thus acting as a command customisation point.
     *  This method is called after authentication and authorization, but before actually executing
     *  any command specific 'work'. Each command class overrides to check required preconditions.
     *
     *  Note: This mechanism is important for TaskCmds, which use the precondition check
     *  to detect/handle the presense of Zombie tasks
     *
     *  @param server the server executing the command
     *  @param reply the reply to be sent the client in case preconditions are invalid
     *  (remains unchanged if preconditions are valid)
     *
     *  @return true if preconditions are valid; otherwise, false.
     */
    [[nodiscard]] virtual bool check_preconditions(AbstractServer* server, [[maybe_unused]] STC_Cmd_ptr& reply) const {
        return true;
    }

    /// Called by the _server_ to service the client depending on the Command
    /// The server will pass itself down via the AbstractServer
    /// The returned Server to Client  command is sent back to the client
    /// Uses template pattern, it first authenticates request and then calls
    /// doHandleRequest. This function can throw exceptions. std::runtime_error
    STC_Cmd_ptr handleRequest(AbstractServer*) const;

    /// After handleRequest() has run, this function can be used reclaim memory
    virtual void cleanup() {}

    /// Returns true if handleRequest is testable. Only used in TEST
    virtual bool handleRequestIsTestable() const { return true; }

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

    /// Some read only commands under exceptional situation can modify the DEFS.
    /// i.e if write permission is removed from ECF_HOME then the check_pt command can fail
    /// in which case if can set flags, Flag::LATE, Flag::CHECKPT_ERROR, Flag::LOG_ERROR to warn the users
    /// This function is used to avoid unnecessary warning message.
    virtual bool is_mutable() const { return false; }

    /// Some commands modify the server but do not affect defs. i.e reload white list file,password file
    /// Other like ClientHandleCmd make edits to defs(well kind off) but are read only.(i.e anyone can call them)
    /// This return true for those commands that affect the defs, that we need sync on the client side.
    virtual bool cmd_updates_defs() const { return isWrite(); }

    /// This Must be called for client->server commands.As this is required
    /// for authentication. *However* task based commands have their own authentication
    /// mechanism, and don't need setup_user_authentification().
    virtual void setup_user_authentification(const std::string& user, const std::string& passwd) = 0; // Used by PlugCmd
    virtual bool setup_user_authentification(AbstractClientEnv&) = 0; // set user and passwd
    virtual void setup_user_authentification()                   = 0; // if user empty setup.

    /// Allow control over connection to different servers/hosts if the main server is down
    /// i.e for a getCmd, we do not want to wait 24 hours, trying all the servers
    /// However for Task based commands like , init,abort,event, meter,complete we
    /// want this behaviour as it can alter Node tree state and thus affect dependent nodes
    virtual bool connect_to_different_servers() const { return false; }

    /// The show occurs on the client side
    virtual PrintStyle::Type_t show_style() const { return PrintStyle::NOTHING; }

    // Other commands
    virtual bool get_cmd() const { return false; }
    virtual bool task_cmd() const { return false; }
    virtual bool terminate_cmd() const { return false; }
    virtual bool group_cmd() const { return false; }
    virtual bool ping_cmd() const { return false; }
    virtual bool why_cmd(std::string&) const { return false; }
    virtual bool show_cmd() const { return false; }
    virtual void add_edit_history(Defs*) const;

    // used by group_cmd to postfix syncCmd on all user commands that modify defs
    virtual void set_client_handle(int /*client_handle*/) {} // used by group_cmd
    virtual void set_group_cmd(const GroupCTSCmd*) {}

    // CLIENT side Parse and command construction, create can throw std::runtime_error for errors
    virtual const char* theArg() const                                              = 0; // used for argument parsing
    virtual void addOption(boost::program_options::options_description& desc) const = 0;
    virtual void
    create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const = 0;

protected:
    ClientToServerCmd();

    /// called by handleRequest, part of the template pattern
    virtual STC_Cmd_ptr doHandleRequest(AbstractServer*) const = 0;

    virtual bool validate() const { return true; }

    // /// return true if authentication succeeds, false and STC_Cmd_ptr to return otherwise
    // /// This function is called from doHandleRequest and hence is called
    // /// from within the server. The default implementation will get the current
    // /// user and authenticate with reference to the white list file
    // virtual bool authenticate(AbstractServer*, STC_Cmd_ptr&) const = 0;

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
    node_ptr find_node(Defs*, const std::string& absNodepath) const;

    /// Find The node for edit, otherwise throw std:::runtime_error
    /// Will add the node edit history collection
    node_ptr find_node_for_edit(Defs*, const std::string& absNodepath) const;

    /// Find The node for edit, otherwise return a NULL pointer
    /// Will add the node edit history collection
    node_ptr find_node_for_edit_no_throw(Defs*, const std::string& absNodepath) const;

    /// finds the associated node and adds to edit history nodes
    void add_node_for_edit_history(Defs* as, const std::string& absNodepath) const;
    void add_node_for_edit_history(node_ptr) const;
    void add_node_path_for_edit_history(const std::string& absNodepath) const;

    void add_edit_history(Defs*, const std::string& path) const;
    void add_delete_edit_history(Defs*, const std::string& path) const;

    mutable bool use_EditHistoryMgr_{
        true}; // sometime quicker to add edit history in command, than using EditHistoryMgr
private:
    friend class GroupCTSCmd;
    friend class EditHistoryMgr;
    mutable std::vector<weak_node_ptr> edit_history_nodes_;    // NOT persisted
    mutable std::vector<std::string> edit_history_node_paths_; // NOT persisted, used when deleting

    std::string cl_host_; // The host where the client was called

    mutable ecf::Identity identity_ = ecf::Identity::make_none(); // The identity of the user who issued the command

private:
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(CEREAL_NVP(cl_host_));
    }
};

#endif /* ecflow_base_cts_ClientToServerCmd_HPP */
