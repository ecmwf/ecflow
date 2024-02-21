/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_ZombieCmd_HPP
#define ecflow_base_cts_user_ZombieCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"
#include "ecflow/core/User.hpp"

class ZombieCmd final : public UserCmd {
public:
    ZombieCmd(ecf::User::Action uc,
              const std::vector<std::string>& paths,
              const std::string& process_id,
              const std::string& password)
        : user_action_(uc),
          process_id_(process_id),
          password_(password),
          paths_(paths) {}
    explicit ZombieCmd(ecf::User::Action uc = ecf::User::BLOCK) : user_action_(uc) {}

    const std::vector<std::string>& paths() const { return paths_; }
    const std::string& process_or_remote_id() const { return process_id_; }
    const std::string& password() const { return password_; }

    void print(std::string&) const override;
    void print_only(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    const char* theArg() const override;
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    void cleanup() override { std::vector<std::string>().swap(paths_); } /// run in the server, after handlerequest

    ecf::User::Action user_action_;
    std::string process_id_; // should be empty for multiple paths and when using CLI
    std::string password_;   // should be empty for multiple paths and when using CLI
    std::vector<std::string> paths_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this),
           CEREAL_NVP(user_action_),
           CEREAL_NVP(process_id_),
           CEREAL_NVP(password_),
           CEREAL_NVP(paths_));
    }
};

std::ostream& operator<<(std::ostream& os, const ZombieCmd&);

CEREAL_FORCE_DYNAMIC_INIT(ZombieCmd)

#endif /* ecflow_base_cts_user_ZombieCmd_HPP */
