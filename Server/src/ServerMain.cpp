//============================================================================
// Name        : ServerMain
// Author      : Avi
// Revision    : $Revision: #37 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Server
//============================================================================

#include "Log.hpp"
#include "ServerEnvironment.hpp"

#include "Server.hpp"
#ifdef ECF_OPENSSL
#include "SslServer.hpp"
#endif

using namespace ecf;
using namespace std;

int run_server(boost::asio::io_service& io_service, ServerEnvironment& server_environment)
{
    for(;;) {
       try {
          /// Start the server
          /// The io_service::run() call will block until all asynchronous operations
          /// have finished. While the server is running, there is always at least one
          /// asynchronous operation outstanding: the asynchronous accept call waiting
          /// for new incoming connections.
          io_service.run();
          if (server_environment.debug()) cout << "Normal exit from server\n";
          break;
       }
       catch ( std::exception& e ) {
          // deal with errors from the handlers
          std::string msg = "run_server:: "; msg += e.what();
          std::cerr << msg  << endl;
          ecf::log(Log::ERR,msg);
       }
       if (server_environment.debug()) cout << "Server EXITING: <------------------------------------------------ port:" << server_environment.port() <<  endl;
    }
    return 0;
}

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

      boost::asio::io_service io_service;
#ifdef ECF_OPENSSL
      if (server_environment.ssl() ) {
         SslServer theServer(io_service, server_environment ); // This can throw exception, bind address in use.
         return run_server(io_service, server_environment );
      }
      else                           {
         Server theServer(io_service, server_environment ); // This can throw exception, bind address in use.
         return run_server(io_service, server_environment );
      }
#else
      Server theServer(io_service, server_environment ); // This can throw exception, bind address in use.
      return run_server(io_service, server_environment );
#endif
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
