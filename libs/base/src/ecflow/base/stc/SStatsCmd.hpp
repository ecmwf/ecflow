// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/base/Stats.hpp"
#include "ecflow/base/stc/ServerToClientCmd.hpp"
class AbstractServer;
namespace ecf {
class Visibility;
}

//================================================================================
// Paired with CtsCmd(SERVER_STATS)
// Client---(CtsCmd(SERVER_STATS))---->Server-----(SStatsCmd)--->client:
// ****** Used in Test ONLY, since Stats is subject to change for each release
// ****** see ECFLOW-880, we use CtsCmd(STATS) to return server stats as a string
// ****** this allows the format to change in the server(with out affecting protocol)
//================================================================================
class SStatsCmd final : public ServerToClientCmd {
public:
    explicit SStatsCmd(AbstractServer* as);
    SStatsCmd()
        : ServerToClientCmd() {}

    ///
    /// @brief Builds the reply in the server; the number of suites counts those the requester can read.
    ///
    /// @param[in] as The server
    /// @param[in] visibility The nodes the requester can read; every node when null
    ///
    void init(AbstractServer* as, const ecf::Visibility* visibility = nullptr);

    ///
    /// @brief Returns the statistics carried by the reply.
    ///
    /// @return The statistics.
    ///
    [[nodiscard]] const Stats& stats() const { return stats_; }

    std::string print() const override;
    /// @copydoc ServerToClientCmd::filtering
    [[nodiscard]] ecf::ReplyFiltering filtering() const override { return ecf::ReplyFiltering::Listing; }
    bool equals(ServerToClientCmd*) const override;
    bool handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const override;

private:
    Stats stats_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(this), CEREAL_NVP(stats_));
    }
};

std::ostream& operator<<(std::ostream& os, const SStatsCmd&);
