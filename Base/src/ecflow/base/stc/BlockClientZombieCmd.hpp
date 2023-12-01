/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_stc_BlockClientZombieCmd_HPP
#define ecflow_base_stc_BlockClientZombieCmd_HPP

#include "ecflow/base/stc/ServerToClientCmd.hpp"
#include "ecflow/core/Child.hpp"

//
// This command send the zombie type to the client
// For the moment we simply block for zombie, but the client
// in the future could do different things depending on the zombie type
//
class BlockClientZombieCmd final : public ServerToClientCmd {
public:
    explicit BlockClientZombieCmd(ecf::Child::ZombieType zt) : zombie_type_(zt) {}
    BlockClientZombieCmd() = default;

    void init(ecf::Child::ZombieType zt) { zombie_type_ = zt; } // server context
    ecf::Child::ZombieType zombie_type() const { return zombie_type_; }

    std::string print() const override;
    bool equals(ServerToClientCmd*) const override;
    bool
    handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const override; // client context

    bool is_returnable_in_group_cmd() const override { return false; } /// used by group command

private:
    ecf::Child::ZombieType zombie_type_{ecf::Child::NOT_SET};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(this), CEREAL_NVP(zombie_type_));
    }
};

std::ostream& operator<<(std::ostream& os, const BlockClientZombieCmd&);

#endif /* ecflow_base_stc_BlockClientZombieCmd_HPP */
