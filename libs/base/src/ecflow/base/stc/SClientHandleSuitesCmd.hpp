// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/base/stc/ServerToClientCmd.hpp"

class AbstractServer;
namespace ecf {
class Visibility;
}

//================================================================================
class SClientHandleSuitesCmd final : public ServerToClientCmd {
public:
    explicit SClientHandleSuitesCmd(AbstractServer* as);
    SClientHandleSuitesCmd()
        : ServerToClientCmd() {}

    ///
    /// @brief Builds the reply in the server: the handles and the suites they hold.
    ///
    /// With a visibility (Permissions mode), only the handles of the requester are listed, with the suites the
    /// requester can read (and the names of suites that do not exist); otherwise every handle is listed.
    ///
    /// @param[in] as The server
    /// @param[in] visibility The nodes the requester can read; none when Permissions do not apply
    /// @param[in] requester The user of the request, as authenticated
    ///
    void init(AbstractServer* as, const ecf::Visibility* visibility = nullptr, const std::string& requester = {});
    std::string print() const override;
    bool equals(ServerToClientCmd*) const override;
    bool handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const override;

private:
    std::vector<std::pair<std::string, std::vector<unsigned int>>> users_;          // users , list of handles
    std::vector<std::pair<unsigned int, std::vector<std::string>>> client_handles_; // handle, list of suites

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(this), CEREAL_NVP(users_), CEREAL_NVP(client_handles_));
    }
};

std::ostream& operator<<(std::ostream& os, const SClientHandleSuitesCmd&);
