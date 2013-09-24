#ifndef TERMINATESERVER_HPP_
#define TERMINATESERVER_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #21 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <boost/test/unit_test.hpp>
//#include <iostream>

#include "ClientInvoker.hpp"
#include "ClientEnvironment.hpp"
#include "boost/filesystem/operations.hpp"
#include "Str.hpp"
#include "Host.hpp"

class TerminateServer {
public:
	TerminateServer(const std::string& port = ecf::Str::DEFAULT_PORT_NUMBER(),
	                  bool remove_checkpt_file_after_server_exit = true)
    : port_(port),remove_checkpt_file_after_server_exit_(remove_checkpt_file_after_server_exit) {}

	~TerminateServer() {
		std::string host = ClientEnvironment::hostSpecified();
		if (host.empty()) {
	 		// terminate the server, but _only_ if we running locally
			doEnd(ecf::Str::LOCALHOST(), port_, remove_checkpt_file_after_server_exit_);
 		}
	}

	static void doEnd( const std::string& host, const std::string& port, bool remove_checkpt_file_after_server_exit = true ) {
//		std::cout << "*****TerminateServer::doEnd    Closing server on  " << host << ecf::Str::COLON() << port << "\n";
		{
			ClientInvoker theClient(host,port);
			BOOST_REQUIRE_NO_THROW( theClient.terminateServer() );
         BOOST_REQUIRE_MESSAGE( theClient.wait_for_server_death(),"Failed to terminate server after 60 seconds\n");
		}

		// Remove generated file un comment for debug
	 	ecf::Host h;
		boost::filesystem::remove(h.ecf_log_file(port));
      BOOST_CHECK_MESSAGE(!boost::filesystem::exists(h.ecf_log_file(port)), "log file " << h.ecf_log_file(port) << " not deleted\n");

	 	if (remove_checkpt_file_after_server_exit) {
	 	   boost::filesystem::remove(h.ecf_checkpt_file(port));
	 	   boost::filesystem::remove(h.ecf_backup_checkpt_file(port));
	 	   BOOST_CHECK_MESSAGE(!boost::filesystem::exists(h.ecf_checkpt_file(port)), "file " << h.ecf_checkpt_file(port) << " not deleted\n");
	 	   BOOST_CHECK_MESSAGE(!boost::filesystem::exists(h.ecf_backup_checkpt_file(port)), "file " << h.ecf_backup_checkpt_file(port) << " not deleted\n");
	 	}
	}

private:
	std::string port_;
	bool remove_checkpt_file_after_server_exit_;
};

#endif
