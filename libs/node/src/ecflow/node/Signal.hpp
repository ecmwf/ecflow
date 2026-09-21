// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

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
    Signal(const Signal&)            = delete;
    Signal& operator=(const Signal&) = delete;
    Signal(Signal&&)                 = delete;
    Signal& operator=(Signal&&)      = delete;

    /// UNBLOCK SIGCHLD at start of destructor
    /// BLOCK SIGCHLD and the end of the destructor
    /// During the gap in between handle process termination
    ~Signal();

    static void block_sigchild();
    static void unblock_sigchild();
};

} // namespace ecf
