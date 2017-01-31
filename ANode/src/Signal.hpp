#ifndef SIGNAL_HPP_
#define SIGNAL_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #7 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description : During destruction will un-block SIGCHILD and then reblock
//               During job generation we want to avoid being notified of
//               of child process termination.
//               We want to control when child process termination is handled
// Collaboration: System.hpp
//============================================================================
#include <boost/noncopyable.hpp>

namespace ecf {

class Signal : private boost::noncopyable {
public:
   Signal();

   /// UNBLOCK SIGCHLD at start of destructor
   /// BLOCK SIGCHLD and the end of the destructor
   /// During the gap in between handle process termination
   ~Signal();

   static void block_sigchild();
   static void unblock_sigchild();
};
}
#endif
