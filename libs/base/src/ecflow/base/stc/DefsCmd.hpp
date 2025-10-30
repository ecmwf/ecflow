/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_DefsCmd_HPP
#define ecflow_base_cts_DefsCmd_HPP

#include "ecflow/base/stc/DefsCache.hpp"
#include "ecflow/base/stc/ServerToClientCmd.hpp"

class AbstractServer;

//================================================================================
// Paired with CtsNodeCmd(GET)
// Client---CtsNodeCmd(GET)---->Server-----(DefsCmd | SNodeCmd)--->client:
//================================================================================
class DefsCmd final : public ServerToClientCmd {
public:
    explicit DefsCmd(const ecf::Identity& identity, AbstractServer* as, bool save_edit_history = false);
    DefsCmd() = default;

    void init(const ecf::Identity& identity, AbstractServer* as, bool save_edit_history);

    bool handle_server_response(ServerReply&, Cmd_ptr cts_cmd, bool debug) const override;
    std::string print() const override;
    bool equals(ServerToClientCmd*) const override;
    void cleanup() override {
        std::string().swap(full_server_defs_as_string_);
    } /// run in the server, after command send to client

private:
    std::string full_server_defs_as_string_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(this));

        if (Archive::is_saving::value) {
            // Avoid copying the string. As this could be very large
            ar& DefsCache::full_server_defs_as_string_;
        }
        else {
            ar & full_server_defs_as_string_;
        }
    }
};

std::ostream& operator<<(std::ostream& os, const DefsCmd&);

#endif /* ecflow_base_cts_DefsCmd_HPP */
