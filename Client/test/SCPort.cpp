
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #8 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <iostream>
#include "SCPort.hpp"
#include "EcfPortLock.hpp"
#include "ClientInvoker.hpp"
#include "ClientEnvironment.hpp"
#include "Str.hpp"

namespace ecf {

// init the globals. Note we dont use 3141, so that in the case where we already
// have a remote/local server started external to the test, it does not clash
// Also debug and release version use different port numbers to avoid clashes, if both tests run at same time

#ifdef DEBUG
int SCPort::thePort_ = 3161;
#else
int SCPort::thePort_ = 3142;
#endif


std::string SCPort::next()
{
   bool debug = false;
   if ( getenv("ECF_DEBUG_TEST") ) debug = true;

   if (debug) std::cout << "\nSCPort::next() : ";

   // Allow parallel tests
   char* ecf_port = getenv("TEST_ECF_PORT");
   if ( ecf_port )  {
      if (debug) std::cout << " seed_port=TEST_ECF_PORT=(" << ecf_port << ")";
      std::string port = ecf_port;
      try { thePort_ = boost::lexical_cast<int>(port);}
      catch (...){ std::cout << "SCPort::next()  TEST_ECF_PORT(" << ecf_port  << ") not convertible to an integer\n";}
   }


   // This is used to test remote servers(or legacy server with new client). Here ECF_HOST=localhost in the test scripts
   std::string host = ClientEnvironment::hostSpecified();
   if (debug) std::cout << " ECF_HOST('" << host << "')";

   if ( host == Str::LOCALHOST() ) {
      char* ecf_port = getenv("ECF_PORT");
      if ( ecf_port )  {
         std::string port = ecf_port;
         if (!port.empty()) {
            if (debug) std::cout << " ECF_PORT('" << ecf_port << "')\n";
            return port;
         }
      }
      if (debug) std::cout << " !!!!!! ERROR when ECF_HOST=localhost  EXPECTED ECF_PORT to be set !!!!!! ";
   }

   if (debug) std::cout << "\n";
   std::string the_port = next_only(debug);
   if (debug) std::cout << " SCPort::next() returning free port=" << the_port << "\n";
   return the_port;
}

std::string SCPort::next_only(bool debug)
{
   if (debug) std::cout << " SCPort::next_only : starting seed_port(" << thePort_ << ")\n";

   // Use a combination of local lock file, and pinging the server
   while (!EcfPortLock::is_free(thePort_,debug)) thePort_++;

   if (debug) std::cout << " SCPort::next_only() seed_port(" << thePort_ << ")\n";
   return ClientInvoker::find_free_port(thePort_,debug);
}

}
