/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_DeleteCmd_HPP
#define ecflow_base_cts_user_DeleteCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

// DELETE If paths_ empty will delete all suites (beware) else will delete the chosen nodes.
class DeleteCmd final : public UserCmd {
public:
    explicit DeleteCmd(const std::vector<std::string>& paths, bool force = false)
        : group_cmd_(nullptr),
          paths_(paths),
          force_(force) {}
    explicit DeleteCmd(const std::string& absNodePath, bool force = false);
    DeleteCmd() = default;

    const std::vector<std::string>& paths() const { return paths_; }
    bool force() const { return force_; }

    void print(std::string&) const override;
    void print_only(std::string&) const override;
    void print(std::string& os, const std::string& path) const override;

    bool equals(ClientToServerCmd*) const override;
    bool isWrite() const override { return true; }

    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;
    [[nodiscard]] ecf::authorisation_t authorise(AbstractServer& server) const override;

    const char* theArg() const override;
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

    // called in the server
    void set_group_cmd(const GroupCTSCmd* cmd) override { group_cmd_ = cmd; }

    static void check_for_active_or_submitted_tasks(AbstractServer* as, Node* theNodeToDelete);

private:
    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    // bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
    void cleanup() override { std::vector<std::string>().swap(paths_); } /// run in the server, after handlerequest

private:
    const GroupCTSCmd* group_cmd_{nullptr}; // not persisted only used in server
    std::vector<std::string> paths_;
    bool force_{false};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this), CEREAL_NVP(paths_), CEREAL_NVP(force_));
    }
};

std::ostream& operator<<(std::ostream& os, const DeleteCmd&);

CEREAL_FORCE_DYNAMIC_INIT(DeleteCmd)

#endif /* ecflow_base_cts_user_DeleteCmd_HPP */
