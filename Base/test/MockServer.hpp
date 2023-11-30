/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_test_MockServer_HPP
#define ecflow_base_test_MockServer_HPP

#include <cassert>

#include <boost/core/noncopyable.hpp>

#include "AbstractServer.hpp"
#include "Defs.hpp"
#include "Ecf.hpp" // In server we increment modify and state change numbers,
#include "Jobs.hpp"
#include "JobsParam.hpp"
#include "Log.hpp"
#include "SuiteChanged.hpp"

/// Act as stand in for a server since Request require at least a AbstractServer
class MockServer : public AbstractServer {
public:
    // For the MockServer don't delete the Defs. since we past in a fixture defs
    struct null_deleter
    {
        void operator()(void const*) const {}
    };

    // Only in server side do we increment state/modify numbers, controlled by: Ecf::set_server(true)
    explicit MockServer(Defs* defs) : defs_(defs_ptr(defs, MockServer::null_deleter())) { Ecf::set_server(true); }
    explicit MockServer(defs_ptr defs) : defs_(defs) { Ecf::set_server(true); }
    ~MockServer() override { Ecf::set_server(false); }

    void set_server_state(SState::State ss) {
        serverState_    = ss;
        stats().status_ = static_cast<int>(serverState_);
        defs_->set_server().set_state(serverState_);
    }

    // AbstractServer functions
    SState::State state() const override { return serverState_; }
    std::pair<std::string, std::string> hostPort() const override {
        assert(defs_.get());
        return defs_->server().hostPort();
    }
    defs_ptr defs() const override { return defs_; }
    void updateDefs(defs_ptr d, bool force) override {
        assert(defs_.get());
        defs_->absorb(d.get(), force);
    }
    void clear_defs() override {
        if (defs_.get())
            defs_->clear();
    } // dont delete since we pass in Fixture defs. Otherwise it will crash
    bool checkPtDefs(ecf::CheckPt::Mode m         = ecf::CheckPt::UNDEFINED,
                     int check_pt_interval        = 0,
                     int check_pt_save_time_alarm = 0) override {
        return true;
    }
    void restore_defs_from_checkpt() override {}
    void nodeTreeStateChanged() override {}
    bool allowTaskCommunication() const override { return (serverState_ != SState::HALTED) ? true : false; }
    void shutdown() override { set_server_state(SState::SHUTDOWN); }
    void halted() override { set_server_state(SState::HALTED); }
    void restart() override { set_server_state(SState::RUNNING); }
    bool reloadWhiteListFile(std::string&) override { return true; }
    bool reloadPasswdFile(std::string& errorMsg) override { return true; }
    bool reloadCustomPasswdFile(std::string& errorMsg) override { return true; }
    bool authenticateReadAccess(const std::string&, bool custom_user, const std::string& passwd) override {
        return true;
    }
    bool authenticateReadAccess(const std::string&,
                                bool custom_user,
                                const std::string& passwd,
                                const std::string&) override {
        return true;
    }
    bool authenticateReadAccess(const std::string&,
                                bool custom_user,
                                const std::string& passwd,
                                const std::vector<std::string>&) override {
        return true;
    }
    bool authenticateWriteAccess(const std::string&) override { return true; }
    bool authenticateWriteAccess(const std::string&, const std::string&) override { return true; }
    bool authenticateWriteAccess(const std::string&, const std::vector<std::string>&) override { return true; }

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
            if (!jobs.generate(jobsParam))
                ecf::log(ecf::Log::ERR, jobsParam.getErrorMsg()); // will automatically add end of line
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
};

/// This class is used to create a Mock Server, so that we can make direct
/// data model changes without using commands.
/// In particular it will:
///   o Ecf::set_server(true): This controls incrementing of state/modify change numbers
///                            which should *only* be done on the server side
///   o Update Suite state/modify change number
class MockSuiteChangedServer : private boost::noncopyable {
public:
    explicit MockSuiteChangedServer(suite_ptr suite) : suiteChanged_(suite) { Ecf::set_server(true); }
    ~MockSuiteChangedServer() { Ecf::set_server(false); }

private:
    ecf::SuiteChanged suiteChanged_;
};

#endif /* ecflow_base_test_MockServer_HPP */
