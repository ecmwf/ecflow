// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/base/Cmd.hpp"
#include "ecflow/base/ServerReply.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/NodeFwd.hpp"

namespace ecf {

///
/// @brief Tells how the content of a reply is filtered for its requester (R5.11).
///
enum class ReplyFiltering {
    None,        ///< No data item about nodes, or data accepted unfiltered (D5.20, D5.21, D5.26, D5.28)
    NodeContent, ///< Definitions or nodes: the nodes the requester cannot read are left out
    Listing      ///< Names or counts of nodes: computed from the nodes the requester can read
};

} // namespace ecf

//================================================================================
// Start of Server->client
//================================================================================
class ServerToClientCmd {
public:
    virtual ~ServerToClientCmd();

    virtual void cleanup() {} /// After the command has run this function can be used to reclaim memory

    virtual std::string print() const = 0;
    virtual bool equals(ServerToClientCmd*) const { return true; }

    virtual const std::string& get_string() const;
    virtual bool ok() const { return true; }
    virtual bool is_returnable_in_group_cmd() const { return true; }

    virtual std::string error() const { return std::string{}; } /// Used by test

    ///
    /// @brief Returns how the content of the reply is filtered for its requester.
    ///
    /// Every reply type declares it, so that a type added without a filtering does not build (R7.7).
    ///
    /// @return The filtering.
    ///
    [[nodiscard]] virtual ecf::ReplyFiltering filtering() const = 0;

    // Called in client, if any data to be returned , the set on class ServerReply
    // Cmd_ptr cts_cmd  can used for additional context.
    // return true, if client should exit, returns false, if further work required, ie blocking, errors, etc
    virtual bool handle_server_response(ServerReply&, Cmd_ptr cts_cmd, bool debug) const = 0;

protected:
    ServerToClientCmd() = default;

private:
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {}
};
