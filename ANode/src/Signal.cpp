//============================================================================
// Name        : Signal
// Author      : Avi
// Revision    : $Revision: #8 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include "Signal.hpp"
#include <csignal>
#include <iostream>

namespace ecf {

// SIGCHLD : Child status has changed (POSIX)
// Signal sent to parent process whenever one of its child processes terminates or stops.

Signal::Signal() = default;

Signal::~Signal()
{
   // Unblock SIGCHLD. This will call the signal-handler in System.cpp,
   // *IF* we have pending SIGCHLD
   // This will not return until we have handled all the pending SIGCHLD signal
   unblock_sigchild();

   // Once the signals are processed, block them until we come in here again
   // Now block again.
   block_sigchild();
}

void Signal::block_sigchild()
{
   // Now block again.
   sigset_t set;
   sigemptyset( &set );
   sigaddset( &set, SIGCHLD );
#ifdef ECFLOW_MT
   rc = pthread_sigmask(SIG_BLOCK, &set, 0 ); // not tested
   if (rc != 0) std::cerr << "Signal::~Signal(): pthread_sigmask(SIG_UNBLOCK, &set, 0) returned " << rc << "\n";
#else
   sigprocmask( SIG_BLOCK, &set, nullptr );
#endif
}

void Signal::unblock_sigchild()
{
   sigset_t set;
   sigemptyset( &set );
   sigaddset( &set, SIGCHLD );
#ifdef ECFLOW_MT
   int rc = pthread_sigmask(SIG_UNBLOCK, &set, 0 ); // not tested
   if (rc != 0) std::cerr << "Signal::~Signal(): pthread_sigmask(SIG_UNBLOCK, &set, 0) returned " << rc << "\n";
#else
   sigprocmask( SIG_UNBLOCK, &set, nullptr );
#endif
}

}
