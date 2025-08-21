/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_task_AbortCmd_HPP
#define ecflow_base_cts_task_AbortCmd_HPP

#include "ecflow/base/cts/task/TaskCmd.hpp"

class AbortCmd final : public TaskCmd {
public:
    AbortCmd(const std::string& pathToTask,
             const std::string& jobsPassword,
             const std::string& process_or_remote_id,
             int try_no                = 1,
             const std::string& reason = "");
    AbortCmd() : TaskCmd() {}

    const std::string& reason() const { return reason_; }

    void print(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;
    [[nodiscard]] ecf::authorisation_t authorise(AbstractServer& server) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    ecf::Child::CmdType child_type() const override { return ecf::Child::ABORT; }

    std::string reason_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<TaskCmd>(this), CEREAL_NVP(reason_));
    }
};

std::ostream& operator<<(std::ostream& os, const AbortCmd&);

CEREAL_FORCE_DYNAMIC_INIT(AbortCmd)

#endif /* ecflow_base_cts_task_AbortCmd_HPP */
