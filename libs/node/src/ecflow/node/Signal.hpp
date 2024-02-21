/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_Signal_HPP
#define ecflow_node_Signal_HPP

///
/// \brief The Signal class will, during destruction, un-block SIGCHILD and then
/// reapply the block. During job generation we want to avoid being notified of
/// of child process termination.
/// We want to control when child process termination is handled.
///

namespace ecf {

class Signal {
public:
    Signal();
    // Disable copy (and move) semantics
    Signal(const Signal&)                  = delete;
    const Signal& operator=(const Signal&) = delete;

    /// UNBLOCK SIGCHLD at start of destructor
    /// BLOCK SIGCHLD and the end of the destructor
    /// During the gap in between handle process termination
    ~Signal();

    static void block_sigchild();
    static void unblock_sigchild();
};

} // namespace ecf

#endif /* ecflow_node_Signal_HPP */
