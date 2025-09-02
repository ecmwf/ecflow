/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_GroupCTSCmd_HPP
#define ecflow_base_cts_user_GroupCTSCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

// The group command allows a series of commands to be be executed:
//
// Client---(GroupCTSCmd)---->Server-----(GroupSTCCmd | StcCmd(OK) | Error )--->client:
//
// If client->server contains GetDefs cmd and log file commands, then a group
// command will be created for returning to the client
//
// If group command contains multiple [ CtsNodeCmd(GET) | LogCmd --get ] commands then
// all the results are returned back to the client, HOWEVER when client calls
// Cmd::defs() | Cmd::get_string() only the first data is returned.
//
class GroupCTSCmd final : public UserCmd {
public:
    GroupCTSCmd(const std::string& list_of_commands, AbstractClientEnv* clientEnv);
    explicit GroupCTSCmd(Cmd_ptr cmd) : cli_(false) { addChild(cmd); }
    GroupCTSCmd() = default;

    bool isWrite() const override;
    bool cmd_updates_defs() const override;

    void set_identity(const ecf::Identity& identity) override;

    PrintStyle::Type_t show_style() const override;
    bool get_cmd() const override;
    bool task_cmd() const override;
    bool terminate_cmd() const override;
    bool why_cmd(std::string&) const override;
    bool group_cmd() const override { return true; }

    void set_client_handle(int client_handle) const; // used in group sync with client register

    void print(std::string&) const override;
    std::string print_short() const override;
    bool equals(ClientToServerCmd*) const override;

    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;
    [[nodiscard]] ecf::authorisation_t authorise(AbstractServer& server) const override;

    void addChild(Cmd_ptr childCmd);
    const std::vector<Cmd_ptr>& cmdVec() const { return cmdVec_; }

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

    void add_edit_history(Defs*) const override;

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

    void setup_user_authentification(const std::string& user, const std::string& passwd) override;
    bool setup_user_authentification(AbstractClientEnv&) override;
    void setup_user_authentification() override;

    // bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    void cleanup() override; // cleanup all children

private:
    std::vector<Cmd_ptr> cmdVec_;
    bool cli_{true};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this), CEREAL_NVP(cmdVec_), CEREAL_NVP(cli_));
    }
};

std::ostream& operator<<(std::ostream& os, const GroupCTSCmd&);

CEREAL_FORCE_DYNAMIC_INIT(GroupCTSCmd)

#endif /* ecflow_base_cts_user_GroupCTSCmd_HPP */
