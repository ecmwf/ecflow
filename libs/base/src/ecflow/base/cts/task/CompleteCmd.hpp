/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_task_CompleteCmd_HPP
#define ecflow_base_cts_task_CompleteCmd_HPP

#include "ecflow/base/cts/task/TaskCmd.hpp"
#include "ecflow/core/cereal_optional_nvp.hpp"

class CompleteCmd final : public TaskCmd {
public:
    CompleteCmd(const std::string& pathToTask,
                const std::string& jobsPassword,
                const std::string& process_or_remote_id = "",
                int try_no                              = 1,
                const std::vector<std::string>& vec     = std::vector<std::string>())
        : TaskCmd(pathToTask, jobsPassword, process_or_remote_id, try_no),
          var_to_del_(vec) {}
    CompleteCmd() : TaskCmd() {}

    const std::vector<std::string>& variables_to_delete() const { return var_to_del_; }

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
    ecf::Child::CmdType child_type() const override { return ecf::Child::COMPLETE; }

private:
    std::vector<std::string> var_to_del_; //  variables to delete on task

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<TaskCmd>(this));
        CEREAL_OPTIONAL_NVP(ar, var_to_del_, [this]() { return !var_to_del_.empty(); }); // conditionally save
    }
};

std::ostream& operator<<(std::ostream& os, const CompleteCmd&);

CEREAL_FORCE_DYNAMIC_INIT(CompleteCmd)

#endif /* ecflow_base_cts_task_CompleteCmd_HPP */
