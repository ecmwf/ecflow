/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_stc_SNodeCmd_HPP
#define ecflow_base_stc_SNodeCmd_HPP

#include "ecflow/base/stc/ServerToClientCmd.hpp"
class AbstractServer;

//================================================================================
// Paired with CtsNodeCmd(Get)
// Client---CtsNodeCmd(GET)---->Server-----(SNodeCmd | SNodeCmd)--->client:
//================================================================================
class SNodeCmd final : public ServerToClientCmd {
public:
    SNodeCmd(AbstractServer* as, node_ptr node);
    SNodeCmd() = default;

    void init(AbstractServer* as, node_ptr node);

    bool handle_server_response(ServerReply&, Cmd_ptr cts_cmd, bool debug) const override;
    std::string print() const override;
    bool equals(ServerToClientCmd*) const override;
    void cleanup() override { std::string{}.swap(the_node_str_); } /// run in the server, after command send to client

private:
    node_ptr get_node_ptr(std::string& error_msg) const;
    std::string the_node_str_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(this), CEREAL_NVP(the_node_str_));
    }
};

std::ostream& operator<<(std::ostream& os, const SNodeCmd&);

#endif /* ecflow_base_stc_SNodeCmd_HPP */
