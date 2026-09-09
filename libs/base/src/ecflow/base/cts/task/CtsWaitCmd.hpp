/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_cts_task_CtsWaitCmd_HPP
#define ecflow_base_cts_task_CtsWaitCmd_HPP

#include "ecflow/base/cts/task/TaskCmd.hpp"

/// A child command that evaluates a expression. If the expression is false.
/// Then client invoker will block.
class CtsWaitCmd final : public TaskCmd {
public:
    CtsWaitCmd(const std::string& pathToTask,
               const std::string& jobsPassword,
               const std::string& process_or_remote_id,
               int try_no,
               const std::string& expression);
    CtsWaitCmd()
        : TaskCmd() {}

    const std::string& expression() const { return expression_; }

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
    ecf::Child::CmdType child_type() const override { return ecf::Child::WAIT; }

    std::string expression_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<TaskCmd>(this), CEREAL_NVP(expression_));
    }
};

std::ostream& operator<<(std::ostream& os, const CtsWaitCmd&);

CEREAL_FORCE_DYNAMIC_INIT(CtsWaitCmd)

#endif /* ecflow_base_cts_task_CtsWaitCmd_HPP */
