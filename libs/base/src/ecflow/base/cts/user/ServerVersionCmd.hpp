/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_ServerVersionCmd_HPP
#define ecflow_base_cts_user_ServerVersionCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

// ========================================================================
// This Command should NEVER be changed
// This will allow new client to ask OLD server about its version
// ========================================================================
class ServerVersionCmd final : public UserCmd {
public:
    ServerVersionCmd() = default;

    void print(std::string&) const override;
    void print_only(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;
    [[nodiscard]] ecf::authorisation_t authorise(AbstractServer& server) const override;

    const char* theArg() const override;
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this));
    }
};

std::ostream& operator<<(std::ostream& os, const ServerVersionCmd&);

CEREAL_FORCE_DYNAMIC_INIT(ServerVersionCmd)

#endif /* ecflow_base_cts_user_ServerVersionCmd_HPP */
