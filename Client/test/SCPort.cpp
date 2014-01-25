
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #8 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include "SCPort.hpp"
#include "Str.hpp"
#include "EcfPortLock.hpp"

//#define FIND_FREE_PORT 1

#if FIND_FREE_PORT
#include "ClientInvoker.hpp"
#include "ClientEnvironment.hpp"
#endif

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
 	try {
#if FIND_FREE_PORT
		// Only port hop on local machine
      std::string host = ClientEnvironment::hostSpecified();
      if (host.empty())  host = ecf::Str::LOCALHOST();
      if (host == ecf::Str::LOCALHOST()) {

         std::cout << "SCPort::next() starting port hopping \n";
         ClientInvoker theClient;
         while ( 1 ) {
            std::string thePort = boost::lexical_cast< std::string >(  thePort_ );
            // std::cout << "SCPort::next() trying port " << thePort << "\n";
            theClient.set_host_port(host,thePort);
            try {
               theClient.pingServer();

               // Ping succeeded, need to try next port
               std::cout << "SCPort::next(): Port " << thePort << " is busy trying next port\n";
               thePort_++;
            }
            catch ( std::runtime_error& e ) {
               // Ping failed, We need to distinguish between:
               //    a/ Server does not exist : <FREE> port
               //    b/ Address in use        : <BUSY> port on existing server
               // ******** Until this is done we can't implement port hopping **********
               std::cout << "SCPort::next() , ping failed assuming Free Port " << thePort << " exception " << e.what() <<"\n";
               thePort_++; // for next time
               return thePort;
            }
         }
      }
#endif

      while (!EcfPortLock::is_free(thePort_)) {
         thePort_++;
      }

		// std::cout << "SCPort::next() = " << thePort << "\n";
      std::string thePort = boost::lexical_cast< std::string >(  thePort_ );
		return thePort;
	}
	catch ( boost::bad_lexical_cast& ) {
		BOOST_REQUIRE_MESSAGE(false,"SCPort::next(): Could no generate unique port");
 	}
	return Str::DEFAULT_PORT_NUMBER();
}

}
