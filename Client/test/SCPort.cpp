
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
   std::stringstream ss;
   ss << "\nSCPort::next() : ";

   // Allow parallel tests
   char* ecf_port = getenv("TEST_ECF_PORT");
   if ( ecf_port )  {
      ss << " TEST_ECF_PORT=" << ecf_port;
      std::string port = ecf_port;
      try {
         thePort_ = boost::lexical_cast<int>(port);
      }
      catch (...){
         std::cout << "SCPort::next()  TEST_ECF_PORT(" << ecf_port  << ") not convertible to an integer\n";
      }
   }

   // This is used to test remote servers(or legacy server with new client). Here ECF_HOST=localhost in the test scripts
   std::string host = ClientEnvironment::hostSpecified();
   ss << " ECF_HOST=" << host;
   if ( host == Str::LOCALHOST() ) {

      std::string port;
      char* ecf_port = getenv("ECF_PORT");
      if ( ecf_port )  {
         port = ecf_port;
         ss << " ECF_PORT=" << ecf_port;
      }
      if (!port.empty()) {
         std::cout << ss.str() << "\n";
         return port;
      }
   }

   std::string the_port = next_only();
   ss << " returning free port=" << the_port;
   std::cout << ss.str() << "\n";
   return the_port;
}

std::string SCPort::next_only()
{
   // Use a combination of local lock file, and pinging the server
   while (!EcfPortLock::is_free(thePort_)) thePort_++;

   // std::cout << "SCPort::next() = " << thePort << "\n";
   return ClientInvoker::find_free_port(thePort_);
}

}
