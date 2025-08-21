/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_task_TaskCmd_HPP
#define ecflow_base_cts_task_TaskCmd_HPP

#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/core/Child.hpp"

//=================================================================================
// Task Command
// ================================================================================
class TaskCmd : public ClientToServerCmd {
protected:
    TaskCmd(const std::string& pathToSubmittable,
            const std::string& jobsPassword,
            const std::string& process_or_remote_id,
            int try_no)
        : submittable_(nullptr),
          path_to_submittable_(pathToSubmittable),
          jobs_password_(jobsPassword),
          process_or_remote_id_(process_or_remote_id),
          try_no_(try_no) {
        assert(!hostname().empty());
    }

    TaskCmd() = default;

public:
    bool isWrite() const override { return true; }
    int timeout() const override { return 190; } // ECFLOW-157 80 -> 190

    const std::string& path_to_node() const { return path_to_submittable_; }
    const std::string& jobs_password() const { return jobs_password_; }
    const std::string& process_or_remote_id() const { return process_or_remote_id_; }
    int try_no() const { return try_no_; }
    virtual ecf::Child::CmdType child_type() const = 0;

    bool equals(ClientToServerCmd*) const override;
    bool task_cmd() const override { return true; }
    bool connect_to_different_servers() const override { return true; }

    bool password_missmatch() const { return password_missmatch_; }
    bool pid_missmatch() const { return pid_missmatch_; }

    /// @see ClientToServer#check_preconditions
    [[nodiscard]] bool check_preconditions(AbstractServer* server, STC_Cmd_ptr& reply) const override;

protected:
    /// Overridden to do nothing since Task based commands don't need _user_ based authentication
    void setup_user_authentification(const std::string& /*user*/, const std::string& /*passwd*/) override {}
    bool setup_user_authentification(AbstractClientEnv&) override { return true; }
    void setup_user_authentification() override {}

    Submittable* get_submittable(AbstractServer* as) const; // can throw std::runtime_error

protected:
    mutable Submittable* submittable_{nullptr};
    // stored during authentication and re-used handle request, not persisted, server side only

private:
    std::string path_to_submittable_;
    std::string jobs_password_;
    std::string process_or_remote_id_;
    int try_no_{0};

private:
    mutable bool password_missmatch_{
        false}; // stored during authentication and re-used handle request, not persisted, server side only
    mutable bool pid_missmatch_{
        false}; // stored during authentication and re-used handle request, not persisted, server side only

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<ClientToServerCmd>(this),
           CEREAL_NVP(path_to_submittable_),
           CEREAL_NVP(jobs_password_),
           CEREAL_NVP(process_or_remote_id_),
           CEREAL_NVP(try_no_));
    }
};

#endif /* ecflow_base_cts_task_TaskCmd_HPP */
