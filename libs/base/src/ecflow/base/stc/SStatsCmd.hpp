/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_stc_SStatsCmd_HPP
#define ecflow_base_stc_SStatsCmd_HPP

#include "ecflow/base/Stats.hpp"
#include "ecflow/base/stc/ServerToClientCmd.hpp"
class AbstractServer;

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

    void init(AbstractServer* as);

    std::string print() const override;
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

#endif /* ecflow_base_stc_SStatsCmd_HPP */
