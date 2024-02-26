/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_task_EventCmd_HPP
#define ecflow_base_cts_task_EventCmd_HPP

#include "ecflow/base/cts/task/TaskCmd.hpp"
#include "ecflow/core/cereal_optional_nvp.hpp"

class EventCmd final : public TaskCmd {
public:
    EventCmd(const std::string& pathToTask,
             const std::string& jobsPassword,
             const std::string& process_or_remote_id,
             int try_no,
             const std::string& eventName,
             bool value = true) // true = set(default), false = clear
        : TaskCmd(pathToTask, jobsPassword, process_or_remote_id, try_no),
          name_(eventName),
          value_(value) {}
    EventCmd() : TaskCmd() {}

    const std::string& name() const { return name_; }
    bool value() const { return value_; }

    void print(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    ecf::Child::CmdType child_type() const override { return ecf::Child::EVENT; }

private:
    std::string name_; // the events name
    bool value_{true}; // true = set(default), false = clear

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<TaskCmd>(this), CEREAL_NVP(name_));
        CEREAL_OPTIONAL_NVP(ar, value_, [this]() { return !value_; }); // conditionally save if value is false
    }
};

std::ostream& operator<<(std::ostream& os, const EventCmd&);

CEREAL_FORCE_DYNAMIC_INIT(EventCmd)

#endif /* ecflow_base_cts_task_EventCmd_HPP */
