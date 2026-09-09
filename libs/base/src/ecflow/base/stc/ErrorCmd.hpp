/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_stc_ErrorCmd_HPP
#define ecflow_base_stc_ErrorCmd_HPP

#include "ecflow/base/stc/ServerToClientCmd.hpp"

class ErrorCmd final : public ServerToClientCmd {
public:
    explicit ErrorCmd(const std::string& errorMsg);
    ErrorCmd()
        : ServerToClientCmd() {}

    void init(const std::string& errorMsg);
    std::string print() const override;
    bool equals(ServerToClientCmd*) const override;
    bool handle_server_response(ServerReply&, Cmd_ptr cts_cmd, bool debug) const override;

    std::string error() const override { return error_msg_; }   /// Used by test
    bool ok() const override { return false; }                  /// Used by group command
    void cleanup() override { std::string{}.swap(error_msg_); } /// run in the server, after command send to client

private:
    std::string error_msg_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(this), CEREAL_NVP(error_msg_));
    }
};

std::ostream& operator<<(std::ostream& os, const ErrorCmd&);

#endif /* ecflow_base_stc_ErrorCmd_HPP */
