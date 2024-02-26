/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_task_QueueCmd_HPP
#define ecflow_base_cts_task_QueueCmd_HPP

#include "ecflow/base/cts/task/TaskCmd.hpp"

class QueueAttr;

class QueueCmd final : public TaskCmd {
public:
    QueueCmd(const std::string& pathToTask,
             const std::string& jobsPassword,
             const std::string& process_or_remote_id,
             int try_no,
             const std::string& queueName,
             const std::string& action,
             const std::string& step                    = "",
             const std::string& path_to_node_with_queue = "") // if empty search for queue up node tree
        : TaskCmd(pathToTask, jobsPassword, process_or_remote_id, try_no),
          name_(queueName),
          action_(action),
          step_(step),
          path_to_node_with_queue_(path_to_node_with_queue) {}
    QueueCmd() : TaskCmd() {}

    const std::string& name() const { return name_; }
    const std::string& action() const { return action_; }
    const std::string& step() const { return step_; }
    const std::string& path_to_node_with_queue() const { return path_to_node_with_queue_; }

    void print(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    ecf::Child::CmdType child_type() const override { return ecf::Child::QUEUE; }

    std::string handle_queue(QueueAttr& queue_attr) const;

private:
    std::string name_;   // the queue name
    std::string action_; // [ active | aborted | complete | no_of_aborted ]
    std::string step_;   // will be empty when action is [ active | no_of_aborted]
    std::string path_to_node_with_queue_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<TaskCmd>(this),
           CEREAL_NVP(name_),
           CEREAL_NVP(action_),
           CEREAL_NVP(step_),
           CEREAL_NVP(path_to_node_with_queue_));
    }
};

CEREAL_FORCE_DYNAMIC_INIT(QueueCmd)

#endif /* ecflow_base_cts_task_QueueCmd_HPP */
