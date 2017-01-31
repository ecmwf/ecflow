#ifndef INVOKESERVER_HPP_
#define INVOKESERVER_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $ 
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
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include <boost/noncopyable.hpp>

#include "TestHelper.hpp"
#include "ClientInvoker.hpp"
#include "Str.hpp"
#include "File.hpp"
#include "EcfPortLock.hpp"
#include "Host.hpp"

class InvokeServer : private boost::noncopyable {
public:
	InvokeServer(const std::string& msg,
	               const std::string& port = ecf::Str::DEFAULT_PORT_NUMBER(),
	               bool disable_job_generation = false,
                  bool remove_checkpt_file_before_server_start = true,
                  bool remove_checkpt_file_after_server_exit = true
	             ) : port_(port),
	                 host_(ClientEnvironment::hostSpecified()),
	                 remove_checkpt_file_after_server_exit_(remove_checkpt_file_after_server_exit)
   {
		if (host_.empty()) {
			if(!msg.empty()) std::cout << msg << "   port(" << port_ << ")" << std::endl;

			doStart(port_,disable_job_generation,remove_checkpt_file_before_server_start);
		}
		else {
			// Start of test, clear any existing defs on remote/localhost server
			// Assuming this has been started on input port
			std::string test_name = msg;
         test_name += " on ";
         test_name += host_;
         test_name += ecf::Str::COLON();
         test_name += port_;

			std::cout << test_name << std::endl;

			ClientInvoker theClient(host_,port_);
		   theClient.logMsg( test_name );
		   BOOST_REQUIRE_MESSAGE( theClient.delete_all() == 0,CtsApi::to_string(CtsApi::delete_node()) << " failed should return 0. Should Delete ALL existing defs in the server\n" << theClient.errorMsg());
		}
	}

   InvokeServer(const std::string& port,
                  bool stop_ambiguaty,
                  bool disable_job_generation = false,
                  bool remove_checkpt_file_before_server_start = true,
                  bool remove_checkpt_file_after_server_exit = true
                ) : port_(port),
                    remove_checkpt_file_after_server_exit_(remove_checkpt_file_after_server_exit)
   {
      // host_ is empty.
      doStart(port_,disable_job_generation,remove_checkpt_file_before_server_start);
   }

	~InvokeServer() {
	   // This will also remove the generated files.
	   // Will only terminate local server, host_ is *EMPTY* for local server, using two constructors above
	   if (host_.empty()) {
	      doEnd(ecf::Str::LOCALHOST(), port_, remove_checkpt_file_after_server_exit_);
	   }
	}

	const std::string& port() const { return port_; }
	const std::string& host() const { if (host_.empty()) return ecf::Str::LOCALHOST(); return host_; }

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

		// Create a port file. To avoid creating multiple servers on the same port number
		ecf::EcfPortLock::create( port );

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


   static void doEnd( const std::string& host, const std::string& port, bool remove_checkpt_file_after_server_exit )
   {
      //    std::cout << "*****InvokeServer::doEnd    Closing server on  " << host << ecf::Str::COLON() << port << "\n";
      {
         ClientInvoker theClient(host,port);
         BOOST_REQUIRE_NO_THROW( theClient.terminateServer() );
         BOOST_REQUIRE_MESSAGE( theClient.wait_for_server_death(),"Failed to terminate server after 60 seconds\n");
      }

      // remove port file. This prevented multiple different process from opening servers with same port number
      ecf::EcfPortLock::remove( port );

      // Remove generated file comment for debug
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
	InvokeServer(const InvokeServer&);
	std::string port_;
	std::string host_;
	ecf::Host   host_name_;
	bool remove_checkpt_file_after_server_exit_;
};

#endif
