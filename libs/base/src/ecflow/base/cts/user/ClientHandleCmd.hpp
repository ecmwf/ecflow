/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_ClientHandleCmd_HPP
#define ecflow_base_cts_user_ClientHandleCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

class ClientHandleCmd final : public UserCmd {
public:
    enum Api { REGISTER, DROP, DROP_USER, ADD, REMOVE, AUTO_ADD, SUITES };

    explicit ClientHandleCmd(Api api = AUTO_ADD) : api_(api) {}

    ClientHandleCmd(int client_handle, const std::vector<std::string>& suites, bool add_add_new_suites)
        : api_(REGISTER),
          client_handle_(client_handle),
          suites_(suites),
          auto_add_new_suites_(add_add_new_suites) {}

    explicit ClientHandleCmd(int client_handle) : api_(DROP), client_handle_(client_handle) {}

    explicit ClientHandleCmd(const std::string& drop_user) : api_(DROP_USER), drop_user_(drop_user) {}

    ClientHandleCmd(int client_handle, const std::vector<std::string>& suites, Api api)
        : api_(api), // Must be ADD or REMOVE
          client_handle_(client_handle),
          suites_(suites) {}

    ClientHandleCmd(int client_handle, bool add_add_new_suites)
        : api_(AUTO_ADD),
          client_handle_(client_handle),
          auto_add_new_suites_(add_add_new_suites) {}

    Api api() const { return api_; }
    const std::string& drop_user() const { return drop_user_; }

    bool cmd_updates_defs() const override;
    void print(std::string&) const override;
    void print_only(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;
    [[nodiscard]] ecf::authorisation_t authorise(AbstractServer& server) const override;

    const char* theArg() const override;
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

    // called in the server
    void set_group_cmd(const GroupCTSCmd* cmd) override { group_cmd_ = cmd; }

private:
    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

    Api api_;
    int client_handle_{0};
    std::string drop_user_;
    std::vector<std::string> suites_;
    bool auto_add_new_suites_{false};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this),
           CEREAL_NVP(api_),
           CEREAL_NVP(client_handle_),
           CEREAL_NVP(drop_user_),
           CEREAL_NVP(suites_),
           CEREAL_NVP(auto_add_new_suites_));
    }

private:
    const GroupCTSCmd* group_cmd_ = nullptr; // not persisted only used in server
};

std::ostream& operator<<(std::ostream& os, const ClientHandleCmd&);

CEREAL_FORCE_DYNAMIC_INIT(ClientHandleCmd)

#endif /* ecflow_base_cts_user_ClientHandleCmd_HPP */
