// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/base/stc/ServerToClientCmd.hpp"

class GroupSTCCmd final : public ServerToClientCmd {
public:
    GroupSTCCmd()
        : ServerToClientCmd() {}

    void cleanup() override; /// After the command has run this function can be used to reclaim memory

    std::string print() const override;
    bool equals(ServerToClientCmd*) const override;
    bool handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const override;

    ///
    /// @brief Appends a reply, snapshotting errors before a preallocated reply is reused.
    ///
    /// @param[in] childCmd Non-null child reply; error children are copied, other replies remain shared.
    /// @throws std::bad_alloc if appending or copying the child fails.
    ///
    void addChild(const STC_Cmd_ptr& childCmd);
    const std::vector<STC_Cmd_ptr>& cmdVec() const { return cmdVec_; }

    // these two must be opposite of each other
    bool ok() const override;
    std::string error() const override;

private:
    std::vector<STC_Cmd_ptr> cmdVec_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(this), CEREAL_NVP(cmdVec_));
    }
};

std::ostream& operator<<(std::ostream& os, const GroupSTCCmd&);
