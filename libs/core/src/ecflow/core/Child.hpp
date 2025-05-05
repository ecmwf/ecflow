/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Child_HPP
#define ecflow_core_Child_HPP

///
/// \brief Specifies the different kinds of child commands, as specified in the
///        job file and communicated to the server.
///

#include <string>
#include <vector>

namespace ecf {

class Child {
public:
    enum CmdType {
        INIT,    // Notify Task start
        EVENT,   // Notify Task event update
        METER,   // Notify Task meter update
        LABEL,   // Notify Task label update
        WAIT,    // Request to wait until given expression evaluates to true
        QUEUE,   // Request to interact with Queue (active|complete|abort|...)
        ABORT,   // Notify Task abortion
        COMPLETE // Notify Task completion
    };

    enum ZombieType {
        USER,           // zombie created by user action
        ECF,            // two init commands, or aborted and complete, and received any other child command
        ECF_PID,        // pid miss-match, but password matches,  -> same job submitted twice |
        ECF_PASSWD,     // password miss-match, but pid matches   -> WTF, user edited ECF_PASS in job file ?
        ECF_PID_PASSWD, // pid and password missmatch             -> Job re-queued and submitted again
        PATH,           // zombie, because path to task does not exist in the server
        NOT_SET
    };

    // Disable default construction
    Child() = delete;
    // Disable copy (and move) semantics
    Child(const Child&)                  = delete;
    const Child& operator=(const Child&) = delete;

    static std::string to_string(ZombieType);
    static bool valid_zombie_type(const std::string&);
    static ZombieType zombie_type(const std::string&);

    static std::string to_string(const std::vector<Child::CmdType>&);
    static std::string to_string(Child::CmdType);
    static std::vector<Child::CmdType> child_cmds(const std::string&);
    static Child::CmdType child_cmd(const std::string&);

    static std::vector<Child::CmdType> list();

    /// Expect a comma-separated string
    static bool valid_child_cmds(const std::string&);
    static bool valid_child_cmd(const std::string&);
};

} // namespace ecf

#endif /* ecflow_core_Child_HPP */
