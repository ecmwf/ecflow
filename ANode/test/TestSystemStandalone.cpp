//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision$
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
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>

#include "System.hpp"
#include "Signal.hpp"

using namespace std;
using namespace ecf;

// leap42 boost_1_64_0 gcc-5.3.0 release mode
//    time bin/gcc-5.3.0/release/perf_aparser_only /var/tmp/ma0/BIG_DEFS/vsms2.31415.def
//    real    0m2.79s
//    user    0m2.59s
//    sys     0m0.18s

int main(int argc, char* argv[])
{
   cout << "argc = " << argc << "\n";
   for(int i = 0; i < argc; i++) {
      cout << "arg " << i << ":" << argv[i] << "\n";
   }

   if (argc != 2) {
      cout << "Expect single argument \n";
      return 1;
   }

   cout << "Invoke command: " << argv[1] << "\n";
   std::string errorMsg;
   if (!System::instance()->spawn(argv[1],"", errorMsg)) {
      throw std::runtime_error( errorMsg );
   }

   while (System::instance()->process() !=0 )
   {
      // cout << "no of System::instance()->process() " << System::instance()->process() << "\n";

      // Capture child process termination. Child sends SIGNAl SIGCHLD, caught by parent
      Signal unblock_on_desctruction_then_reblock;
      sleep(1); // Need to wait for child termination()
      System::instance()->processTerminatedChildren();
   }

   return 0;
}
