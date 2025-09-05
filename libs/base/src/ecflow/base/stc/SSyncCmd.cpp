/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/stc/SSyncCmd.hpp"

#include <iostream>

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/node/Defs.hpp"

using namespace ecf;
using namespace std;
using namespace boost;

// =====================================================================================================
// #define DEBUG_SERVER_SYNC 1  # Also update DefsCache::DEBUG_SERVER_SYNC
// #define DEBUG_CLIENT_SYNC 1  # Also update DefsCache::DEBUG_CLIENT_SYNC

SSyncCmd::SSyncCmd(unsigned int client_handle,
                   unsigned int client_state_change_no,
                   unsigned int client_modify_change_no,
                   AbstractServer* as)
    : incremental_changes_(client_state_change_no) {
    init(client_handle, client_state_change_no, client_modify_change_no, false, false, as);
}

void SSyncCmd::reset_data_members(unsigned int client_state_change_no, bool sync_suite_clock) {
    full_defs_ = false;
    incremental_changes_.init(client_state_change_no,
                              sync_suite_clock); // persisted, used for returning INCREMENTAL changes
    server_defs_.clear();                        // persisted, used for returning FULL definition
    full_server_defs_as_string_.clear(); // semi-persisted, i.e on load & not on saving used to return cached defs
}

void SSyncCmd::init(unsigned int client_handle, // a reference to a set of suites used by client
                    unsigned int client_state_change_no,
                    unsigned int client_modify_change_no,
                    bool do_full_sync,
                    bool sync_suite_clock,
                    AbstractServer* as) {
    // ********************************************************
    // This is called in the server
    // ********************************************************
#ifdef DEBUG_SERVER_SYNC
    cout << "SSyncCmd::init: client(" << client_state_change_no << "," << client_modify_change_no << ") server("
         << Ecf::state_change_no() << "," << Ecf::modify_change_no() << ") sync_suite_clock(" << sync_suite_clock
         << ")";
#endif

    // Reset all data members since this command can be re-used
    reset_data_members(client_state_change_no, sync_suite_clock);

    // explicit request
    if (do_full_sync) {
#ifdef DEBUG_SERVER_SYNC
        cout << ": *Flag do_full_sync set* ";
#endif
        full_sync(client_handle, as);
        return;
    }

    if (client_handle == 0) {
#ifdef DEBUG_SERVER_SYNC
        cout << ": No *handle* ";
#endif

        /// *** The client_modify_change_no and client_state_change_no should always be trailing the server
        /// *** i.e the value should be less or equal to server. However if the server **dies** we can
        /// *** get the case, where client numbers are greater than server numbers.
        /// *** When no handle are involved, we get by with a full sync
        /// *** Note: whenever the server starts, the state and modify numbers start from zero
        if (client_modify_change_no > Ecf::modify_change_no() || client_state_change_no > Ecf::state_change_no()) {
#ifdef DEBUG_SERVER_SYNC
            cout << ": client modify no > server modify no: Server died/restored? ";
#endif
            full_sync(client_handle, as);
            return;
        }

        if (client_modify_change_no < Ecf::modify_change_no()) {
#ifdef DEBUG_SERVER_SYNC
            cout << ": *Large* scale changes: modify numbers not in sync ";
#endif
            full_sync(client_handle, as);
            return;
        }

        //      // small scale changes.
        //      // When we going to get thousand of memento, quicker to do a full sync
        //      if (Ecf::state_change_no()-client_state_change_no > 200000) {
        //         full_sync(client_handle,as);
        //         return;
        //      }

        // small scale changes. Collate changes over *defs* and all suites.
        // Suite stores the maximum state change, over *all* its children, this is used by client handle mechanism
        // and here to avoid traversing down the hierarchy.
        // ******** We must trap all child changes under the suite. See class SuiteChanged
        // ******** otherwise some attribute sync's will be missed
        as->defs()->collateChanges(client_handle, incremental_changes_);
        incremental_changes_.set_server_state_change_no(Ecf::state_change_no());
        incremental_changes_.set_server_modify_change_no(Ecf::modify_change_no());
#ifdef DEBUG_SERVER_SYNC
        if (incremental_changes_.size()) {
            cout << ":*small* scale changes: no of changes(" << incremental_changes_.size() << ")\n";
        }
        else {
            cout << ": *No changes*\n";
        }
#endif
        //      LOG(Log::DBG,"SSyncCmd::init incremental_changes_.size() " << incremental_changes_.size() << "
        //      Ecf::state_change_no()-client_state_change_no " << Ecf::state_change_no()-client_state_change_no );
        return;
    }

    //==========================================================================================
    // Handle used
    //==========================================================================================
    ClientSuiteMgr& client_suite_mgr = as->defs()->client_suite_mgr();
#ifdef DEBUG_SERVER_SYNC
    cout << ": *handle* used " << client_handle << " : ";
    std::vector<string> suites;
    client_suite_mgr.suites(client_handle, suites);
    for (const std::string& name : suites) {
        std::cout << name << " ";
    }
#endif

    /// *** The client_modify_change_no and client_state_change_no should always be trailing the server
    /// *** i.e the value should be less or equal to server. However if the server **dies** we can
    /// *** get the case, where client numbers are greater than server numbers. In this case we do a full sync
    /// *** It is up to the client to catch the exception. and do a full sync *including* re-registering suites
    /// *** Note: whenever the server starts, the state and modify numbers start from zero
    unsigned int max_client_handle_modify_change_no = 0;
    unsigned int max_client_handle_state_change_no  = 0;
    client_suite_mgr.max_change_no(
        client_handle, max_client_handle_state_change_no, max_client_handle_modify_change_no);
#ifdef DEBUG_SERVER_SYNC
    cout << ": server_handle(" << max_client_handle_state_change_no << "," << max_client_handle_modify_change_no << ")";
#endif

    if (client_modify_change_no > max_client_handle_modify_change_no ||
        client_state_change_no > max_client_handle_state_change_no) {
#ifdef DEBUG_SERVER_SYNC
        cout << ": client no > server no: Server died/restored?";
#endif
        full_sync(client_handle, as);
        return;
    }

    if (client_modify_change_no < max_client_handle_modify_change_no) {
#ifdef DEBUG_SERVER_SYNC
        cout << ": *Large* scale changes : modify numbers not in sync";
#endif
        full_sync(client_handle, as);
        return;
    }

    if (client_suite_mgr.handle_changed(client_handle)) {
        // *Large* scale handle changes, i.e. created handle/ added or removed suites
#ifdef DEBUG_SERVER_SYNC
        cout << ": *Large* scale changes: added/removed suites to handle";
#endif
        full_sync(client_handle, as);
        return;
    }

    // small scale changes
    as->defs()->collateChanges(client_handle, incremental_changes_);
    incremental_changes_.set_server_state_change_no(max_client_handle_state_change_no);
    incremental_changes_.set_server_modify_change_no(max_client_handle_modify_change_no);
#ifdef DEBUG_SERVER_SYNC
    if (incremental_changes_.size()) {
        cout << ": *small* scale changes: no of changes(" << incremental_changes_.size() << ")\n";
    }
    else {
        cout << ": *No changes*\n";
    }
#endif
}

void SSyncCmd::full_sync(unsigned int client_handle, AbstractServer* as) {
    Defs* server_defs = as->defs().get();

    if (0 == client_handle) {
        // Have already checked for no defs.
        server_defs->set_state_change_no(Ecf::state_change_no());
        server_defs->set_modify_change_no(Ecf::modify_change_no());

        DefsCache::update_cache_if_state_changed(server_defs);
        full_defs_ = true;
#ifdef DEBUG_SERVER_SYNC
        cout << ": *no handle* returning FULL defs(*cached* string, size("
             << DefsCache::full_server_defs_as_string_.size() << "))" << endl;
#endif
        return;
    }

#ifdef DEBUG_SERVER_SYNC
    cout << ": returning handle based FULL defs";
#endif
    // Only return the defs state and suites that the client has registered in the client handle
    // *HOWEVER* if the client has registered *ALL* the suites, just return the server defs
    //           *with* the updated change numbers
    //
    // *OTHERWISE* this compute the **maximum** state and modify change numbers over
    // the suites managed by the client handle and then set it on the newly created defs file.
    // **** It also takes special precaution *NOT* to change Ecf::state_change_no() and Ecf::modify_change_no()
    // **** so that we avoid changing max modify no for suites not in our handle
    // **** Will clear the handle_changed flag
    //
    // **** Although we create a new defs, we use the same suites. This presents
    // **** a problem with the suites defs() pointer. To avoid corrupting the server defs
    // **** we set the suite defs ptr to the the real server defs.
    // **** --> The defs serialisation will setup the suite defs pointers. <---
    // **** An alternative would be to clone the entire suites, since this can have
    // **** hundreds of tasks. It would be very expensive.
    // **** This means that server_defs_ will fail invarint_checking before serialisation
    defs_ptr the_server_defs = server_defs->client_suite_mgr().create_defs(client_handle, as->defs());
    if (the_server_defs.get() == server_defs) {
        DefsCache::update_cache_if_state_changed(server_defs);
        full_defs_ = true;
#ifdef DEBUG_SERVER_SYNC
        cout << ": The handle has *ALL* the suites: return the FULL defs(*cached* string, size("
             << DefsCache::full_server_defs_as_string_.size() << "))";
#endif
    }
    else {
        the_server_defs->write_to_string(server_defs_, PrintStyle::NET);
    }

#ifdef DEBUG_SERVER_SYNC
    if (the_server_defs) {
        cout << ": no of suites(" << the_server_defs->suiteVec().size() << ")" << endl;
    }
    else {
        cout << ": NULL defs!" << endl;
    }
#endif
}

void SSyncCmd::cleanup() {
    /// run in the server, after command sent to client
    incremental_changes_.cleanup();
    std::string().swap(server_defs_);
    std::string().swap(full_server_defs_as_string_); // will typically be empty in server
}

bool SSyncCmd::equals(ServerToClientCmd* rhs) const {
    auto* the_rhs = dynamic_cast<SSyncCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    return ServerToClientCmd::equals(rhs);
}

std::string SSyncCmd::print() const {
    return "cmd:SSyncCmd";
}

bool SSyncCmd::handle_server_response(ServerReply& server_reply, Cmd_ptr /*cts_cmd*/, bool debug) const {
    // Update server_reply.client_defs_, with the changes returned from the server
    do_sync(server_reply, debug);

    return true;
}

bool SSyncCmd::do_sync(ServerReply& server_reply, bool debug) const {
    // ****************************************************
    // On the client side
    // ****************************************************
    if (!server_defs_.empty()) {
        // *FULL* sync
        // to keep pace with the state changes. Passed back later on, get further changes
        // If non zero handle will contain suites specified in the client handle, including their max change numbers
        defs_ptr server_defs = Defs::create();
        server_defs->restore_from_string(server_defs_); // this can throw

        server_reply.client_defs_ = server_defs;
        server_reply.set_sync(true);
        server_reply.set_full_sync(true);
        if (debug) {
            cout << "  SSyncCmd::do_sync::*FULL sync*, client side state/modify numbers("
                 << server_defs->state_change_no() << "," << server_defs->modify_change_no() << ")\n";
        }
#ifdef DEBUG_CLIENT_SYNC
        cout << "SSyncCmd::do_sync: defs *FULL sync*, client side state/modify numbers("
             << server_defs->state_change_no() << "," << server_defs->modify_change_no() << ")\n";
#endif
        return true;
    }

    if (full_defs_) {
        if (full_server_defs_as_string_.empty()) {
            // TEST path.(i.e no client server): re-use static string
#ifdef DEBUG_CLIENT_SYNC
            cout << "SSyncCmd::do_sync: TEST PATH: *FULL CACHE sync* : using static cache";
#endif
            server_reply.client_defs_ = DefsCache::restore_defs_from_string();
        }
        else {
#ifdef DEBUG_CLIENT_SYNC
            cout << "SSyncCmd::do_sync: *FULL CACHE sync* : using cache returned from server: cache_size("
                 << full_server_defs_as_string_.size() << ")";
#endif
            server_reply.client_defs_ = DefsCache::restore_defs_from_string(full_server_defs_as_string_);
        }
        server_reply.set_sync(true);
        server_reply.set_full_sync(true);
        if (debug) {
            cout << "  SSyncCmd::do_sync::*FULL CACHE sync*, client side state/modify numbers("
                 << server_reply.client_defs_->state_change_no() << "," << server_reply.client_defs_->modify_change_no()
                 << ")\n";
        }
#ifdef DEBUG_CLIENT_SYNC
        cout << ": client side state/modify numbers(" << server_reply.client_defs_->state_change_no() << ","
             << server_reply.client_defs_->modify_change_no() << ")\n";
#endif
        return true;
    }

    // Can only sync, *if* we have definition on the client side
    if (server_reply.client_defs_.get()) {
        // *INCREMENTAL* sync

        if (server_reply.client_defs_->in_notification()) {
            // For debug: place a break point here: It appear as Change manager observers, has called another client to
            // server command
            std::cout << "SSyncCmd::do_sync ERROR!!!!! called in the middle of notification(server->client sync)\n";
            std::cout << "It appears that change observer have called *ANOTHER* client->server command in the middle "
                         "synchronising client definition\n";
        }
        /// - Sets notification flag, so that observers can also query if they are in the middle of notification.
        ChangeStartNotification start_notification(server_reply.client_defs_);

        // Apply mementos to the client side defs, to bring in sync with server defs
        // If *no* server loaded, then no changes applied
        // Returns true if are any memento's, i.e. server changed.
        server_reply.set_full_sync(false);
        bool changes_made_to_client = incremental_changes_.incremental_sync(
            server_reply.client_defs_, server_reply.changed_nodes(), server_reply.client_handle());
        server_reply.set_sync(changes_made_to_client);

        if (debug) {
            cout << "  SSyncCmd::do_sync::*INCREMENTAL sync*, client side state/modify numbers("
                 << incremental_changes_.get_server_state_change_no() << ","
                 << incremental_changes_.get_server_modify_change_no() << ") changes_made_to_client("
                 << changes_made_to_client << ")\n";
        }

#ifdef DEBUG_CLIENT_SYNC
        cout << "SSyncCmd::do_sync::*INCREMENTAL sync*, client side state/modify numbers("
             << incremental_changes_.get_server_state_change_no() << ","
             << incremental_changes_.get_server_modify_change_no() << ") changes_made_to_client("
             << changes_made_to_client << "), changed_node_paths(" << server_reply.changed_nodes().size() << ")\n";
#endif
        return changes_made_to_client;
    }
    return false;
}

std::ostream& operator<<(std::ostream& os, const SSyncCmd& c) {
    os << c.print();
    return os;
}
