// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/base/stc/ServerToClientCmd.hpp"
class AbstractServer;
namespace ecf {
class Visibility;
}

//================================================================================
// Paired with CtsCmd(SUITES)
// Client---(CtsCmd(SUITES))---->Server-----(SSuitesCmd)--->client:
//================================================================================
class SSuitesCmd final : public ServerToClientCmd {
public:
    explicit SSuitesCmd(AbstractServer* as);
    SSuitesCmd()
        : ServerToClientCmd() {}

    ///
    /// @brief Builds the reply in the server: the names of the suites the requester can read.
    ///
    /// @param[in] as The server
    /// @param[in] visibility The nodes the requester can read; every node when null
    ///
    void init(AbstractServer* as, const ecf::Visibility* visibility = nullptr);
    std::string print() const override;
    /// @copydoc ServerToClientCmd::filtering
    [[nodiscard]] ecf::ReplyFiltering filtering() const override { return ecf::ReplyFiltering::Listing; }
    bool equals(ServerToClientCmd*) const override;
    bool handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const override;
    void cleanup() override {
        std::vector<std::string>().swap(suites_);
    } /// run in the server, after command send to client

private:
    std::vector<std::string> suites_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(this), CEREAL_NVP(suites_));
    }
};

std::ostream& operator<<(std::ostream& os, const SSuitesCmd&);
