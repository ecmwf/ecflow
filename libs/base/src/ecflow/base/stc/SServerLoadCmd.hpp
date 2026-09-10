/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_stc_SServerLoadCmd_HPP
#define ecflow_base_stc_SServerLoadCmd_HPP

#include "ecflow/base/stc/ServerToClientCmd.hpp"

///================================================================================
/// Paired with CtsCmd::SERVER_LOAD
/// Client---(CtsCmd::SERVER_LOAD)---->Server-----(SServerLoadCmd)--->client:
///================================================================================
class SServerLoadCmd final : public ServerToClientCmd {
public:
    explicit SServerLoadCmd(const std::string& log_file_path)
        : log_file_path_(log_file_path) {}
    SServerLoadCmd()
        : ServerToClientCmd() {}

    void init(const std::string& s) { log_file_path_ = s; }
    const std::string& log_file_path() const { return log_file_path_; }
    std::string print() const override;
    bool equals(ServerToClientCmd*) const override;
    bool handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const override;
    void cleanup() override { std::string{}.swap(log_file_path_); } /// run in the server, after command send to client

private:
    std::string log_file_path_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(this), CEREAL_NVP(log_file_path_));
    }
};

std::ostream& operator<<(std::ostream& os, const SServerLoadCmd&);

#endif /* ecflow_base_stc_SServerLoadCmd_HPP */
