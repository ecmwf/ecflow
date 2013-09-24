#ifndef INVOKESERVER_HPP_
#define INVOKESERVER_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $ 
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
#include <iostream>
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include <boost/noncopyable.hpp>

#include "TerminateServer.hpp"
#include "TestHelper.hpp"
#include "ClientInvoker.hpp"
#include "Str.hpp"
#include "File.hpp"
#include "Host.hpp"

class InvokeServer : private boost::noncopyable {
public:
	InvokeServer(const std::string& msg,
	               const std::string& port = ecf::Str::DEFAULT_PORT_NUMBER(),
	               bool disable_job_generation = false,
                  bool remove_checkpt_file_before_server_start = true,
                  bool remove_checkpt_file_after_server_exit = true
	             ) : port_(port),remove_checkpt_file_after_server_exit_(remove_checkpt_file_after_server_exit)  {

 		host_ = ClientEnvironment::hostSpecified();
		if (host_.empty()) {
			if(!msg.empty()) std::cout << msg << "   port(" << port_ << ")\n";

			host_ = ecf::Str::LOCALHOST();

			doStart(port_,disable_job_generation,remove_checkpt_file_before_server_start);
		}
		else {
			// Start of test, clear any existing defs on remote server
			// Assuming this has been started on DEFAULT_PORT_NUMBER, can't use existing port_
			port_ = ecf::Str::DEFAULT_PORT_NUMBER();
			std::string test_name = msg;
         test_name += " on ";
         test_name += host_;
         test_name += ecf::Str::COLON();
         test_name += port_;

			std::cout <<test_name << "\n";

			ClientInvoker theClient(host_,port_);
		   theClient.logMsg( test_name );
		   BOOST_REQUIRE_MESSAGE( theClient.delete_all() == 0,CtsApi::to_string(CtsApi::delete_node()) << " failed should return 0. Should Delete ALL existing defs in the server\n" << theClient.errorMsg());
		}
	}

	// This will also remove the generated files. Please see TerminateServer
	~InvokeServer() { TerminateServer terminateServer(port_,remove_checkpt_file_after_server_exit_); }

	const std::string& port() const { return port_; }
	const std::string& host() const { return host_; }

	std::string ecf_log_file() const            { return host_name_.ecf_log_file(port_);}
 	std::string ecf_checkpt_file() const        { return host_name_.ecf_checkpt_file(port_); }
	std::string ecf_backup_checkpt_file() const { return host_name_.ecf_backup_checkpt_file(port_); }

	static void doStart(const std::string& port,bool disable_job_generation = false, bool remove_checkpt_file_before_server_start = true)
	{

		/// Remove check pt and backup check pt file, else server will load it & remove log file
		ecf::Host h;
		if (remove_checkpt_file_before_server_start) {
		   boost::filesystem::remove(h.ecf_checkpt_file(port));
		   boost::filesystem::remove(h.ecf_backup_checkpt_file(port));
		}
		boost::filesystem::remove(h.ecf_log_file(port));

		// start the server in the background
		std::string theServerInvokePath = ecf::File::find_ecf_server_path();
		BOOST_REQUIRE_MESSAGE(!theServerInvokePath.empty(),"InvokeServer::doStart: The server program could not be found");
      BOOST_REQUIRE_MESSAGE(boost::filesystem::exists(theServerInvokePath),"InvokeServer::doStart: server exe does not exist at:" << theServerInvokePath);

		// Make sure server starts in the background to avoid hanging test
		theServerInvokePath += " --port=" + port;
		if (disable_job_generation) {
		   theServerInvokePath += " --dis_job_gen";
		}
		theServerInvokePath += " &";

		//std::cout << "InvokeServer::doStart port = " << port << " server path = " <<  theServerInvokePath << "\n";
		(void)system( theServerInvokePath.c_str() );

		// Allow time for server process to kick in.
      ClientInvoker theClient(ecf::Str::LOCALHOST(),port);
      BOOST_REQUIRE_MESSAGE(theClient.wait_for_server_reply(),"InvokeServer::doStart: Server failed to start after 60 second on " << ecf::Str::LOCALHOST() << ":" << port);
	}

private:
	InvokeServer(const InvokeServer&);
	std::string port_;
	std::string host_;
	ecf::Host   host_name_;
	bool remove_checkpt_file_after_server_exit_;
};

#endif
