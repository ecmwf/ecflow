/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_stc_SSyncCmd_HPP
#define ecflow_base_stc_SSyncCmd_HPP

#include "ecflow/base/stc/DefsCache.hpp"
#include "ecflow/base/stc/ServerToClientCmd.hpp"
#include "ecflow/node/DefsDelta.hpp"

class AbstractServer;

///
/// \brief the class SSyncCmd is used to transfer changes made in the server to the client.
///  The client can then apply the changes to the client side defs.
///
/// *** This class should be used in conjunction with the news command.
/// *** i.e The news command is used to test for server changes. This command
/// *** will then get those changes and merge them with client side defs, bringing
/// *** client and server defs in sync.
///
/// The *client_state_change_no* was passed from the client to the server
/// The *client_modify_change_no* was passed from the client to the server
///
/// This class make use of DefsCache as a performance optimisation.
///

class SSyncCmd final : public ServerToClientCmd {
public:
    // The constructor is *called* in the server.
    // This will collate the incremental changes made so far relative to the client_state_change_no.
    // For large scale change we use client_modify_change_no this will require a full update
    SSyncCmd(unsigned int client_handle, // a reference to a set of suites used by client
             unsigned int client_state_change_no,
             unsigned int client_modify_change_no,
             AbstractServer* as);

    SSyncCmd() : ServerToClientCmd(), incremental_changes_(0) {}

    std::string print() const override;
    bool equals(ServerToClientCmd*) const override;

    // Client side functions:
    bool handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const override;

    /// do_sync() is invoked on the *client side*, Can throw std::runtime_error
    /// Either does a *FULL* or *INCREMENTAL sync depending on the
    /// changes in the server. Returns true if client defs changed.
    bool do_sync(ServerReply& server_reply, bool debug = false) const;

private:
    friend class PreAllocatedReply;
    void init(unsigned int client_handle, // a reference to a set of suites used by client
              unsigned int client_state_change_no,
              unsigned int client_modify_change_no,
              bool full_sync,
              bool sync_suite_clock,
              AbstractServer* as);

    /// For use when doing a full sync
    void init(unsigned int client_handle, AbstractServer* as);

    void reset_data_members(unsigned int client_state_change_no, bool sync_suite_clock);
    void full_sync(unsigned int client_handle, AbstractServer* as);
    void cleanup() override; /// run in the server, after command sent to client

private:
    bool full_defs_{false};
    DefsDelta incremental_changes_;
    std::string server_defs_;                // for returning a subset of the suites
    std::string full_server_defs_as_string_; // semi-persisted, i.e on load & not on saving, used to return cached defs

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(this),
           CEREAL_NVP(full_defs_),           // returning full defs as a string
           CEREAL_NVP(incremental_changes_), // state changes, small scale changes

           /// When the server_defs_ was created the def's pointer on the suites was reset back to real server defs
           /// This is not correct for server_defs_, since we use the *same* suites
           /// **** This is OK since by default the Defs serialisation will fix up the suite's def's pointers ***
           /// The alternative is to clone all the suites, which is very expensive
           CEREAL_NVP(server_defs_)); // large scale changes, if non zero handle, a small subset of the suites

        // when full_defs_ is set 'server_defs_' will be empty.
        if (Archive::is_saving::value) {
            // Avoid copying the string. As this could be very large
            if (full_defs_) {
                ar& DefsCache::full_server_defs_as_string_;
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

std::ostream& operator<<(std::ostream& os, const SSyncCmd&);

#endif /* ecflow_base_stc_SSyncCmd_HPP */
