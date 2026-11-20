// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/base/stc/DefsCache.hpp"
#include "ecflow/base/stc/ServerToClientCmd.hpp"

class AbstractServer;

//================================================================================
// Paired with CtsNodeCmd(GET)
// Client---CtsNodeCmd(GET)---->Server-----(DefsCmd | SNodeCmd)--->client:
//================================================================================
class DefsCmd final : public ServerToClientCmd {
public:
    explicit DefsCmd(AbstractServer* as, bool save_edit_history = false);
    DefsCmd() = default;

    ///
    /// @brief Builds the reply in the server: the definitions as the requester can read them, refreshed in the cache.
    ///
    /// @param[in] as The server
    /// @param[in] save_edit_history Whether the edit history is included
    /// @param[in] visibility The nodes the requester can read; every node when null
    ///
    void init(AbstractServer* as, bool save_edit_history, const ecf::Visibility* visibility = nullptr);

    bool handle_server_response(ServerReply&, Cmd_ptr cts_cmd, bool debug) const override;
    std::string print() const override;
    /// @copydoc ServerToClientCmd::filtering
    [[nodiscard]] ecf::ReplyFiltering filtering() const override { return ecf::ReplyFiltering::NodeContent; }
    bool equals(ServerToClientCmd*) const override;
    void cleanup() override {
        std::string{}.swap(full_server_defs_as_string_);
        cached_.reset();
    } /// run in the server, after command send to client

private:
    std::string full_server_defs_as_string_;    // client side: the definitions received
    std::shared_ptr<const std::string> cached_; // server side: the entry of the cache sent

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(this));

        if constexpr (Archive::is_saving::value) {
            // Avoid copying the string. As this could be very large
            if (cached_) {
                ar(*cached_);
            }
            else {
                ar & full_server_defs_as_string_;
            }
        }
        else {
            ar & full_server_defs_as_string_;
        }
    }
};

std::ostream& operator<<(std::ostream& os, const DefsCmd&);
