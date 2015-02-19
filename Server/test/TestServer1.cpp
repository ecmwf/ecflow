//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $
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
#include <iostream>
#include <fstream>
#include <stdlib.h>

#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include "boost/filesystem/operations.hpp"

#include "ServerEnvironment.hpp"
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
   TestServer(ServerEnvironment& s) : Server(s) {}
   virtual ~TestServer() {}

   // abort server if check pt files exist, but can't be loaded
   //   bool load_check_pt_file_on_startup();
   //   void loadCheckPtFile();
   //   bool restore_from_checkpt(const std::string& filename, bool& failed);
   //   void set_server_state(SState::State);

   /// AbstractServer functions
   virtual SState::State state() const { return Server::state(); }
   virtual std::pair<std::string,std::string> hostPort() const { return Server::hostPort(); }
   virtual defs_ptr defs() const { return Server::defs();}
   // virtual void updateDefs(defs_ptr,bool force);
   virtual void clear_defs() { Server::clear_defs();}
   //   virtual void checkPtDefs(ecf::CheckPt::Mode m = ecf::CheckPt::UNDEFINED,
   //                               int check_pt_interval = 0,
   //                               int check_pt_save_time_alarm = 0);
   virtual void restore_defs_from_checkpt() { Server::restore_defs_from_checkpt();}
   virtual void nodeTreeStateChanged()  { Server::nodeTreeStateChanged(); }
   virtual bool allowTaskCommunication() const { return Server::allowTaskCommunication();}
   virtual void shutdown() { Server::shutdown(); }
   virtual void halted() { Server::halted(); }
   virtual void restart() { Server::restart(); }
   virtual bool reloadWhiteListFile(std::string& errorMsg) { return Server::reloadWhiteListFile(errorMsg);}
   virtual bool authenticateUser(const std::string& user) { return Server::authenticateUser(user); }
   virtual bool authenticateWriteAccess(const std::string& user, bool client_request_can_change_server_state) { return Server::authenticateWriteAccess(user,client_request_can_change_server_state); }
   virtual bool lock(const std::string& user) { return Server::lock(user); }
   virtual void unlock() { Server::unlock();}
   virtual const std::string& lockedUser() const { return Server::lockedUser(); }
   //   virtual void traverse_node_tree_and_job_generate(const boost::posix_time::ptime& time_now, bool user_cmd_context) const;
   virtual int poll_interval() const { return Server::poll_interval();}
   //   virtual void debug_server_on();
   //   virtual void debug_server_off();
   //   virtual bool debug() const;
};


BOOST_AUTO_TEST_CASE( test_server )
{
   cout << "Server:: ...test_server\n";

   int argc = 3;
   char* argv[] = { const_cast<char*>("ServerEnvironment"),
                     const_cast<char*>("--port=3144"),
                     const_cast<char*>("--ecfinterval=12")
                   };

   ServerEnvironment server_environment(argc, argv);  // This can throw ServerEnvironmentException
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(server_environment.valid(errorMsg),errorMsg);

   {
      TestServer theServer( server_environment ); // This can throw exception, bind address in use.

      BOOST_REQUIRE_MESSAGE(theServer.defs(),"Expected defs to be created");

      const std::vector<Variable>& server_variables = theServer.defs()->server().server_variables();
      BOOST_REQUIRE_MESSAGE(!server_variables.empty(),"Expected defs to be updated with the server variables");

      //for(size_t i = 0; i < server_variables.size(); ++i)  cout << server_variables[i].dump() << "\n";
      const std::string& ecf_port = theServer.defs()->server().find_variable("ECF_PORT");
      BOOST_REQUIRE_MESSAGE(ecf_port == "3144","Expected port 3144 but found " << ecf_port << " defs server variables, should be in sync with server");


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

   Host h;
   fs::remove(h.ecf_log_file(Str::DEFAULT_PORT_NUMBER()));

   /// Destroy Log singleton to avoid valgrind from complaining
   Log::destroy();
}

BOOST_AUTO_TEST_SUITE_END()
