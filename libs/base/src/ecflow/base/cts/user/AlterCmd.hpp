/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_AlterCmd_HPP
#define ecflow_base_cts_user_AlterCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"
#include "ecflow/node/Flag.hpp"

class AlterCmd final : public UserCmd {
public:
    enum Delete_attr_type {
        DEL_VARIABLE,
        DEL_TIME,
        DEL_TODAY,
        DEL_DATE,
        DEL_DAY,
        DEL_CRON,
        DEL_EVENT,
        DEL_METER,
        DEL_LABEL,
        DEL_TRIGGER,
        DEL_COMPLETE,
        DEL_REPEAT,
        DEL_LIMIT,
        DEL_LIMIT_PATH,
        DEL_INLIMIT,
        DEL_ZOMBIE,
        DELETE_ATTR_ND,
        DEL_LATE,
        DEL_QUEUE,
        DEL_GENERIC,
        DEL_AVISO,
    };

    enum Change_attr_type {
        VARIABLE,
        CLOCK_TYPE,
        CLOCK_DATE,
        CLOCK_GAIN,
        EVENT,
        METER,
        LABEL,
        TRIGGER,
        COMPLETE,
        REPEAT,
        LIMIT_MAX,
        LIMIT_VAL,
        DEFSTATUS,
        CHANGE_ATTR_ND,
        CLOCK_SYNC,
        LATE,
        TIME,
        TODAY,
        AVISO,
    };

    enum Add_attr_type {
        ADD_TIME,
        ADD_TODAY,
        ADD_DATE,
        ADD_DAY,
        ADD_ZOMBIE,
        ADD_VARIABLE,
        ADD_ATTR_ND,
        ADD_LATE,
        ADD_LIMIT,
        ADD_INLIMIT,
        ADD_LABEL,
        ADD_AVISO,
    };

    // Python
    AlterCmd(const std::vector<std::string>& paths,
             const std::string& alterType, /* one of [ add | change | delete | set_flag | clear_flag ] */
             const std::string& attrType,
             const std::string& name,
             const std::string& value);
    // add
    AlterCmd(const std::string& path, Add_attr_type attr, const std::string& name, const std::string& value = "")
        : paths_(std::vector<std::string>(1, path)),
          name_(name),
          value_(value),
          add_attr_type_(attr) {}
    AlterCmd(const std::vector<std::string>& paths,
             Add_attr_type attr,
             const std::string& name,
             const std::string& value = "")
        : paths_(paths),
          name_(name),
          value_(value),
          add_attr_type_(attr) {}
    // delete
    AlterCmd(const std::string& path, Delete_attr_type del, const std::string& name = "", const std::string& value = "")
        : paths_(std::vector<std::string>(1, path)),
          name_(name),
          value_(value),
          del_attr_type_(del) {}
    AlterCmd(const std::vector<std::string>& paths,
             Delete_attr_type del,
             const std::string& name  = "",
             const std::string& value = "")
        : paths_(paths),
          name_(name),
          value_(value),
          del_attr_type_(del) {}
    // change
    AlterCmd(const std::string& path, Change_attr_type attr, const std::string& name, const std::string& value = "")
        : paths_(std::vector<std::string>(1, path)),
          name_(name),
          value_(value),
          change_attr_type_(attr) {}
    AlterCmd(const std::vector<std::string>& paths,
             Change_attr_type attr,
             const std::string& name,
             const std::string& value = "")
        : paths_(paths),
          name_(name),
          value_(value),
          change_attr_type_(attr) {}
    // flag
    AlterCmd(const std::string& path, ecf::Flag::Type ft, bool flag)
        : paths_(std::vector<std::string>(1, path)),
          flag_type_(ft),
          flag_(flag) {}
    AlterCmd(const std::vector<std::string>& paths, ecf::Flag::Type ft, bool flag)
        : paths_(paths),
          flag_type_(ft),
          flag_(flag) {}
    // sort
    AlterCmd(const std::string& path, const std::string& name, const std::string& value)
        : paths_(std::vector<std::string>(1, path)),
          name_(name),
          value_(value) {}
    AlterCmd(const std::vector<std::string>& paths, const std::string& name, const std::string& value)
        : paths_(paths),
          name_(name),
          value_(value) {}

    AlterCmd() = default;

    // Uses by equals only
    const std::vector<std::string>& paths() const { return paths_; }
    const std::string& name() const { return name_; }
    const std::string& value() const { return value_; }
    Delete_attr_type delete_attr_type() const { return del_attr_type_; }
    Change_attr_type change_attr_type() const { return change_attr_type_; }
    Add_attr_type add_attr_type() const { return add_attr_type_; }
    ecf::Flag::Type flag_type() const { return flag_type_; }
    bool flag() const { return flag_; }

    bool isWrite() const override { return true; }
    void print(std::string&) const override;
    void print(std::string& os, const std::string& path) const override;
    void print_only(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    STC_Cmd_ptr alter_server_state(AbstractServer*) const;
    bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
    void cleanup() override { std::vector<std::string>().swap(paths_); } /// run in the server, after doHandleRequest

    void my_print(std::string& os, const std::vector<std::string>& paths) const;

    Add_attr_type get_add_attr_type(const std::string&) const;
    void createAdd(Cmd_ptr& cmd, std::vector<std::string>& options, std::vector<std::string>& paths) const;
    void extract_name_and_value_for_add(Add_attr_type,
                                        std::string& name,
                                        std::string& value,
                                        std::vector<std::string>& options,
                                        std::vector<std::string>& paths) const;
    void check_for_add(Add_attr_type, const std::string& name, const std::string& value) const;

    Delete_attr_type get_delete_attr_type(const std::string&) const;
    void
    createDelete(Cmd_ptr& cmd, const std::vector<std::string>& options, const std::vector<std::string>& paths) const;
    void extract_name_and_value_for_delete(Delete_attr_type,
                                           std::string& name,
                                           std::string& value,
                                           const std::vector<std::string>& options,
                                           const std::vector<std::string>& paths) const;
    void check_for_delete(Delete_attr_type, const std::string& name, const std::string& value) const;

    Change_attr_type get_change_attr_type(const std::string&) const;
    void createChange(Cmd_ptr& cmd, std::vector<std::string>& options, std::vector<std::string>& paths) const;
    void extract_name_and_value_for_change(Change_attr_type,
                                           std::string& name,
                                           std::string& value,
                                           std::vector<std::string>& options,
                                           std::vector<std::string>& paths) const;
    void check_for_change(Change_attr_type, const std::string& name, const std::string& value) const;

    ecf::Flag::Type get_flag_type(const std::string&) const;
    void create_flag(Cmd_ptr& cmd,
                     const std::vector<std::string>& options,
                     const std::vector<std::string>& paths,
                     bool flag) const;

    void check_sort_attr_type(const std::string&) const;
    void create_sort_attributes(Cmd_ptr& cmd,
                                const std::vector<std::string>& options,
                                const std::vector<std::string>& paths) const;

    void alter_and_attr_type(std::string& alter_type, std::string& attr_type) const;

private:
    std::vector<std::string> paths_;
    std::string name_;
    std::string value_;
    Add_attr_type add_attr_type_{ADD_ATTR_ND};
    Delete_attr_type del_attr_type_{DELETE_ATTR_ND};
    Change_attr_type change_attr_type_{CHANGE_ATTR_ND};
    ecf::Flag::Type flag_type_{ecf::Flag::NOT_SET};
    bool flag_{false}; // true means set false means clear

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this),
           CEREAL_NVP(paths_),
           CEREAL_NVP(name_),
           CEREAL_NVP(value_),
           CEREAL_NVP(add_attr_type_),
           CEREAL_NVP(del_attr_type_),
           CEREAL_NVP(change_attr_type_),
           CEREAL_NVP(flag_type_),
           CEREAL_NVP(flag_));
    }
};

std::ostream& operator<<(std::ostream& os, const AlterCmd&);

CEREAL_FORCE_DYNAMIC_INIT(AlterCmd)

#endif /* ecflow_base_cts_user_AlterCmd_HPP */
