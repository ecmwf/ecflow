/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Ecf_HPP
#define ecflow_core_Ecf_HPP

///
/// \brief Provides globals used by server for determining change
///

#include <atomic>
#include <iostream>
#include <string>

/**
 * This class holds *global data*, and is used in the server to determine incremental changes to the data model.
 *
 * This data is composed of two parts:
 *  - `state_change_no_`: leads the number of node state changes on the currently loaded defs
 *  - `modify_change_no_`: leads the number of structural changes to the currently loaded defs
 *
 * Each Node/Attribute stores a local `state_change_no_`, and whenever there is a change, this local number is
 * assigned based on the incremented value of the `global state_change_no_`.
 *
 * When making large scale changes (e.g. adding or deleting nodes), we increment the global `modify_change_no_`.
 *
 * The synchronization strategy is such that when the client eventually copies over the full defs, the state_change_no_
 * and modify_change_no_ are also copied. In future synchronization attempts, the client includes these two numbers in
 * the sync request, and thus allows the server to determine what has changed.
 */

class Ecf {
public:
    using counter_t        = unsigned int;
    using atomic_counter_t = std::atomic<counter_t>;

    // Disable default construction
    Ecf() = delete;
    // Disable copy (and move) semantics
    Ecf(const Ecf&)                  = delete;
    const Ecf& operator=(const Ecf&) = delete;

    /// Increment and then return state change no
    static counter_t incr_state_change_no() {
        if (server_) {
            ++state_change_no_;
        }
        return state_change_no_;
    }
    static counter_t state_change_no() {
        return state_change_no_;
    }
    static void set_state_change_no(counter_t x) {
        state_change_no_ = x;
    }

    /// The modify_change_no_ is used for node addition and deletion and re-ordering
    static counter_t incr_modify_change_no() {
        if (server_) {
            ++modify_change_no_;
        }
        return modify_change_no_;
    }
    static counter_t modify_change_no() {
        return modify_change_no_;
    }
    static void set_modify_change_no(counter_t x) {
        modify_change_no_ = x;
    }

    /// Returns true if we are on the server side.
    /// Only in server side do we increment state/modify numbers
    /// Also used in debug/test: Allows print to add know if in server/client
    static bool server() { return server_; }

    /// Should only be set by the server, made public so that testing can also set it
    /// Only in server side do we increment state/modify numbers
    static void set_server(bool f) { server_ = f; }

    static bool debug_equality() { return debug_equality_; }
    static void set_debug_equality(bool f) { debug_equality_ = f; }

    // ECFLOW-99
    static unsigned int debug_level() { return debug_level_; }
    static void set_debug_level(unsigned int level) { debug_level_ = level; }

    static const char* SERVER_NAME();
    static const char* CLIENT_NAME();

    static const std::string& LOG_FILE();
    static const std::string& CHECKPT();
    static const std::string& BACKUP_CHECKPT();
    static const std::string& MICRO();
    static const std::string& JOB_CMD();
    static const std::string& KILL_CMD();
    static const std::string& STATUS_CMD(); // Typical run on the server
    static const std::string& CHECK_CMD();  // Typical run on the client
    static const std::string& URL_CMD();
    static const std::string& URL_BASE();
    static const std::string& URL();

private:
    static bool server_;
    static bool debug_equality_;
    static unsigned int debug_level_;
    static atomic_counter_t state_change_no_;
    static atomic_counter_t modify_change_no_;
};

/// Make sure the Ecf number don't change
class EcfPreserveChangeNo {
public:
    EcfPreserveChangeNo();
    ~EcfPreserveChangeNo();

private:
    unsigned int state_change_no_;
    unsigned int modify_change_no_;
};

class DebugEquality {
public:
    DebugEquality() { Ecf::set_debug_equality(true); }
    // Disable copy (and move) semantics
    DebugEquality(const DebugEquality&)                  = delete;
    const DebugEquality& operator=(const DebugEquality&) = delete;

    ~DebugEquality() {
        Ecf::set_debug_equality(false);
        set_ignore_server_variables(false);
    }

    static bool ignore_server_variables() { return ignore_server_variables_; }
    static void set_ignore_server_variables(bool flg) { ignore_server_variables_ = flg; }

private:
    static bool ignore_server_variables_;
};

#endif /* ecflow_core_Ecf_HPP */
