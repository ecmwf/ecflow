//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $
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
#include <iostream>
#include <fstream>
#include <cstdlib>

#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include "boost/filesystem/operations.hpp"

#include "ServerEnvironment.hpp"
#include "EcfPortLock.hpp"
#include "Log.hpp"
#include "Host.hpp"
#include "Str.hpp"
#include "Ecf.hpp"
#include "File.hpp"
#include "Server.hpp"
#include "Defs.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( TestServer )

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// make public the function we wish to test:
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class TestServer : public Server {
public:
   explicit TestServer(boost::asio::io_service& io_service, ServerEnvironment& s) : Server(io_service,s) {}
   ~TestServer() override = default;

   // abort server if check pt files exist, but can't be loaded
   //   bool load_check_pt_file_on_startup();
   //   void loadCheckPtFile();
   //   bool restore_from_checkpt(const std::string& filename, bool& failed);
   //   void set_server_state(SState::State);

   /// AbstractServer functions
   SState::State state() const override { return Server::state(); }
   std::pair<std::string,std::string> hostPort() const override { return Server::hostPort(); }
   defs_ptr defs() const override { return Server::defs();}
   // virtual void updateDefs(defs_ptr,bool force);
   void clear_defs() override { Server::clear_defs();}
   //   virtual void checkPtDefs(ecf::CheckPt::Mode m = ecf::CheckPt::UNDEFINED,
   //                               int check_pt_interval = 0,
   //                               int check_pt_save_time_alarm = 0);
   void restore_defs_from_checkpt() override { Server::restore_defs_from_checkpt();}
   void nodeTreeStateChanged() override  { Server::nodeTreeStateChanged(); }
   bool allowTaskCommunication() const override { return Server::allowTaskCommunication();}
   void shutdown() override { Server::shutdown(); }
   void halted() override { Server::halted(); }
   void restart() override { Server::restart(); }

   bool reloadWhiteListFile(std::string& errorMsg) override { return Server::reloadWhiteListFile(errorMsg);}
   bool reloadPasswdFile(std::string& errorMsg) override { return Server::reloadPasswdFile(errorMsg);}

   bool authenticateReadAccess(const std::string& user,bool custom_user,const std::string& passwd) override { return Server::authenticateReadAccess(user,custom_user,passwd); }
   bool authenticateReadAccess(const std::string& user,bool custom_user,const std::string& passwd, const std::string& path) override { return Server::authenticateReadAccess(user,custom_user,passwd,path); }
   bool authenticateReadAccess(const std::string& user,bool custom_user,const std::string& passwd, const std::vector<std::string>& paths) override{ return Server::authenticateReadAccess(user,custom_user,passwd,paths); }

   bool authenticateWriteAccess(const std::string& user) override { return Server::authenticateWriteAccess(user); }
   bool authenticateWriteAccess(const std::string& user, const std::string& path) override { return Server::authenticateWriteAccess(user,path); }
   bool authenticateWriteAccess(const std::string& user, const std::vector<std::string>& paths) override{ return Server::authenticateWriteAccess(user,paths); }

   bool lock(const std::string& user) override { return Server::lock(user); }
   void unlock() override { Server::unlock();}
   const std::string& lockedUser() const override { return Server::lockedUser(); }
   //   virtual void traverse_node_tree_and_job_generate(const boost::posix_time::ptime& time_now, bool user_cmd_context) const;
   int poll_interval() const override { return Server::poll_interval();}
   //   virtual void debug_server_on();
   //   virtual void debug_server_off();
   //   virtual bool debug() const;
};


BOOST_AUTO_TEST_CASE( test_server )
{
   cout << "Server:: ...test_server\n";

   // Create a unique port number, allowing debug and release,gnu,clang,intel to run at the same time
   // Hence the lock file is not always sufficient.
   // TEST_ECF_PORT should be unique among  gnu,clang,intel, etc
   std::string the_port1 = "3144" ;
   char* test_ecf_port = getenv("TEST_ECF_PORT");  // from metabuilder, allow parallel tests
   if ( test_ecf_port ) the_port1 = test_ecf_port;
   cout << "Find free port to start server, starting with port " << the_port1 << "\n";

   auto the_port = boost::lexical_cast<int>(the_port1);
   while (!EcfPortLock::is_free(the_port)) the_port++;
   std::string port = boost::lexical_cast<std::string>(the_port);
   EcfPortLock::create(port);
   cout << "Found free port: " << port << "\n";

   std::string server_port = "--port=" + port;
   int argc = 3;
   char* argv[] = { const_cast<char*>("ServerEnvironment"),
                    const_cast<char*>(server_port.c_str()),
                    const_cast<char*>("--ecfinterval=12")
                  };


   ServerEnvironment server_environment(argc, argv);  // This can throw ServerEnvironmentException
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(server_environment.valid(errorMsg),errorMsg);

   {
      boost::asio::io_service io_service;
      TestServer theServer(io_service, server_environment ); // This can throw exception, bind address in use.

      BOOST_REQUIRE_MESSAGE(theServer.defs(),"Expected defs to be created");

      const std::vector<Variable>& server_variables = theServer.defs()->server().server_variables();
      BOOST_REQUIRE_MESSAGE(!server_variables.empty(),"Expected defs to be updated with the server variables");

      //for(size_t i = 0; i < server_variables.size(); ++i)  cout << server_variables[i].dump() << "\n";
      const std::string& ecf_port = theServer.defs()->server().find_variable("ECF_PORT");
      BOOST_REQUIRE_MESSAGE(ecf_port == port,"Expected port " << port << " but found " << ecf_port << " defs server variables, should be in sync with server");


      const std::string& interval = theServer.defs()->server().find_variable("ECF_INTERVAL");
      BOOST_REQUIRE_MESSAGE(interval == "12","Expected interval 12 but found " << interval << " defs server variables, should be in sync with server");
      BOOST_REQUIRE_MESSAGE(theServer.poll_interval() == 12,"Expected poll interval 12 but found " << theServer.poll_interval());


      BOOST_REQUIRE_MESSAGE(theServer.state() == SState::HALTED,"Expected halted at server start but found " << SState::to_string(theServer.state()));
      theServer.halted(); BOOST_REQUIRE_MESSAGE(theServer.state() == SState::HALTED,"Expected halted ");
      theServer.shutdown(); BOOST_REQUIRE_MESSAGE(theServer.state() == SState::SHUTDOWN,"Expected shutdown ");
      theServer.restart(); BOOST_REQUIRE_MESSAGE(theServer.state() == SState::RUNNING,"Expected shutdown ");


      BOOST_REQUIRE_MESSAGE(theServer.lock("fred"),"Expected to lock user fred");
      BOOST_REQUIRE_MESSAGE(theServer.state() == SState::SHUTDOWN,"Locking should shutdown server");
      BOOST_REQUIRE_MESSAGE(theServer.lockedUser() == "fred","Expected locked user 'fred' but found " << theServer.lockedUser());
      theServer.unlock();
      BOOST_REQUIRE_MESSAGE(theServer.lockedUser().empty(),"Expected no locked user but found " << theServer.lockedUser());
      BOOST_REQUIRE_MESSAGE(theServer.state() == SState::RUNNING,"Expected unlock to restart server ");
   }

   // cleanup
   Host h;
   fs::remove(h.ecf_log_file(port));
   EcfPortLock::remove(port);

   /// Destroy Log singleton to avoid valgrind from complaining
   Log::destroy();
}

BOOST_AUTO_TEST_SUITE_END()
