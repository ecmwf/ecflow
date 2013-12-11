//============================================================================
// Name        : ServerMain
// Author      : Avi
// Revision    : $Revision: #37 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Server
//============================================================================

#include <memory>
#include "Server.hpp"
#include "Log.hpp"
#include "ServerEnvironment.hpp"

using namespace ecf;
using namespace std;

int main( int argc, char* argv[] ) {

   try {
      // Get the environment settings, and parse argument line and init the log file
      ServerEnvironment server_environment(argc, argv);  // This can throw ServerEnvironmentException
      if ( server_environment.help_option()) return 0;
      if ( server_environment.version_option()) return 0;
      std::string errorMsg;
      if (!server_environment.valid(errorMsg))  {
         cerr << errorMsg;
         ecf::log(Log::ERR,errorMsg);
         return 1;
      }

      if (server_environment.debug()) cout << "Server started: ------------------------------------------------>port:" << server_environment.port() <<  endl;
      Server theServer( server_environment ); // This can throw exception, bind address in use.
      for(;;) {
         try {
            theServer.run();
            if (server_environment.debug()) cout << "Normal exit from server\n";
            break;
         }
         catch ( std::exception& e ) {
            // deal with errors from the handlers
            std::string msg = "ServerMain:: "; msg += e.what();
            std::cerr << msg  << endl;
            ecf::log(Log::ERR,msg);
         }
      }

      if (server_environment.debug()) cout << "Server EXITING: <------------------------------------------------ port:" << server_environment.port() <<  endl;
      return 0;
   }
   catch ( ServerEnvironmentException& e ) {
      // *** deal with server options and environment exceptions
      std::cerr << "Could not load the server environment or options :\n" << e.what() << "\n";
      std::cerr << "Invalid locale? Check locale using 'locale -a', then export/set LANG environment\n";
   }
   catch ( std::exception& e ) {
      // *** deal with errors from the server constructor ****
      std::string msg = "Exception in ServerMain:: "; msg += e.what();
      std::cerr << msg  << endl;
      ecf::log(Log::ERR,msg);

      // dump server environment
      cerr << "\nServer environment:\n";
      ServerEnvironment server_env(argc, argv);  // This can throw ServerEnvironmentException
      std::cerr << server_env.dump();
   }
   catch ( ... ) {
       std::string msg = "ServerMain:: unknown exception";
       std::cerr << msg  << endl;
       ecf::log(Log::ERR,msg);
   }
   return 1;
}
