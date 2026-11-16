// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/base/stc/ServerToClientCmd.hpp"
class AbstractServer;
namespace ecf {
class Visibility;
}

//================================================================================
// Paired with CtsNodeCmd(Get)
// Client---CtsNodeCmd(GET)---->Server-----(SNodeCmd | SNodeCmd)--->client:
//================================================================================
class SNodeCmd final : public ServerToClientCmd {
public:
    SNodeCmd(AbstractServer* as, node_ptr node);
    SNodeCmd() = default;

    ///
    /// @brief Builds the reply in the server: the node and the nodes below it that the requester can read.
    ///
    /// @param[in] as The server
    /// @param[in] node The node requested
    /// @param[in] visibility The nodes the requester can read; every node when null
    /// @throws std::runtime_error when the requester cannot read the node requested: the node is never sent. The
    ///         authorisation refuses such a request first, in every mode; this guards the reply itself.
    ///
    void init(AbstractServer* as, const node_ptr& node, const ecf::Visibility* visibility = nullptr);

    bool handle_server_response(ServerReply&, Cmd_ptr cts_cmd, bool debug) const override;
    std::string print() const override;
    /// @copydoc ServerToClientCmd::filtering
    [[nodiscard]] ecf::ReplyFiltering filtering() const override { return ecf::ReplyFiltering::NodeContent; }
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
