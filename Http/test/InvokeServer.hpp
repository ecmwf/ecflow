#ifndef INVOKESERVER_HPP_
#define INVOKESERVER_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $ 
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <iostream>
#include <thread>

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>

#include "ClientInvoker.hpp"
#include "EcfPortLock.hpp"
#include "Host.hpp"
#include "Server.hpp"
#include "ServerEnvironment.hpp"
#include "Str.hpp"
#include "TestHelper.hpp"

class InvokeServer {
public:
   InvokeServer()
   {
        std::string port(getenv("ECF_PORT"));
      /// Remove check pt and backup check pt file, else server will load it & remove log file
        ecf::Host h;
        boost::filesystem::remove(h.ecf_checkpt_file(port));
        boost::filesystem::remove(h.ecf_backup_checkpt_file(port));
        boost::filesystem::remove(h.ecf_log_file(port));

        std::string theServerInvokePath = ecf::File::find_ecf_server_path();

        BOOST_REQUIRE_MESSAGE(!theServerInvokePath.empty(),"The server program could not be found");
        BOOST_REQUIRE_MESSAGE(boost::filesystem::exists(theServerInvokePath),"Server exe does not exist at:" << theServerInvokePath);

        BOOST_TEST_MESSAGE("Using eclow_server from " << theServerInvokePath);

        std::thread t([&]{
            system(theServerInvokePath.c_str());
        });

        t.detach();

        sleep(1);

        ClientInvoker theClient("localhost", port);
        if (theClient.wait_for_server_reply()) {
            theClient.restartServer();
            server_started = true;
        }
        else {
            server_started = false;
        }
   }
    
   ~InvokeServer() {
      std::string port(getenv("ECF_PORT"));

      BOOST_TEST_MESSAGE("*****InvokeServer:: Closing server on port " << port);
      {
         ClientInvoker theClient("localhost",port);
         BOOST_REQUIRE_NO_THROW( theClient.terminateServer() );
         BOOST_REQUIRE_MESSAGE( theClient.wait_for_server_death(),"Failed to terminate server after 60 seconds\n");
      }

      ecf::Host h;
      boost::filesystem::remove(h.ecf_log_file(port));
      BOOST_CHECK_MESSAGE(!boost::filesystem::exists(h.ecf_log_file(port)), "log file " << h.ecf_log_file(port) << " not deleted\n");

      boost::filesystem::remove(h.ecf_checkpt_file(port));
      boost::filesystem::remove(h.ecf_backup_checkpt_file(port));
      BOOST_CHECK_MESSAGE(!boost::filesystem::exists(h.ecf_checkpt_file(port)), "file " << h.ecf_checkpt_file(port) << " not deleted\n");
      BOOST_CHECK_MESSAGE(!boost::filesystem::exists(h.ecf_backup_checkpt_file(port)), "file " << h.ecf_backup_checkpt_file(port) << " not deleted\n");
   }

   bool server_started{false};
};

#endif
