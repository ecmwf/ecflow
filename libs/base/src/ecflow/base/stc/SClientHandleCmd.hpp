/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_stc_SClientHandleCmd_HPP
#define ecflow_base_stc_SClientHandleCmd_HPP

#include "ecflow/base/stc/ServerToClientCmd.hpp"

class SClientHandleCmd final : public ServerToClientCmd {
public:
    explicit SClientHandleCmd(int handle)
        : handle_(handle) {}
    SClientHandleCmd()
        : ServerToClientCmd() {}

    void init(int handle) { handle_ = handle; }
    std::string print() const override;
    bool equals(ServerToClientCmd*) const override;
    bool handle_server_response(ServerReply&, Cmd_ptr cts_cmd, bool debug) const override;

private:
    int handle_{0};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(this), CEREAL_NVP(handle_));
    }
};

std::ostream& operator<<(std::ostream& os, const SClientHandleCmd&);

#endif /* ecflow_base_stc_SClientHandleCmd_HPP */
