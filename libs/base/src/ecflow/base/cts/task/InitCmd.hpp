/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_cts_task_InitCmd_HPP
#define ecflow_base_cts_task_InitCmd_HPP

#include "ecflow/base/cts/task/TaskCmd.hpp"
#include "ecflow/core/cereal_optional_nvp.hpp"

class InitCmd final : public TaskCmd {
public:
    InitCmd(const std::string& pathToTask,
            const std::string& jobsPassword,
            const std::string& process_or_remote_id,
            int try_no,
            const std::vector<Variable>& vec = {})
        : TaskCmd(pathToTask, jobsPassword, process_or_remote_id, try_no),
          var_to_add_(vec) {}

    InitCmd()
        : TaskCmd() {}

    const std::vector<Variable>& variables_to_add() const { return var_to_add_; }

    void print(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;
    [[nodiscard]] ecf::authorisation_t authorise(AbstractServer& server) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg(); // used for argument parsing

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    ecf::Child::CmdType child_type() const override { return ecf::Child::INIT; }

private:
    std::vector<Variable> var_to_add_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<TaskCmd>(this));
        CEREAL_OPTIONAL_NVP(ar, var_to_add_, [this]() { return !var_to_add_.empty(); }); // conditionally save
    }
};

std::ostream& operator<<(std::ostream& os, const InitCmd&);

CEREAL_FORCE_DYNAMIC_INIT(InitCmd)

#endif /* ecflow_base_cts_task_InitCmd_HPP */
