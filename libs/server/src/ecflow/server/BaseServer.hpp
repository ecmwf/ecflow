/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_server_BaseServer_HPP
#define ecflow_server_BaseServer_HPP

#include <boost/asio.hpp>

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/server/CheckPtSaver.hpp"
#include "ecflow/server/NodeTreeTraverser.hpp"
#include "ecflow/server/ServerEnvironment.hpp"

class ServerEnvironment;

///
/// \brief ecFlow Server, with implementation based on Boost.asio
///
/// The port numbers are divided into three ranges:
///  o the Well Known Ports, (require root permission)  0   -1023
///  o the Registered Ports,                            1024-49151
///  o Dynamic and/or Private Ports.                    49151-65535
///

class BaseServer : public AbstractServer {
public:
    /// Constructor opens the acceptor and starts waiting for the first incoming
    /// connection.
    explicit BaseServer(boost::asio::io_context& io, ServerEnvironment&);
    ~BaseServer() override;

    void handle_terminate();

    // abort server if check pt files exist, but can't be loaded
    bool load_check_pt_file_on_startup();
    void loadCheckPtFile();
    bool restore_from_checkpt(const std::string& filename, bool& failed);
    void update_defs_server_state();
    void set_server_state(SState::State);

    /// AbstractServer functions
    SState::State state() const override { return serverState_; }
    std::pair<std::string, std::string> hostPort() const override;
    defs_ptr defs() const override { return defs_; }
    void updateDefs(defs_ptr, bool force) override;
    void clear_defs() override;
    bool checkPtDefs(ecf::CheckPt::Mode m         = ecf::CheckPt::UNDEFINED,
                     int check_pt_interval        = 0,
                     int check_pt_save_time_alarm = 0) override;
    void restore_defs_from_checkpt() override;
    void nodeTreeStateChanged() override;
    bool allowTaskCommunication() const override;
    void shutdown() override;
    void halted() override;
    void restart() override;

    bool reloadWhiteListFile(std::string& errorMsg) override;
    bool reloadPasswdFile(std::string& errorMsg) override;
    bool reloadCustomPasswdFile(std::string& errorMsg) override;

    ecf::AuthenticationService& authentication() override { return serverEnv_.authentication(); }
    const ecf::AuthenticationService& authentication() const override { return serverEnv_.authentication(); }

    ecf::AuthorisationService& authorisation() override { return serverEnv_.authorisation(); }
    const ecf::AuthorisationService& authorisation() const override { return serverEnv_.authorisation(); }

    bool lock(const std::string& user) override;
    void unlock() override;
    const std::string& lockedUser() const override;
    void traverse_node_tree_and_job_generate(const boost::posix_time::ptime& time_now,
                                             bool user_cmd_context) const override;
    int poll_interval() const override;
    void debug_server_on() override;
    void debug_server_off() override;
    bool debug() const override;

    // used in signal, for emergency check point during system session
    void sigterm_signal_handler();

    friend int run(BaseServer& server);

protected:
    /// The io_context used to perform asynchronous operations.
    boost::asio::io_context& io_;

    /// The signal_set is used to register for automatic check pointing
    boost::asio::signal_set signals_;

    defs_ptr defs_; // shared because is deleted in Test, and used in System::instance()
    NodeTreeTraverser traverser_;
    friend class NodeTreeTraverser;

    CheckPtSaver checkPtSaver_;
    friend class CheckPtSaver;

    SState::State serverState_;
    SState::State server_state_to_preserve_{SState::RUNNING}; // preserve state over plug cmd ECFLOW-1606

    ServerEnvironment& serverEnv_;
    std::string userWhoHasLock_;
};

#endif /* ecflow_server_BaseServer_HPP */
