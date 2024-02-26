/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_EditScriptCmd_HPP
#define ecflow_base_cts_user_EditScriptCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

//================================================================================
// Paired with SStringCmd
// Client---(EditScriptCmd)---->Server-----(SStringCmd)--->client:
//================================================================================
class EditScriptCmd final : public UserCmd {
public:
    enum EditType { EDIT, PREPROCESS, SUBMIT, PREPROCESS_USER_FILE, SUBMIT_USER_FILE };
    EditScriptCmd(const std::string& path_to_node, EditType et) // EDIT or PREPROCESS
        : edit_type_(et),
          path_to_node_(path_to_node) {}

    EditScriptCmd(const std::string& path_to_node, const NameValueVec& user_variables)
        : edit_type_(SUBMIT),
          path_to_node_(path_to_node),
          user_variables_(user_variables) {}

    EditScriptCmd(const std::string& path_to_node, const std::vector<std::string>& user_file_contents)
        : edit_type_(PREPROCESS_USER_FILE),
          path_to_node_(path_to_node),
          user_file_contents_(user_file_contents) {}

    EditScriptCmd(const std::string& path_to_node,
                  const NameValueVec& user_variables,
                  const std::vector<std::string>& user_file_contents,
                  bool create_alias,
                  bool run_alias)
        : edit_type_(SUBMIT_USER_FILE),
          path_to_node_(path_to_node),
          user_file_contents_(user_file_contents),
          user_variables_(user_variables),
          alias_(create_alias),
          run_(run_alias) {}

    EditScriptCmd() = default;

    // Uses by equals only
    const std::string& path_to_node() const { return path_to_node_; }
    EditType edit_type() const { return edit_type_; }
    bool alias() const { return alias_; }
    bool run() const { return run_; }

    bool handleRequestIsTestable() const override { return false; }
    bool isWrite() const override;
    void print(std::string&) const override;
    void print_only(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
    void cleanup() override {
        std::vector<std::string>().swap(user_file_contents_);
    } /// run in the server, after doHandleRequest

private:
    EditType edit_type_{EDIT};
    std::string path_to_node_;
    mutable std::vector<std::string> user_file_contents_;
    NameValueVec user_variables_;
    bool alias_{false};
    bool run_{false};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this),
           CEREAL_NVP(edit_type_),
           CEREAL_NVP(path_to_node_),
           CEREAL_NVP(user_file_contents_),
           CEREAL_NVP(user_variables_),
           CEREAL_NVP(alias_),
           CEREAL_NVP(run_));
    }
};

std::ostream& operator<<(std::ostream& os, const EditScriptCmd&);

CEREAL_FORCE_DYNAMIC_INIT(EditScriptCmd)

#endif /* ecflow_base_cts_user_EditScriptCmd_HPP */
