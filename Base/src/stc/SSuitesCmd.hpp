/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_stc_SSuitesCmd_HPP
#define ecflow_base_stc_SSuitesCmd_HPP

#include "ServerToClientCmd.hpp"
class AbstractServer;

//================================================================================
// Paired with CtsCmd(SUITES)
// Client---(CtsCmd(SUITES))---->Server-----(SSuitesCmd)--->client:
//================================================================================
class SSuitesCmd final : public ServerToClientCmd {
public:
    explicit SSuitesCmd(AbstractServer* as);
    SSuitesCmd() : ServerToClientCmd() {}

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
