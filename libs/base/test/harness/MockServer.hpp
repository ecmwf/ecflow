// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cassert>

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/RequestContext.hpp"
#include "ecflow/core/Ecf.hpp" // In server we increment modify and state change numbers,
#include "ecflow/core/Log.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Jobs.hpp"
#include "ecflow/node/JobsParam.hpp"
#include "ecflow/node/SuiteChanged.hpp"

/// Act as stand in for a server since Request require at least a AbstractServer
class MockServer : public AbstractServer {
public:
    // For the MockServer do not delete the Defs. since we past in a fixture defs
    struct null_deleter
    {
        void operator()(void const*) const {}
    };

    std::string ssl() const override { return ""; }

    // Only in server side do we increment state/modify numbers, controlled by: Ecf::set_server(true)
    explicit MockServer(Defs* defs)
        : defs_(defs_ptr(defs, MockServer::null_deleter())) {
        Ecf::set_server(true);
    }
    explicit MockServer(defs_ptr defs)
        : defs_(defs) {
        Ecf::set_server(true);
    }
    ~MockServer() override { Ecf::set_server(false); }

    void set_server_state(SState::State ss);

    // AbstractServer functions
    SState::State state() const override { return serverState_; }
    std::pair<std::string, std::string> hostPort() const override {
        assert(defs_.get());
        return defs_->server_state().hostPort();
    }
    defs_ptr defs() const override { return defs_; }
    void updateDefs(defs_ptr d, bool force) override {
        assert(defs_.get());
        defs_->absorb(d.get(), force);
    }
    void clear_defs() override {
        if (defs_.get()) {
            defs_->clear();
        }
    } // dont delete since we pass in Fixture defs. Otherwise it will crash
    bool checkPtDefs(ecf::CheckPt::Mode m         = ecf::CheckPt::UNDEFINED,
                     int check_pt_interval        = 0,
                     int check_pt_save_time_alarm = 0) override {
        return true;
    }
    void restore_defs_from_checkpt() override {}
    void nodeTreeStateChanged() override { ++node_tree_state_changes_; }

    ///
    /// @brief Returns how many times a change of the definitions was notified, i.e. could trigger a checkpoint.
    ///
    /// @return The number of notifications.
    ///
    [[nodiscard]] int node_tree_state_changes() const { return node_tree_state_changes_; }
    bool allowTaskCommunication() const override { return (serverState_ != SState::HALTED) ? true : false; }
    void shutdown() override { set_server_state(SState::SHUTDOWN); }
    void halted() override { set_server_state(SState::HALTED); }
    void restart() override { set_server_state(SState::RUNNING); }

    ecf::AuthenticationService& authentication() override { return authentication_service_; }
    const ecf::AuthenticationService& authentication() const override { return authentication_service_; }

    ///
    /// @brief Replaces the authorisation service applied to the requests handled.
    ///
    /// @param[in] service The service.
    ///
    void set_authorisation(ecf::AuthorisationService service) { authorisation_service_ = std::move(service); }

    ///
    /// @brief Records a synchronisation reply built by the requests handled.
    ///
    struct SyncReply
    {
        bool full;                    ///< True for a full synchronisation
        std::string user;             ///< The user the reply was built for
        const AbstractServer* server; ///< The server of the context of the request
        bool permissions_apply;       ///< Whether Permissions apply, by the evaluator of the context
        std::string server_roles;     ///< The roles of the user at the server, by the evaluator of the context
    };

    ///
    /// @brief Returns the synchronisation replies built so far, oldest first.
    ///
    /// @return The replies; the reference stays valid for the lifetime of the server.
    ///
    const std::vector<SyncReply>& sync_replies() const { return sync_replies_; }

    ///
    /// @brief Records a reply builder called by the requests handled.
    ///
    struct ReplyBuilt
    {
        std::string builder;          ///< The name of the builder
        std::string user;             ///< The requester of the context the builder received
        const AbstractServer* server; ///< The server of that context
    };

    ///
    /// @brief Returns the reply builders called so far, oldest first.
    ///
    /// @return The builders; the reference stays valid for the lifetime of the server.
    ///
    const std::vector<ReplyBuilt>& replies_built() const { return replies_built_; }

    /// @copydoc AbstractServer::reply_built
    void reply_built(const char* builder, const ecf::RequestContext& context) override {
        replies_built_.push_back(ReplyBuilt{builder, context.identity().username().value(), &context.server()});
    }

    /// @copydoc AbstractServer::sync_reply_built
    void sync_reply_built(bool full, const ecf::RequestContext& context) override {
        const auto& permissions = context.permissions();
        sync_replies_.push_back(SyncReply{full,
                                          context.identity().username().value(),
                                          &context.server(),
                                          permissions.applies(),
                                          permissions.roles(permissions.at_server()).to_string()});
    }

    ecf::AuthorisationService& authorisation() override { return authorisation_service_; }
    const ecf::AuthorisationService& authorisation() const override { return authorisation_service_; }

    bool audit_only() const override { return audit_only_; }

    ///
    /// @brief Selects the audit-only mode (R12.3), as the configuration of a server does.
    ///
    /// @param[in] audit_only True to execute, and log, the requests the Permissions attributes deny
    ///
    void set_audit_only(bool audit_only) { audit_only_ = audit_only; }

    bool reloadWhiteListFile(std::string&) override { return true; }
    bool reloadPasswdFile(std::string& errorMsg) override { return true; }
    bool reloadCustomPasswdFile(std::string& errorMsg) override { return true; }

    bool lock(const std::string& user) override {
        if (userWhoHasLock_.empty()) {
            userWhoHasLock_           = user;
            stats().locked_by_user_   = user;
            server_state_to_preserve_ = state();
            shutdown();
            return true;
        }
        else if (userWhoHasLock_ == user && serverState_ == SState::SHUTDOWN) {
            // Same user attempting multiple locks
            return true;
        }
        return false;
    }
    void unlock() override {
        userWhoHasLock_.clear();
        stats().locked_by_user_.clear();
        switch (server_state_to_preserve_) {
            case SState::RUNNING:
                restart();
                break;
            case SState::SHUTDOWN:
                shutdown();
                break;
            case SState::HALTED:
                halted();
                break;
        }
    }
    const std::string& lockedUser() const override { return userWhoHasLock_; }

    void traverse_node_tree_and_job_generate(const boost::posix_time::ptime& time_now,
                                             bool user_cmd_context) const override {
        if (state() == SState::RUNNING && defs_.get()) {
            JobsParam jobsParam(poll_interval(), false /* as->allow_job_creation_during_tree_walk() */);
            Jobs jobs(defs_);
            if (!jobs.generate(jobsParam)) {
                ecf::log(ecf::Log::ERR, jobsParam.getErrorMsg()); // will automatically add end of line
            }
        }
    }
    int poll_interval() const override { return 60; }
    void debug_server_on() override {}
    void debug_server_off() override {}
    bool debug() const override { return true; }

private:
    defs_ptr defs_;
    std::string userWhoHasLock_;
    SState::State serverState_{SState::RUNNING};
    SState::State server_state_to_preserve_{SState::RUNNING};

    ecf::AuthenticationService authentication_service_;
    ecf::AuthorisationService authorisation_service_;
    std::vector<SyncReply> sync_replies_;
    std::vector<ReplyBuilt> replies_built_;
    int node_tree_state_changes_{0};
    bool audit_only_{false};
};

/// This class is used to create a Mock Server, so that we can make direct
/// data model changes without using commands.
/// In particular it will:
///   o Ecf::set_server(true): This controls incrementing of state/modify change numbers
///                            which should *only* be done on the server side
///   o Update Suite state/modify change number
class MockSuiteChangedServer {
public:
    MockSuiteChangedServer() = delete;

    explicit MockSuiteChangedServer(suite_ptr suite)
        : suiteChanged_(suite) {
        Ecf::set_server(true);
    }

    MockSuiteChangedServer(const MockSuiteChangedServer&)            = delete;
    MockSuiteChangedServer& operator=(const MockSuiteChangedServer&) = delete;
    MockSuiteChangedServer(MockSuiteChangedServer&&)                 = delete;
    MockSuiteChangedServer& operator=(MockSuiteChangedServer&&)      = delete;

    ~MockSuiteChangedServer() { Ecf::set_server(false); }

private:
    ecf::SuiteChanged suiteChanged_;
};
