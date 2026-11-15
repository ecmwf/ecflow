// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/attribute/Zombie.hpp"
#include "ecflow/base/stc/ServerToClientCmd.hpp"
class AbstractServer;
namespace ecf {
class Visibility;
}

//================================================================================
// Paired with CtsCmd(GET_ZOMBIES)
// Client---CtsCmd(GET_ZOMBIES)---->Server-----(ZombieGetCmd)--->client:
//================================================================================
class ZombieGetCmd final : public ServerToClientCmd {
public:
    explicit ZombieGetCmd(AbstractServer*);
    ZombieGetCmd()
        : ServerToClientCmd() {}

    ///
    /// @brief Builds the reply in the server: the zombies of the tasks the requester can read.
    ///
    /// A zombie whose task no longer exists is listed when its closest existing ancestor below the server is readable.
    ///
    /// @param[in] as The server
    /// @param[in] visibility The nodes the requester can read; every node when null
    ///
    void init(AbstractServer* as, const ecf::Visibility* visibility = nullptr);
    bool handle_server_response(ServerReply&, Cmd_ptr cts_cmd, bool debug) const override;
    std::string print() const override;
    /// @copydoc ServerToClientCmd::filtering
    [[nodiscard]] ecf::ReplyFiltering filtering() const override { return ecf::ReplyFiltering::Listing; }
    bool equals(ServerToClientCmd*) const override;
    void cleanup() override {
        std::vector<Zombie>().swap(zombies_);
    } /// run in the server, after command send to client

private:
    std::vector<Zombie> zombies_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(this), CEREAL_NVP(zombies_));
    }
};

std::ostream& operator<<(std::ostream& os, const ZombieGetCmd&);
