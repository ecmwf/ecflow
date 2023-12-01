/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_stc_GroupSTCCmd_HPP
#define ecflow_base_stc_GroupSTCCmd_HPP

#include "ecflow/base/stc/ServerToClientCmd.hpp"

class GroupSTCCmd final : public ServerToClientCmd {
public:
    GroupSTCCmd() : ServerToClientCmd() {}

    void cleanup() override; /// After the command has run this function can be used to reclaim memory

    std::string print() const override;
    bool equals(ServerToClientCmd*) const override;
    bool handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const override;

    void addChild(STC_Cmd_ptr childCmd);
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

#endif /* ecflow_base_stc_GroupSTCCmd_HPP */
