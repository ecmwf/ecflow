/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_QueryCmd_HPP
#define ecflow_base_cts_user_QueryCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

class QueryCmd final : public UserCmd {
public:
    QueryCmd(const std::string& query_type,
             const std::string& path_to_attribute,
             const std::string& attribute,
             const std::string& path_to_task)
        : query_type_(query_type),
          path_to_attribute_(path_to_attribute),
          attribute_(attribute),
          path_to_task_(path_to_task) {}
    QueryCmd() : UserCmd() {}
    ~QueryCmd() override;

    const std::string& query_type() const { return query_type_; }
    const std::string& path_to_attribute() const { return path_to_attribute_; }
    const std::string& attribute() const { return attribute_; }
    const std::string& path_to_task() const { return path_to_task_; }

    void print(std::string&) const override;
    void print_only(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

    bool handleRequestIsTestable() const override { return false; }
    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

private:
    std::string query_type_; // [ state | dstate | event | meter | label | trigger ]
    std::string path_to_attribute_;
    std::string attribute_;    // [ event_name | meter_name | label_name | variable_name | trigger expression] empty for
                               // state and dstate
    std::string path_to_task_; // The task the invoked this command, needed for logging

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this),
           CEREAL_NVP(query_type_),
           CEREAL_NVP(path_to_attribute_),
           CEREAL_NVP(attribute_),
           CEREAL_NVP(path_to_task_));
    }
};

std::ostream& operator<<(std::ostream& os, const QueryCmd&);

CEREAL_FORCE_DYNAMIC_INIT(QueryCmd)

#endif /* ecflow_base_cts_user_QueryCmd_HPP */
