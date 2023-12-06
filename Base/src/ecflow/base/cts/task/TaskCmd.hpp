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

protected:
    /// Overridden to do nothing since Task based commands don't need _user_ based authentication
    void setup_user_authentification(const std::string& /*user*/, const std::string& /*passwd*/) override {}
    bool setup_user_authentification(AbstractClientEnv&) override { return true; }
    void setup_user_authentification() override {}

    bool authenticate(AbstractServer*,
                      STC_Cmd_ptr&) const override; /// Task have their own mechanism,can throw std::runtime_error
    Submittable* get_submittable(AbstractServer* as) const; // can throw std::runtime_error

protected:
    mutable Submittable* submittable_{
        nullptr}; // stored during authentication and re-used handle request, not persisted, server side only

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

class InitCmd final : public TaskCmd {
public:
    InitCmd(const std::string& pathToTask,
            const std::string& jobsPassword,
            const std::string& process_or_remote_id,
            int try_no,
            const std::vector<Variable>& vec = {})
        : TaskCmd(pathToTask, jobsPassword, process_or_remote_id, try_no),
          var_to_add_(vec) {}

    InitCmd() : TaskCmd() {}

    const std::vector<Variable>& variables_to_add() const { return var_to_add_; }

    void print(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

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

/// A child command that evaluates a expression. If the expression is false.
/// Then client invoker will block.
class CtsWaitCmd final : public TaskCmd {
public:
    CtsWaitCmd(const std::string& pathToTask,
               const std::string& jobsPassword,
               const std::string& process_or_remote_id,
               int try_no,
               const std::string& expression);
    CtsWaitCmd() : TaskCmd() {}

    const std::string& expression() const { return expression_; }

    void print(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    ecf::Child::CmdType child_type() const override { return ecf::Child::WAIT; }

    std::string expression_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<TaskCmd>(this), CEREAL_NVP(expression_));
    }
};

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

class MeterCmd final : public TaskCmd {
public:
    MeterCmd(const std::string& pathToTask,
             const std::string& jobsPassword,
             const std::string& process_or_remote_id,
             int try_no,
             const std::string& meterName,
             int meterValue)
        : TaskCmd(pathToTask, jobsPassword, process_or_remote_id, try_no),
          name_(meterName),
          value_(meterValue) {}
    MeterCmd() : TaskCmd() {}

    const std::string& name() const { return name_; }
    int value() const { return value_; }

    void print(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    ecf::Child::CmdType child_type() const override { return ecf::Child::METER; }

private:
    std::string name_; // the meters name
    int value_{0};     // the meters value

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<TaskCmd>(this), CEREAL_NVP(name_), CEREAL_NVP(value_));
    }
};

class LabelCmd final : public TaskCmd {
public:
    LabelCmd(const std::string& pathToTask,
             const std::string& jobsPassword,
             const std::string& process_or_remote_id,
             int try_no,
             const std::string& name,
             const std::string& label)
        : TaskCmd(pathToTask, jobsPassword, process_or_remote_id, try_no),
          name_(name),
          label_(label) {}
    LabelCmd() : TaskCmd() {}

    const std::string& name() const { return name_; }
    const std::string& label() const { return label_; }

    void print(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    ecf::Child::CmdType child_type() const override { return ecf::Child::LABEL; }

private:
    std::string name_;  // the label name
    std::string label_; // a single label, or multi-line label

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<TaskCmd>(this), CEREAL_NVP(name_), CEREAL_NVP(label_));
    }
};

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

std::ostream& operator<<(std::ostream& os, const InitCmd&);
std::ostream& operator<<(std::ostream& os, const EventCmd&);
std::ostream& operator<<(std::ostream& os, const MeterCmd&);
std::ostream& operator<<(std::ostream& os, const LabelCmd&);
std::ostream& operator<<(std::ostream& os, const CompleteCmd&);
std::ostream& operator<<(std::ostream& os, const CtsWaitCmd&);
std::ostream& operator<<(std::ostream& os, const AbortCmd&);

CEREAL_REGISTER_TYPE(InitCmd)
CEREAL_REGISTER_TYPE(EventCmd)
CEREAL_REGISTER_TYPE(MeterCmd)
CEREAL_REGISTER_TYPE(LabelCmd)
CEREAL_REGISTER_TYPE(QueueCmd)
CEREAL_REGISTER_TYPE(AbortCmd)
CEREAL_REGISTER_TYPE(CtsWaitCmd)
CEREAL_REGISTER_TYPE(CompleteCmd)

#endif /* ecflow_base_cts_task_TaskCmd_HPP */
