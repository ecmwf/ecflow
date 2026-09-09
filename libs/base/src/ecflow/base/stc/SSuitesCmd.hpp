/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_stc_SSuitesCmd_HPP
#define ecflow_base_stc_SSuitesCmd_HPP

#include "ecflow/base/stc/ServerToClientCmd.hpp"
class AbstractServer;

//================================================================================
// Paired with CtsCmd(SUITES)
// Client---(CtsCmd(SUITES))---->Server-----(SSuitesCmd)--->client:
//================================================================================
class SSuitesCmd final : public ServerToClientCmd {
public:
    explicit SSuitesCmd(AbstractServer* as);
    SSuitesCmd()
        : ServerToClientCmd() {}

    void init(AbstractServer* as);
    std::string print() const override;
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

#endif /* ecflow_base_stc_SSuitesCmd_HPP */
