/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_ClientToServerRequest_HPP
#define ecflow_base_ClientToServerRequest_HPP

#include "ecflow/base/cts/ClientToServerCmd.hpp"

// Base class for client to server requesting.
// This class is used in the IPC messaging from  client to server.
class ClientToServerRequest {
public:
    ClientToServerRequest() = default;

    ClientToServerRequest(const ClientToServerRequest&)            = delete;
    ClientToServerRequest& operator=(const ClientToServerRequest&) = delete;
    ClientToServerRequest(ClientToServerRequest&&)                 = delete;
    ClientToServerRequest& operator=(ClientToServerRequest&&)      = delete;

    ~ClientToServerRequest() = default;

    void set_cmd(const Cmd_ptr& cmd) {
        cmd_ = cmd;
        cmd_->setup_user_authentification();
    }
    Cmd_ptr get_cmd() const { return cmd_; }

    /// This is called in the server only, to handle the quest.
    STC_Cmd_ptr handleRequest(AbstractServer*) const;

    std::ostream& print(std::ostream& os) const;

    bool getRequest() const { return (cmd_.get()) ? cmd_->get_cmd() : false; }
    bool terminateRequest() const { return (cmd_.get()) ? cmd_->terminate_cmd() : false; }
    bool groupRequest() const { return (cmd_.get()) ? cmd_->group_cmd() : false; }

    void cleanup() {
        if (cmd_.get()) {
            cmd_->cleanup();
        }
    } // reclaim memory *AFTER* command has run

    /// Used by boost test, to verify persistence
    bool operator==(const ClientToServerRequest& rhs) const;

private:
    Cmd_ptr cmd_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(cmd_));
    }
};

std::ostream& operator<<(std::ostream& os, const ClientToServerRequest& d);

#endif /* ecflow_base_ClientToServerRequest_HPP */
