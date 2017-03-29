/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision$ 
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
#include <iterator>
#include <boost/date_time/posix_time/time_formatters.hpp>  // requires boost date and time lib

#include "ClientInvoker.hpp"
#include "Client.hpp"
#include "ClientEnvironment.hpp"
#include "ClientOptions.hpp"
#include "Defs.hpp"
#include "ArgvCreator.hpp"
#include "Str.hpp"
#include "ClientToServerCmd.hpp"
#include "Rtt.hpp"
#include "Ecf.hpp"
#include "DurationTimer.hpp"
#include "TimeStamp.hpp"
#include "Log.hpp"
#ifdef ECF_OPENSSL
#include "Openssl.hpp"
#endif
#include "PasswdFile.hpp"

#ifdef DEBUG

#if defined(HPUX) || defined(_AIX)
#define RETRY_CONNECTION_PERIOD 2
#define NEXT_HOST_POLL_PERIOD 2
#else
#define RETRY_CONNECTION_PERIOD 1
#define NEXT_HOST_POLL_PERIOD 1
#endif

#else
#define RETRY_CONNECTION_PERIOD 10
#define NEXT_HOST_POLL_PERIOD 30
#endif

using namespace std;
using namespace ecf;
using namespace boost::posix_time;

// ==================================================================================
// class ClientInvoker
ClientInvoker::ClientInvoker()
: on_error_throw_exception_(true), test_(false),testInterface_(false),
  connection_attempts_(2),retry_connection_period_(RETRY_CONNECTION_PERIOD),child_task_try_no_(0)
{
	if (clientEnv_.debug()) cout << TimeStamp::now() << "ClientInvoker::ClientInvoker(): 1=================start=================\n";
}

ClientInvoker::ClientInvoker(const std::string& host_port)
: on_error_throw_exception_(true), test_(false),testInterface_(false),
  connection_attempts_(2),retry_connection_period_(RETRY_CONNECTION_PERIOD),child_task_try_no_(0)
{
   if (clientEnv_.debug()) cout << TimeStamp::now() << "ClientInvoker::ClientInvoker(): 2=================start=================\n";
   // assume format <host>:<port>
   size_t colonPos = host_port.find_first_of(':');
   if (colonPos == string::npos)  throw std::runtime_error("ClientInvoker::ClientInvoker: expected <host>:<port> : no ':' found in " + host_port);
   std::string host = host_port.substr(0,colonPos);
   std::string port = host_port.substr(colonPos+1);
   set_host_port(host,port);
}

ClientInvoker::ClientInvoker(const std::string& host, const std::string& port)
: on_error_throw_exception_(true), test_(false),testInterface_(false),
  connection_attempts_(2),retry_connection_period_(RETRY_CONNECTION_PERIOD),child_task_try_no_(0)
{
   if (clientEnv_.debug()) cout << TimeStamp::now() << "ClientInvoker::ClientInvoker(): 3=================start=================\n";
   set_host_port(host,port);
}

ClientInvoker::ClientInvoker(const std::string& host, int port)
: on_error_throw_exception_(true), test_(false),testInterface_(false),
  connection_attempts_(2),retry_connection_period_(RETRY_CONNECTION_PERIOD),child_task_try_no_(0)
{
   if (clientEnv_.debug()) cout << TimeStamp::now() << "ClientInvoker::ClientInvoker(): 4=================start=================\n";
   set_host_port(host, boost::lexical_cast<std::string>(port));
}

void ClientInvoker::set_host_port(const std::string& host, const std::string& port)
{
	// Allow host and port to be overridden.
   // o Override environment setting
   // o For child commands will override opening of ecf_hosts file
	clientEnv_.set_host_port(host,port);
}

const std::string& ClientInvoker::host() const
{
   return clientEnv_.host();
}
const std::string& ClientInvoker::port() const
{
   return clientEnv_.port();
}

void ClientInvoker::allow_new_client_old_server(int archive_version_of_old_server)
{
   clientEnv_.allow_new_client_old_server(archive_version_of_old_server);
}

int ClientInvoker::allow_new_client_old_server() const
{
   return clientEnv_.allow_new_client_old_server();
}

void ClientInvoker::taskPath(const std::string& s) {
	assert(!s.empty());
	test_ = true;
	clientEnv_.taskPath(s);
}
void ClientInvoker::set_jobs_password(const std::string& djp)
{
    test_ = true;
    clientEnv_.set_jobs_password(djp);
}

void ClientInvoker::setEnv( const std::vector<std::pair<std::string,std::string> >& e) {
	assert(!e.empty());
	test_ = true;
	clientEnv_.setEnv(e); // For test allow env variable to be set on defs
}

void ClientInvoker::testInterface() {
   testInterface_ = true;
   clientEnv_.set_test();
}

const std::string& ClientInvoker::process_or_remote_id() const
{
   return clientEnv_.process_or_remote_id();
}

void ClientInvoker::enable_logging(const std::string& log_file_name)
{
   Rtt::create(log_file_name);
}

void ClientInvoker::disable_logging()
{
   Rtt::destroy();
}

void ClientInvoker::set_connect_timeout(int t)
{
   clientEnv_.set_connect_timeout(t);
}

void ClientInvoker::set_connection_attempts( unsigned int attempts)
{
   connection_attempts_ = attempts;
   if ( connection_attempts_ < 1 ) connection_attempts_ = 1;
}

int ClientInvoker::invoke(int argc, char* argv[]) const
{
   // Allow request to logged & allow logging of round trip time, Hence must be placed *before* RoundTripRecorder
   RequestLogger request_logger(this);

   // initialise start_time_ and rtt_,
   RoundTripRecorder round_trip_recorder(this);

	/// If NO_ECF set then abort immediately. returning success. Useful in testing  jobs stand-alone.
	if (clientEnv_.no_ecf()) { cout << "NO_ECF\n"; return 0; } // success

	// Clear error message. For test. Don't keep previous error.
	// i.e If next test passes when it shouldn't the wrong message is output
	server_reply_.get_error_msg().clear();

	Cmd_ptr cts_cmd;
 	try {
		// read in program option, and construct the client to server commands from them.
		// This will extract host/port from the environment/ args
		// This will throw std::runtime_error for invalid arguments or options
		cts_cmd = args_.parse(argc,argv,&clientEnv_);

		// For --help and --debug, --load defs check_only no command is created
		// When testInterface avoid writing to standard out.
		if (!cts_cmd.get()) {
			if (!testInterface_ && clientEnv_.debug()) {
				cout << "args: "; for ( int x=0; x< argc; x++) cout << argv[x] << " "; cout << "\n";
			}
			return 0;
		}
	}
	catch ( std::exception& e ) {
		stringstream ss;
		if (argc == 1) {
		   ss << Ecf::CLIENT_NAME() << ": No options specified\n";
		   ss << "Usage: " << Ecf::CLIENT_NAME() << " [OPTION]...\n";
		   ss << "Try '" << Ecf::CLIENT_NAME() << " --help' for list of options\n";
		}
		else {
		   ss << Ecf::CLIENT_NAME() << ": Caught exception whilst parsing arguments:\n" << e.what() << "\n";
		   ss << "args: "; for ( int x=0; x< argc; x++) ss << argv[x] << " "; ss << "\n";
		}
		server_reply_.set_error_msg(ss.str());
 		if (on_error_throw_exception_) throw std::runtime_error(server_reply_.error_msg());
		return 1;
	}
	catch ( ... ) {
		server_reply_.set_error_msg("ecflow:ClientInvoker: caught exception: Parsing arguments: unknown type!\n");
 		if (on_error_throw_exception_) throw std::runtime_error(server_reply_.error_msg());
		return 1;
	}

   // Under debug we display round trip time for each request
	request_logger.set_cts_cmd(cts_cmd);

	int res = do_invoke_cmd( cts_cmd );
	if (res == 1 && on_error_throw_exception_) throw std::runtime_error(server_reply_.error_msg());
	return res;
}

int ClientInvoker::invoke( const std::string& arg ) const
{
	int argc = 2;
	char* argv[] = { const_cast<char*>("ClientInvoker"),  const_cast<char*>(arg.c_str()) };
 	return invoke(argc,argv);
}

int ClientInvoker::invoke( const std::vector<std::string>& args ) const
{
	std::vector<std::string> theArgs;
	theArgs.push_back("ClientInvoker");
	std::copy( args.begin(), args.end(), std::back_inserter(theArgs) );
	ArgvCreator argvCreator( theArgs );
	return invoke(argvCreator.argc(),argvCreator.argv());
}

int ClientInvoker::invoke(Cmd_ptr cts_cmd) const
{
   // assumes clients of Cmd_ptr constructor has caught exceptions

   // Allow request to be logged & allow logging of round trip time, Hence must be placed *before* RoundTripRecorder
   RequestLogger request_logger(this);

   // initialise start_time_ and rtt_,
   RoundTripRecorder round_trip_recorder(this);

   // allow display of round trip time for each request
   request_logger.set_cts_cmd(cts_cmd);

   int res = do_invoke_cmd( cts_cmd );
   if (res == 1 && on_error_throw_exception_) throw std::runtime_error(server_reply_.error_msg());
   return res;
}

int ClientInvoker::do_invoke_cmd(Cmd_ptr cts_cmd) const
{
	if (clientEnv_.debug()) cout << "\n" << TimeStamp::now() << "ClientInvoker::do_invoke_cmd : on_error_throw_exception_(" << on_error_throw_exception_ << ")" << std::endl;
	if (clientEnv_.no_ecf()) { cout << "NO_ECF\n"; return 0;} // success If NO_ECF set then abort immediately. returning success. Useful in testing  jobs stand-alone.
	if (testInterface_) return 0;       // The testInterface_ flag allows testing of client interface, parsing of args, without needing to contact server
	assert(!clientEnv_.host().empty()); // make sure host is NOT empty.

	/// retry_connection_period_ specifies the time to wait, before retrying to connect to server.
	/// Added to get round glitches in the network.
	/// However for ping() always default to 1 second. This avoids 10 second wait in release mode.
	/// We do this both for the CLI(command level interface) and python api
	unsigned int retry_connection_period = retry_connection_period_;
	if (cts_cmd->ping_cmd()) retry_connection_period = 1;

	try {
	   /// report this message at least once. So client has a clue what's going on
	   bool report_block_client_on_home_server = false;
	   bool report_block_client_server_halted = false;
	   bool report_block_client_zombie_detected = false;
		// We do not want to loop over the sms host list indefinitely hence we use a timer.
		// The time out period is supplied via ClientEnvironment
		bool never_polled = true; // don't wait for the first host only subsequent ones

		while ( true ) {

			// for each host try connecting several times. To compensate for network glitches.
 			int no_of_tries = connection_attempts_;
			while ( no_of_tries > 0 ) {
				try {
					if (clientEnv_.debug()) { cout << TimeStamp::now() << "ClientInvoker: >>> About to invoke "; cts_cmd->print(cout); cout << " on " << client_env_host_port() << " : retry_connection_period(" << retry_connection_period << ") no_of_tries(" << no_of_tries << ") cmd_connect_timeout(" << cts_cmd->timeout() << ") ECF_CONNECT_TIMEOUT(" << clientEnv_.connect_timeout() << ")<<<" << endl;}

					/// *** Each call to io_service.run(); is a *REQUEST* to the server ***
					/// *** Hence we *MUST* clear the server_reply before each call *******
					/// *** Found during zombie test. i.e when blocking, we were responding to previous, reply, since server_reply was not being reset
					/// *Note* server_reply_.client_handle_ is kept until the next call to register a new client_handle
					/// The client invoker can be used multiple times, hence keep value of defs, and client handle in server reply
					/// However this is only done, if we are not using the Command Level Interface(cli)
					server_reply_.clear_for_invoke(cli());

					cts_cmd->setup_user_authentification( clientEnv_ );

					boost::asio::io_service io_service;
#ifdef ECF_OPENSSL
				   boost::asio::ssl::context ctx(ecf::Openssl::method());
				   ctx.load_verify_file(ecf::Openssl::certificates_dir() + "server.crt");
	            Client theClient(io_service,ctx,cts_cmd,clientEnv_.host(),clientEnv_.port(),clientEnv_.connect_timeout() );
#else
	            Client theClient(io_service,cts_cmd,clientEnv_.host(),clientEnv_.port(),clientEnv_.connect_timeout());
#endif
					if (clientEnv_.allow_new_client_old_server() != 0) theClient.allow_new_client_old_server(clientEnv_.allow_new_client_old_server());
					io_service.run();
					if (clientEnv_.debug()) cout << TimeStamp::now() << "ClientInvoker: >>> After: io_service.run() <<<" << endl;;

					/// Let see how the server responded if at all.
					try {
						/// will return false if further action required
						if (theClient.handle_server_response( server_reply_, clientEnv_.debug() )) {
							// The normal response.  RoundTriprecorder will record in rtt_

						   // If the command was a delete_all command, reset client_handle
						   if (cts_cmd->delete_all_cmd()) {
						      ClientInvoker* non_const_this = const_cast<ClientInvoker*>(this);
						      non_const_this->reset();
						   }
							return 0; // the normal exit path
						}
					}
					catch (std::exception& e) {
 						server_reply_.set_error_msg( e.what() );
						return 1;
					}

					if ( server_reply_.block_client_on_home_server()) {
						// Valid reply from server. Typically waiting on a expression
						// Ok _Block_ on _current_ server, and continue waiting, until server reply is ok
						if (!report_block_client_on_home_server || clientEnv_.debug()) { cout << TimeStamp::now() << "ecflow:ClientInvoker: "; cts_cmd->print(cout); cout << " : " << client_env_host_port() << " : WAITING on home server, continue waiting\n";report_block_client_on_home_server = true;}
						no_of_tries++;
 					}
					else if (server_reply_.block_client_server_halted()) {
						// Valid reply from server.
					   // fall through try again, then try other hosts
						if (!report_block_client_server_halted || clientEnv_.debug()){ cout << TimeStamp::now() << "ecflow:ClientInvoker: "; cts_cmd->print(cout); cout << " : " << client_env_host_port() << " : blocking : server is HALTED, continue waiting\n";report_block_client_server_halted = true;}
  					}
					else if (server_reply_.block_client_zombie_detected()) {
						// Valid reply from server.
					   // fall through try again, then try other hosts
						if (!report_block_client_zombie_detected || clientEnv_.debug()){ cout << TimeStamp::now() << "ecflow:ClientInvoker: "; cts_cmd->print(cout); cout << " : " << client_env_host_port() << " : blocking : zombie detected, continue waiting\n";report_block_client_zombie_detected = true;}
  					}
					else  if (server_reply_.client_request_failed()) {
						// Valid reply from server
						// This error is ONLY valid if we got a real reply from the server
						// as opposed to some kind of connection errors. For connections errors
						// we fall through and try again.
						if (clientEnv_.debug()) {cout << TimeStamp::now() << "ecflow:ClientInvoker:"; cts_cmd->print(cout); cout << " failed : " << client_env_host_port() << " : " << server_reply_.error_msg() << "\n";}
						return 1;
					}
					else {
						std::cout << TimeStamp::now() << "ecflow:ClientInvoker: missed response? for request "; cts_cmd->print(cout); std::cout << " oops" << endl;
					}
 				}
				catch (std::exception& e) {
					// *Some kind of connection error*: fall through and try again. Avoid this message when pinging, i.e to see if server is alive.
				   if (clientEnv_.debug()) { cerr << TimeStamp::now() << "ecflow:ClientInvoker: Connection error: (" << e.what() <<  ")" << endl; }
				   if (!cts_cmd->ping_cmd()) {
				      cerr << TimeStamp::now() << "ecflow:ClientInvoker: Connection error: (" << e.what() <<  ")" << endl;
				   }
 				}

				// Wait a bit before trying to connect again, but only if no_of_tries > 0
				no_of_tries--;
 				if (no_of_tries > 0) sleep( retry_connection_period );
			}

			// Don't bother with other hosts when:
			//  1/ Testing
 			//  2/ ping-ing
			//  3/ ECF_DENIED has been set
			//  4/ Dealing with non tasks based request
			if (!cts_cmd->connect_to_different_servers() || test_ || cts_cmd->ping_cmd() || clientEnv_.denied() ) {
				std::stringstream ss;
 				ss << TimeStamp::now() << "Request( "; cts_cmd->print(ss) << " )";
 				if (clientEnv_.denied()) ss << " ECF_DENIED ";
 				ss << ", Failed to connect to "  << client_env_host_port()
 				   << ". After " << connection_attempts_ << " attempts. Is the server running ?\n";
 				// Only print client environment if not pinging
            if (!cts_cmd->ping_cmd())  ss << "Client environment:\n" << clientEnv_.toString() << endl;
 				server_reply_.set_error_msg(ss.str());
 				return 1;
			}

			boost::posix_time::time_duration duration = microsec_clock::universal_time() - start_time_;
			if (clientEnv_.debug()) { cout << "ClientInvoker: Time duration = " << duration.total_seconds() << " clientEnv_.max_child_cmd_timeout() = " << clientEnv_.max_child_cmd_timeout() << endl;}

			if ( duration.total_seconds() >= clientEnv_.max_child_cmd_timeout() ) {
				std::stringstream ss; ss << TimeStamp::now() << "ecflow:ClientInvoker: Timed out after ECF_TIMOUT(" << clientEnv_.max_child_cmd_timeout() << ") seconds : for " << client_env_host_port() << "\n";
 				std::string msg = ss.str();
 				cout << msg;
				server_reply_.set_error_msg(msg);
				return 1;
			}
         if (server_reply_.block_client_zombie_detected() && duration.total_seconds() >= clientEnv_.max_zombie_child_cmd_timeout() ) {
            std::stringstream ss; ss << TimeStamp::now() << "ecflow:ClientInvoker: *ZOMBIE* Timed out after ECF_ZOMBIE_TIMEOUT(" << clientEnv_.max_zombie_child_cmd_timeout() << ") seconds : for " << client_env_host_port() << "\n";
            std::string msg = ss.str();
            cout << msg;
            server_reply_.set_error_msg(msg);
            return 1;
         }

			// The host is not playing ball, try the next host, will *restart* with home server, if end reached
			// *get_next_host* *only* returns false if host exists, and parsing it fails
			std::string current_host_port = client_env_host_port();

			std::string local_error_msg;
			if (!clientEnv_.get_next_host(local_error_msg)) {
			   /// Instead of exiting, Just spit out a warning
			   cout << TimeStamp::now() << "ecflow:ClientInvoker: "; cts_cmd->print(cout); cout << " get next host failed because: " << local_error_msg  << endl;
			}

			cout << TimeStamp::now() << "ecflow:ClientInvoker: "; cts_cmd->print(cout); cout << " current host(" << current_host_port << ") trying next host(" << client_env_host_port() << ")" << endl;

			if( never_polled ) never_polled = false; // To avoid the first wait
			else               sleep(NEXT_HOST_POLL_PERIOD);
		}
	}
	catch ( std::exception& e ) {
		stringstream ss; ss << TimeStamp::now() << "ecflow:ClientInvoker: caught exception: " << e.what() << "\n";
 		server_reply_.set_error_msg(ss.str());
	}
	catch ( ... ) {
      stringstream ss; ss << TimeStamp::now() << "ecflow:ClientInvoker: Caught Exception of unknown type!\n";
		server_reply_.set_error_msg(ss.str());
	}
	return 1;
}


void ClientInvoker::reset()
{
   server_reply_.set_client_defs( defs_ptr() );
   server_reply_.set_client_node( node_ptr() );
   server_reply_.set_client_handle( 0 );
}

//=====================================================================================
// By using the command directly, it is a lot faster than using argc/argv
// preserve old method to test api/command level interface.

int ClientInvoker::getDefs() const
{
   if (testInterface_) return invoke(CtsApi::get());
   return invoke( Cmd_ptr( new CtsNodeCmd( CtsNodeCmd::GET) ) );
}

int ClientInvoker::loadDefs(
         const std::string& filePath,
         bool force,     /* true means overwrite suite of same name */
         bool check_only /* true means don't send to server, just check only */
) const
{
   if (testInterface_) return invoke(CtsApi::loadDefs(filePath,force,check_only));
   Cmd_ptr cmd = LoadDefsCmd::create(filePath,force,check_only,&clientEnv_);
   if (cmd) return invoke(cmd); // If check_only cmd will be empty
   return 0;
}

int ClientInvoker::sync(defs_ptr& client_defs) const
{
   if (client_defs.get()) {
      server_reply_.set_client_defs( client_defs );
      if (testInterface_) return invoke(CtsApi::sync(server_reply_.client_handle(), client_defs->state_change_no(), client_defs->modify_change_no()));
      return invoke( Cmd_ptr( new CSyncCmd(CSyncCmd::SYNC,server_reply_.client_handle(), client_defs->state_change_no(), client_defs->modify_change_no() ) ) );
   }

   if (testInterface_) return invoke(CtsApi::get());
   int res =  invoke( Cmd_ptr( new CtsNodeCmd(CtsNodeCmd::GET)));
   if (res == 0) {
      client_defs = server_reply_.client_defs(); // update change number
   }
   return res;
}

int ClientInvoker::sync_local(bool sync_suite_clock) const
{
   defs_ptr defs = server_reply_.client_defs();
   if (defs.get()) {

      // Prevent infinite loops in change observers.
      // This can be removed  when we do the new ecflowview. TODO
      if ( defs->in_notification()) {
         std::cout << "ecflow:ClientInvoker::sync_local() called in the middle of notification. Ignoring..... \n";
         return 0;
      }

      if (testInterface_) {
         if (sync_suite_clock) return invoke(CtsApi::sync_clock(server_reply_.client_handle(),defs->state_change_no(),defs->modify_change_no()));
         return invoke(CtsApi::sync(server_reply_.client_handle(),defs->state_change_no(), defs->modify_change_no()));
      }
      if (sync_suite_clock) return invoke( Cmd_ptr( new CSyncCmd(CSyncCmd::SYNC_CLOCK,server_reply_.client_handle(),defs->state_change_no(),defs->modify_change_no())));
      return invoke( Cmd_ptr( new CSyncCmd(CSyncCmd::SYNC,server_reply_.client_handle(),defs->state_change_no(),defs->modify_change_no())));
   }
   // If we have a handle return the defs, with the registered suites, else returns the full defs
   if (testInterface_) return invoke(CtsApi::sync_full(server_reply_.client_handle()));
   return invoke( Cmd_ptr( new CSyncCmd(server_reply_.client_handle()) ) );
}

int ClientInvoker::news(defs_ptr& client_defs) const
{
   if (client_defs.get()) {
      if (testInterface_) return invoke(CtsApi::news(server_reply_.client_handle(),client_defs->state_change_no(), client_defs->modify_change_no()));
      return invoke( Cmd_ptr( new CSyncCmd(CSyncCmd::NEWS,server_reply_.client_handle(), client_defs->state_change_no(), client_defs->modify_change_no() ) ) );
   }
   server_reply_.set_error_msg("The client definition is empty.");
   if (on_error_throw_exception_) throw std::runtime_error(server_reply_.error_msg());
   return 1;
}

int ClientInvoker::news_local() const
{
   defs_ptr defs = server_reply_.client_defs();
   if (defs.get()) {
      if (testInterface_) return invoke(CtsApi::news(server_reply_.client_handle(), defs->state_change_no(), defs->modify_change_no()));
      return invoke( Cmd_ptr( new CSyncCmd(CSyncCmd::NEWS,server_reply_.client_handle(), defs->state_change_no(), defs->modify_change_no() ) ) );
   }

   // There is no local defs, i.e first time call, The default client handle should be 0.
   // go with defaults for state and modify change numbers
   // User is expected to call sync_local(), which will update local defs.
   if (testInterface_) return invoke(CtsApi::news(server_reply_.client_handle(), 0, 0));
   return invoke( Cmd_ptr( new CSyncCmd(CSyncCmd::NEWS,server_reply_.client_handle(), 0, 0 ) ) );
}
//=====================================================================================
int ClientInvoker::restartServer() const
{
   if (testInterface_) return invoke(CtsApi::restartServer());
   return invoke(Cmd_ptr(new CtsCmd(CtsCmd::RESTART_SERVER)));
}
int ClientInvoker::haltServer() const
{
   if (testInterface_) return invoke(CtsApi::haltServer(true/*auto_confirm*/));
   return invoke(Cmd_ptr(new CtsCmd(CtsCmd::HALT_SERVER)));
}
int ClientInvoker::pingServer() const
{
   if (testInterface_) return invoke( CtsApi::pingServer());
   return invoke(Cmd_ptr(new CtsCmd(CtsCmd::PING)));
}
int ClientInvoker::shutdownServer() const
{
   if (testInterface_) return invoke(CtsApi::shutdownServer(true/*auto_confirm*/));
   return invoke(Cmd_ptr(new CtsCmd(CtsCmd::SHUTDOWN_SERVER)));
}
int ClientInvoker::terminateServer() const
{
   if (testInterface_) return invoke(CtsApi::terminateServer(true/*auto_confirm*/));
   return invoke(Cmd_ptr(new CtsCmd(CtsCmd::TERMINATE_SERVER)));
}
int ClientInvoker::stats() const
{
   if (testInterface_) return invoke(CtsApi::stats());
   return invoke(Cmd_ptr(new CtsCmd(CtsCmd::STATS)));
}
int ClientInvoker::stats_server() const
{
   if (testInterface_) return invoke(CtsApi::stats_server());
   return invoke(Cmd_ptr(new CtsCmd(CtsCmd::STATS_SERVER)));
}
int ClientInvoker::stats_reset() const
{
   if (testInterface_) return invoke(CtsApi::stats_reset());
   return invoke(Cmd_ptr(new CtsCmd(CtsCmd::STATS_RESET)));
}
int ClientInvoker::suites() const
{
   if (testInterface_) return invoke(CtsApi::suites());
   return invoke(Cmd_ptr(new CtsCmd(CtsCmd::SUITES)));
}
int ClientInvoker::server_version() const
{
   if (testInterface_) return invoke(CtsApi::server_version());
   return invoke(Cmd_ptr(new ServerVersionCmd()) );
}
int ClientInvoker::debug_server_on() const
{
   if (testInterface_) return invoke(CtsApi::debug_server_on());
   return invoke(Cmd_ptr(new CtsCmd(CtsCmd::DEBUG_SERVER_ON)));
}
int ClientInvoker::debug_server_off() const
{
   if (testInterface_) return invoke(CtsApi::debug_server_off());
   return invoke(Cmd_ptr(new CtsCmd(CtsCmd::DEBUG_SERVER_OFF)));
}

//=====================================================================================

int ClientInvoker::ch_register( bool auto_add_new_suites,const std::vector<std::string>& suites ) const
{
   if (testInterface_) return invoke(CtsApi::ch_register(auto_add_new_suites, suites));
   return invoke(Cmd_ptr(new ClientHandleCmd(suites, auto_add_new_suites)) );
}
int ClientInvoker::ch_suites() const
{
   if (testInterface_) return invoke(CtsApi::ch_suites());
   return invoke(Cmd_ptr(new ClientHandleCmd(ClientHandleCmd::SUITES)) );
}
int ClientInvoker::ch_drop( int client_handle ) const
{
   if (testInterface_) return invoke(CtsApi::ch_drop(client_handle));
   return invoke(Cmd_ptr(new ClientHandleCmd(client_handle)) );
}
int ClientInvoker::ch_drop_user( const std::string& user) const
{
   if (testInterface_) return invoke(CtsApi::ch_drop_user(user));
   return invoke(Cmd_ptr(new ClientHandleCmd(user)) );
}
int ClientInvoker::ch_add( int client_handle, const std::vector<std::string>& suites ) const
{
   if (testInterface_) return invoke(CtsApi::ch_add(client_handle, suites));
   return invoke(Cmd_ptr(new ClientHandleCmd(client_handle, suites, ClientHandleCmd::ADD)) );
}
int ClientInvoker::ch_remove( int client_handle, const std::vector<std::string>& suites ) const
{
   if (testInterface_) return invoke(CtsApi::ch_remove(client_handle, suites));
   return invoke(Cmd_ptr(new ClientHandleCmd(client_handle, suites, ClientHandleCmd::REMOVE)) );
}
int ClientInvoker::ch_auto_add( int client_handle, bool auto_add_new_suites ) const
{
   if (testInterface_) return invoke(CtsApi::ch_auto_add(client_handle, auto_add_new_suites));
   return invoke(Cmd_ptr(new ClientHandleCmd(client_handle,auto_add_new_suites)) );
}
int ClientInvoker::ch1_drop() const
{
   if (0 == server_reply_.client_handle()) return 0;
   if (testInterface_) return invoke(CtsApi::ch_drop(server_reply_.client_handle()));
   return invoke(Cmd_ptr(new ClientHandleCmd(server_reply_.client_handle())) );
}
int ClientInvoker::ch1_add( const std::vector<std::string>& suites ) const
{
   if (testInterface_) return invoke(CtsApi::ch_add(server_reply_.client_handle(), suites));
   return invoke(Cmd_ptr(new ClientHandleCmd(server_reply_.client_handle(), suites, ClientHandleCmd::ADD)) );
}
int ClientInvoker::ch1_remove( const std::vector<std::string>& suites ) const
{
   if (testInterface_) return invoke(CtsApi::ch_remove(server_reply_.client_handle(), suites));
   return invoke(Cmd_ptr(new ClientHandleCmd(server_reply_.client_handle(), suites, ClientHandleCmd::REMOVE)) );
}
int ClientInvoker::ch1_auto_add( bool auto_add_new_suites ) const
{
   if (testInterface_) return invoke(CtsApi::ch_auto_add(server_reply_.client_handle(), auto_add_new_suites));
   return invoke(Cmd_ptr(new ClientHandleCmd(server_reply_.client_handle(),auto_add_new_suites)) );
}

// ======================================================================================================

int ClientInvoker::begin( const std::string& suiteName, bool force ) const
{
   if (testInterface_) return invoke(CtsApi::begin(suiteName, force));
   return invoke(Cmd_ptr(new BeginCmd(suiteName, force )) );
}
int ClientInvoker::begin_all_suites( bool force ) const
{
   if (testInterface_) return invoke(CtsApi::begin("", force));
   return invoke(Cmd_ptr(new BeginCmd("", force )) );
}
// ======================================================================================================

int ClientInvoker::zombieGet() const
{
   if (testInterface_) return invoke(CtsApi::zombieGet());
   return invoke(Cmd_ptr(new CtsCmd(CtsCmd::GET_ZOMBIES)));
}
int ClientInvoker::zombieFob( const Zombie& z ) const
{
   if (testInterface_) return invoke(CtsApi::zombieFob(std::vector<std::string>(1,z.path_to_task()), z.process_or_remote_id(), z.jobs_password()));
   return invoke(Cmd_ptr(new ZombieCmd(User::FOB, std::vector<std::string>(1,z.path_to_task()), z.process_or_remote_id(), z.jobs_password() )));
}
int ClientInvoker::zombieFail( const Zombie& z ) const
{
   if (testInterface_) return invoke(CtsApi::zombieFail(std::vector<std::string>(1,z.path_to_task()), z.process_or_remote_id(), z.jobs_password()));
   return invoke(Cmd_ptr(new ZombieCmd(User::FAIL, std::vector<std::string>(1,z.path_to_task()), z.process_or_remote_id(), z.jobs_password() )));
}
int ClientInvoker::zombieAdopt( const Zombie& z ) const
{
   if (testInterface_) return invoke(CtsApi::zombieAdopt(std::vector<std::string>(1,z.path_to_task()), z.process_or_remote_id(), z.jobs_password()));
   return invoke(Cmd_ptr(new ZombieCmd(User::ADOPT, std::vector<std::string>(1,z.path_to_task()), z.process_or_remote_id(), z.jobs_password() )));
}
int ClientInvoker::zombieBlock( const Zombie& z ) const
{
   if (testInterface_) return invoke(CtsApi::zombieBlock(std::vector<std::string>(1,z.path_to_task()), z.process_or_remote_id(), z.jobs_password()));
   return invoke(Cmd_ptr(new ZombieCmd(User::BLOCK, std::vector<std::string>(1,z.path_to_task()), z.process_or_remote_id(), z.jobs_password() )));
}
int ClientInvoker::zombieRemove( const Zombie& z ) const
{
   if (testInterface_) return invoke(CtsApi::zombieRemove(std::vector<std::string>(1,z.path_to_task()), z.process_or_remote_id(), z.jobs_password()));
   return invoke(Cmd_ptr(new ZombieCmd(User::REMOVE, std::vector<std::string>(1,z.path_to_task()), z.process_or_remote_id(), z.jobs_password() )));
}
int ClientInvoker::zombieKill( const Zombie& z ) const
{
   if (testInterface_) return invoke(CtsApi::zombieKill(std::vector<std::string>(1,z.path_to_task()), z.process_or_remote_id(), z.jobs_password()));
   return invoke(Cmd_ptr(new ZombieCmd(User::KILL, std::vector<std::string>(1,z.path_to_task()), z.process_or_remote_id(), z.jobs_password() )));
}
int ClientInvoker::zombieFobCli( const std::string& absNodePath ) const
{
   if (testInterface_) return invoke(CtsApi::zombieFobCli(absNodePath));
   return invoke(Cmd_ptr(new ZombieCmd(User::FOB, std::vector<std::string>(1,absNodePath),"","")));
}
int ClientInvoker::zombieFailCli( const std::string& absNodePath ) const
{
   if (testInterface_) return invoke(CtsApi::zombieFailCli(absNodePath));
   return invoke(Cmd_ptr(new ZombieCmd(User::FAIL, std::vector<std::string>(1,absNodePath),"","" )));
}
int ClientInvoker::zombieAdoptCli( const std::string& absNodePath ) const
{
   if (testInterface_) return invoke(CtsApi::zombieAdoptCli(absNodePath));
   return invoke(Cmd_ptr(new ZombieCmd(User::ADOPT, std::vector<std::string>(1,absNodePath),"","" )));
}
int ClientInvoker::zombieBlockCli( const std::string& absNodePath ) const
{
   if (testInterface_) return invoke(CtsApi::zombieBlockCli(absNodePath));
   return invoke(Cmd_ptr(new ZombieCmd(User::BLOCK, std::vector<std::string>(1,absNodePath),"","" )));
}
int ClientInvoker::zombieRemoveCli( const std::string& absNodePath ) const
{
   if (testInterface_) return invoke(CtsApi::zombieRemoveCli(absNodePath));
   return invoke(Cmd_ptr(new ZombieCmd(User::REMOVE, std::vector<std::string>(1,absNodePath),"","" )));
}
int ClientInvoker::zombieKillCli( const std::string& absNodePath ) const
{
   if (testInterface_) return invoke(CtsApi::zombieKillCli(absNodePath));
   return invoke(Cmd_ptr(new ZombieCmd(User::KILL, std::vector<std::string>(1,absNodePath),"","" )));
}

int ClientInvoker::zombieFobCliPaths(const std::vector<std::string>& paths) const {
   if (testInterface_) return invoke(CtsApi::zombieFobCli(paths));
   return invoke(Cmd_ptr(new ZombieCmd(User::FOB, paths,"","")));
}
int ClientInvoker::zombieFailCliPaths(const std::vector<std::string>& paths) const {
   if (testInterface_) return invoke(CtsApi::zombieFailCli(paths));
   return invoke(Cmd_ptr(new ZombieCmd(User::FAIL, paths,"","" )));
}
int ClientInvoker::zombieAdoptCliPaths(const std::vector<std::string>& paths) const {
   if (testInterface_) return invoke(CtsApi::zombieAdoptCli(paths));
   return invoke(Cmd_ptr(new ZombieCmd(User::ADOPT, paths,"","" )));
}
int ClientInvoker::zombieBlockCliPaths(const std::vector<std::string>& paths) const {
   if (testInterface_) return invoke(CtsApi::zombieBlockCli(paths));
   return invoke(Cmd_ptr(new ZombieCmd(User::BLOCK, paths,"","" )));
}
int ClientInvoker::zombieRemoveCliPaths(const std::vector<std::string>& paths) const {
   if (testInterface_) return invoke(CtsApi::zombieRemoveCli(paths));
   return invoke(Cmd_ptr(new ZombieCmd(User::REMOVE, paths,"","" )));
}
int ClientInvoker::zombieKillCliPaths(const std::vector<std::string>& paths) const {
   if (testInterface_) return invoke(CtsApi::zombieKillCli(paths));
   return invoke(Cmd_ptr(new ZombieCmd(User::KILL, paths,"","" )));
}


// ======================================================================================================

int ClientInvoker::job_gen( const std::string& absNodePath ) const
{
   if (testInterface_) return invoke(CtsApi::job_gen(absNodePath));
   return invoke(Cmd_ptr(new CtsNodeCmd( CtsNodeCmd::JOB_GEN, absNodePath)));
}

int ClientInvoker::edit_history( const std::string& path ) const
{
   if (testInterface_) return invoke(CtsApi::edit_history(path));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::EDIT_HISTORY, path)));
}
int ClientInvoker::kill( const std::vector<std::string>& paths ) const
{
   if (testInterface_) return invoke(CtsApi::kill(paths));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::KILL, paths)));
}
int ClientInvoker::kill( const std::string& absNodePath ) const
{
   if (testInterface_) return invoke(CtsApi::kill(absNodePath));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::KILL, absNodePath)));
}
int ClientInvoker::status( const std::vector<std::string>& paths ) const
{
   if (testInterface_) return invoke(CtsApi::status(paths));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::STATUS, paths)));
}
int ClientInvoker::status( const std::string& absNodePath ) const
{
   if (testInterface_) return invoke(CtsApi::status(absNodePath));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::STATUS, absNodePath)));
}
int ClientInvoker::suspend( const std::vector<std::string>& paths ) const
{
   if (testInterface_) return invoke(CtsApi::suspend(paths));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::SUSPEND, paths)));
}
int ClientInvoker::suspend( const std::string& absNodePath ) const
{
   if (testInterface_) return invoke(CtsApi::suspend(absNodePath));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::SUSPEND, absNodePath)));
}
int ClientInvoker::resume( const std::vector<std::string>& paths ) const
{
   if (testInterface_) return invoke(CtsApi::resume(paths));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::RESUME, paths)));
}
int ClientInvoker::resume( const std::string& absNodePath ) const
{
   if (testInterface_) return invoke(CtsApi::resume(absNodePath));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::RESUME, absNodePath)));
}
int ClientInvoker::check( const std::vector<std::string>& paths ) const
{
   if (testInterface_) return invoke(CtsApi::check(paths));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::CHECK, paths)));
}
int ClientInvoker::check( const std::string& absNodePath ) const
{
   if (testInterface_) return invoke(CtsApi::check(absNodePath));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::CHECK, absNodePath)));
}
int ClientInvoker::delete_nodes( const std::vector<std::string>& paths, bool force ) const
{
   if (testInterface_) return invoke(CtsApi::delete_node(paths, force, true/*auto_confirm*/));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::DELETE, paths,force)));
}
int ClientInvoker::delete_node( const std::string& absNodePath, bool force ) const
{
   if (testInterface_) return invoke(CtsApi::delete_node(absNodePath, force, true/*auto_confirm*/));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::DELETE, absNodePath,force)));
}
int ClientInvoker::delete_all( bool force) const
{
   if (testInterface_) return invoke(CtsApi::delete_node(std::vector<std::string>(),force));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::DELETE, std::vector<std::string>(),force)));
}

int ClientInvoker::archive(const std::vector<std::string>& paths,bool force) const {
   if (testInterface_) return invoke(CtsApi::archive(paths,force));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::ARCHIVE, paths, force)));
}
int ClientInvoker::archive(const std::string& absNodePath,bool force) const {
   if (testInterface_) return invoke(CtsApi::archive(absNodePath,force));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::ARCHIVE, absNodePath, force)));
}
int ClientInvoker::restore(const std::vector<std::string>& paths) const {
   if (testInterface_) return invoke(CtsApi::restore(paths));
    return invoke(Cmd_ptr(new PathsCmd( PathsCmd::RESTORE, paths)));
}
int ClientInvoker::restore(const std::string& absNodePath) const {
   if (testInterface_) return invoke(CtsApi::restore(absNodePath));
   return invoke(Cmd_ptr(new PathsCmd( PathsCmd::RESTORE, absNodePath)));
}

// ======================================================================================================

int ClientInvoker::replace( const std::string& absNodePath, const std::string& path_to_client_defs,
                            bool create_parents_as_required, bool force) const
{
   if (testInterface_) return invoke(CtsApi::replace(absNodePath, path_to_client_defs, create_parents_as_required, force));

   /// *Note* server_reply_.client_handle_ is kept until the next call to register_client_handle
   /// The client invoker can be used multiple times, hence keep value of defs, and client handle in server reply
   server_reply_.clear_for_invoke(cli());

   /// Handle command constructors that can throw
   Cmd_ptr cts_cmd;
   try {

      ReplaceNodeCmd* replace_cmd = new ReplaceNodeCmd( absNodePath, create_parents_as_required, path_to_client_defs, force);

      // For test allow the defs environment to changed, i.e. allow us to inject  ECF_CLIENT ???
      replace_cmd->theDefs()->set_server().add_or_update_user_variables( clientEnv_.env() );

      cts_cmd = Cmd_ptr( replace_cmd );
   }
   catch (std::exception& e ){
      std::stringstream ss; ss << "ecflow:ClientInvoker::replace(" << absNodePath << "," << path_to_client_defs << ", ...) failed: " << e.what();
      server_reply_.set_error_msg( ss.str() );
      if (on_error_throw_exception_) throw std::runtime_error( server_reply_.error_msg() );
      return 1;
   }

   return invoke( cts_cmd );
}

int ClientInvoker::replace_1(const std::string& absNodePath, defs_ptr client_defs, bool create_parents_as_required, bool force)  const
{
   /// *Note* server_reply_.client_handle_ is kept until the next call to register_client_handle
   /// The client invoker can be used multiple times, hence keep value of defs, and client handle in server reply
   server_reply_.clear_for_invoke(cli());

   /// Handle command constructors that can throw
   Cmd_ptr cts_cmd;
   try {
      cts_cmd = Cmd_ptr( new ReplaceNodeCmd( absNodePath, create_parents_as_required, client_defs, force) );
   }
   catch (std::exception& e ){
      std::stringstream ss; ss << "ecflow:ClientInvoker::replace_1(" << absNodePath << " ...) failed: " << e.what();
      server_reply_.set_error_msg( ss.str() );
      if (on_error_throw_exception_) throw std::runtime_error( server_reply_.error_msg() );
      return 1;
   }

   return invoke( cts_cmd );
}

int ClientInvoker::requeue( const std::vector<std::string>& paths,  const std::string& option ) const
{
   if (testInterface_) return invoke(CtsApi::requeue(paths, option));

   RequeueNodeCmd::Option the_option = RequeueNodeCmd::NO_OPTION;
   if (!option.empty()) {
      if (option == "abort") the_option = RequeueNodeCmd::ABORT;
      else if (option == "force") the_option = RequeueNodeCmd::FORCE;
      else {
         server_reply_.set_error_msg("ecflow:ClientInvoker::requeue: Expected option = [ force | abort ]");
         if (on_error_throw_exception_) {
            throw std::runtime_error(server_reply_.error_msg());
         }
         return 1;
      }
   }
   return invoke(Cmd_ptr(new RequeueNodeCmd(paths, the_option)));
}
int ClientInvoker::requeue( const std::string& absNodePath, const std::string& option) const
{
   if (testInterface_) return invoke(CtsApi::requeue(absNodePath, option));

   RequeueNodeCmd::Option the_option = RequeueNodeCmd::NO_OPTION;
   if (!option.empty()) {
      if (option == "abort") the_option = RequeueNodeCmd::ABORT;
      else if (option == "force") the_option = RequeueNodeCmd::FORCE;
      else {
         server_reply_.set_error_msg("ecflow:ClientInvoker::requeue: Expected option = [ force | abort ]");
         if (on_error_throw_exception_) throw std::runtime_error(server_reply_.error_msg());
         return 1;
      }
   }
   return invoke(Cmd_ptr(new RequeueNodeCmd(absNodePath, the_option)));
}

int ClientInvoker::run( const std::vector<std::string>& paths, bool force ) const
{
   if (testInterface_) return invoke(CtsApi::run(paths, force));
   return invoke(Cmd_ptr(new RunNodeCmd(paths, force)));
}
int ClientInvoker::run( const std::string& absNodePath, bool force ) const
{
   if (testInterface_) return invoke(CtsApi::run(absNodePath, force));
   return invoke(Cmd_ptr(new RunNodeCmd(absNodePath, force)));
}
int ClientInvoker::order( const std::string& absNodePath, const std::string& order ) const
{
   if (testInterface_) return invoke(CtsApi::order(absNodePath, order));

   if (!NOrder::isValid(order)) {
      server_reply_.set_error_msg("ecflow:ClientInvoker::order: please specify one of [ top, bottom, alpha, order, up, down ]\n");
      if (on_error_throw_exception_) throw std::runtime_error(server_reply_.error_msg());
      return 1;
   }
   return invoke(Cmd_ptr(new OrderNodeCmd(absNodePath, NOrder::toOrder(order))));
}
int ClientInvoker::order(const std::string& absNodePath,NOrder::Order order) const
{
   return invoke(Cmd_ptr(new OrderNodeCmd(absNodePath, order)));
}

// ======================================================================================================

int ClientInvoker::checkPtDefs(ecf::CheckPt::Mode m,int check_pt_interval,int check_pt_save_time_alarm) const
{
   if (testInterface_) return invoke(CtsApi::checkPtDefs(m, check_pt_interval, check_pt_save_time_alarm));
   return invoke(Cmd_ptr(new CheckPtCmd(m,check_pt_interval,check_pt_save_time_alarm)));
}
int ClientInvoker::restoreDefsFromCheckPt() const
{
   if (testInterface_) return invoke(CtsApi::restoreDefsFromCheckPt());
   return invoke(Cmd_ptr(new CtsCmd( CtsCmd::RESTORE_DEFS_FROM_CHECKPT )));
}

int ClientInvoker::force( const std::string& absNodePath, const std::string& state_or_event,bool recursive, bool set_repeats_to_last_value ) const
{
   if (testInterface_) return invoke(CtsApi::force(absNodePath, state_or_event, recursive, set_repeats_to_last_value));
   return invoke(Cmd_ptr(new ForceCmd(absNodePath, state_or_event, recursive, set_repeats_to_last_value  )));
}
int ClientInvoker::force( const std::vector<std::string>& paths, const std::string& state_or_event,bool recursive, bool set_repeats_to_last_value) const
{
   if (testInterface_) return invoke(CtsApi::force(paths, state_or_event, recursive, set_repeats_to_last_value));
   return invoke(Cmd_ptr(new ForceCmd(paths, state_or_event, recursive, set_repeats_to_last_value  )));
}

int ClientInvoker::freeDep( const std::vector<std::string>& paths, bool trigger,bool all, bool date, bool the_time ) const
{
   if (testInterface_) return invoke(CtsApi::freeDep(paths, trigger, all, date, the_time));
   return invoke(Cmd_ptr(new FreeDepCmd(paths, trigger, all, date , the_time)));
}
int ClientInvoker::freeDep( const std::string& absNodePath, bool trigger, bool all,bool date, bool the_time ) const
{
   if (testInterface_) return invoke(CtsApi::freeDep(absNodePath, trigger, all, date, the_time));
   return invoke(Cmd_ptr(new FreeDepCmd(absNodePath, trigger, all, date , the_time)));
}

int ClientInvoker::file( const std::string& absNodePath, const std::string& fileType, const std::string& max_lines ) const
{
   if (testInterface_) return invoke(CtsApi::file(absNodePath, fileType, max_lines));

   /// Handle command constructors that can throw
   Cmd_ptr cts_cmd;
   try {
      cts_cmd = Cmd_ptr( new CFileCmd(absNodePath, fileType, max_lines));
   }
   catch (std::exception& e ){
      std::stringstream ss; ss << "ecflow:ClientInvoker::file(" << absNodePath << "," << fileType << "," << max_lines << ") failed:\n" << e.what();
      server_reply_.set_error_msg( ss.str() );
      if (on_error_throw_exception_) {
         throw std::runtime_error( server_reply_.error_msg() );
      }
      return 1;
   }

   return invoke( cts_cmd );
}

int ClientInvoker::plug( const std::string& sourcePath, const std::string& destPath ) const
{
   if (testInterface_) return invoke(CtsApi::plug(sourcePath, destPath));
   return invoke(Cmd_ptr(new PlugCmd(sourcePath, destPath)));
}

// ======================================================================================================

int ClientInvoker::reloadwsfile() const
{
   if (testInterface_) return invoke(CtsApi::reloadwsfile());
   return invoke(Cmd_ptr(new CtsCmd( CtsCmd::RELOAD_WHITE_LIST_FILE )));
}

int ClientInvoker::reloadpasswdfile() const
{
   if (testInterface_) return invoke(CtsApi::reloadpasswdfile());
   return invoke(Cmd_ptr(new CtsCmd( CtsCmd::RELOAD_PASSWD_FILE )));
}

int ClientInvoker::group( const std::string& groupRequest ) const
{
   if (testInterface_)  return invoke(CtsApi::group(groupRequest));
   return invoke(Cmd_ptr(new GroupCTSCmd(groupRequest,&clientEnv_)));
}

int ClientInvoker::logMsg( const std::string& msg ) const
{
   if (testInterface_) return invoke(CtsApi::logMsg(msg));
   return invoke(Cmd_ptr(new LogMessageCmd( msg )));
}
int ClientInvoker::new_log( const std::string& new_path) const
{
   if (testInterface_) return invoke(CtsApi::new_log(new_path));

   /// Handle command constructors that can throw
   Cmd_ptr cts_cmd;
   try {
      cts_cmd = Cmd_ptr(new LogCmd( new_path ));
   }
   catch (std::exception& e ){
      server_reply_.set_error_msg( e.what() );
      if (on_error_throw_exception_) throw std::runtime_error( server_reply_.error_msg() );
      return 1;
   }
   return invoke(cts_cmd);
}
int ClientInvoker::getLog( int lastLines) const
{
   if (lastLines == 0) lastLines = Log::get_last_n_lines_default();
   if (testInterface_) return invoke(CtsApi::getLog(lastLines));
   return invoke(Cmd_ptr(new LogCmd( LogCmd::GET, lastLines )));
}
int ClientInvoker::clearLog() const
{
   if (testInterface_) return invoke(CtsApi::clearLog());
   return invoke(Cmd_ptr(new LogCmd( LogCmd::CLEAR )));
}
int ClientInvoker::flushLog() const
{
   if (testInterface_) return invoke(CtsApi::flushLog());
   return invoke(Cmd_ptr(new LogCmd( LogCmd::FLUSH )));
}
int ClientInvoker::get_log_path() const
{
   if (testInterface_) return invoke(CtsApi::get_log_path());
   return invoke(Cmd_ptr(new LogCmd( LogCmd::PATH )));
}

int ClientInvoker::forceDependencyEval() const
{
   return invoke(CtsApi::forceDependencyEval());
}

// ======================================================================================================

int ClientInvoker::edit_script_edit(const std::string& path_to_task)
{
   return invoke(Cmd_ptr( new EditScriptCmd( path_to_task, EditScriptCmd::EDIT) ) );
}

int ClientInvoker::edit_script_preprocess(const std::string& path_to_task)
{
   return invoke(Cmd_ptr( new EditScriptCmd( path_to_task, EditScriptCmd::PREPROCESS) ) );
}

int ClientInvoker::edit_script_preprocess(const std::string& path_to_task,const std::vector<std::string>& file_contents)
{
   return invoke(Cmd_ptr( new EditScriptCmd( path_to_task,file_contents) ) );
}

int ClientInvoker::edit_script_submit(const std::string& path_to_task,const NameValueVec& used_variables)
{
   return invoke(Cmd_ptr( new EditScriptCmd( path_to_task,used_variables) ) );
}

int ClientInvoker::edit_script_submit(
         const std::string& path_to_task,
         const NameValueVec& used_variables,
         const std::vector<std::string>& file_contents,
         bool create_alias,
         bool run_alias)
{
   return invoke(Cmd_ptr( new EditScriptCmd( path_to_task,used_variables,file_contents,create_alias,run_alias) ) );
}

std::string ClientInvoker::client_env_host_port() const
{
   std::string host_port = clientEnv_.host();
   host_port += Str::COLON();
   host_port += clientEnv_.port();
   return host_port;
}

std::string ClientInvoker::find_free_port(int seed_port_number, bool debug)
{
   // Ping failed, We need to distinguish between:
   //    a/ Server does not exist : <FREE> port
   //    b/ Address in use        : <BUSY> port on existing server
   // Using server_version() but then get error messages
   // ******** Until this is done we can't implement port hopping **********

   if (debug) cout << "  ClientInvoker::find_free_port: starting with port " << seed_port_number << "\n";
   int the_port = seed_port_number;
   std::string free_port;
   ClientInvoker client;
   client.set_retry_connection_period(1); // avoid long wait
   client.set_connection_attempts(1);     // avoid long wait
   while (1) {
      free_port = boost::lexical_cast<std::string>(the_port);
      try {
         if (debug) cout << "   Trying to connect to server on '" << Str::LOCALHOST() << ":" << free_port << "'\n";
         client.set_host_port(Str::LOCALHOST(),free_port);
         client.pingServer();
         if (debug) cout << "   Connected to server on port " << free_port << " trying next port\n";
         the_port++;
      }
      catch ( std::runtime_error& e) {
         std::string error_msg = e.what();
         if (debug) cout << "   " << e.what();
         if (error_msg.find("authentication failed") != std::string::npos) {
            if (debug) cout << "   Could not connect, due to authentication failure, hence port " << the_port << " is used,trying next port\n";
            the_port++;
            continue;
         }
         else {
            if (debug) cout << "   Found free port " << free_port << "\n";
            break;
         }
      }
   }
   return free_port;
}

bool ClientInvoker::wait_for_server_reply(int time_out) const
{
   DurationTimer timer;
   while(1) {
      sleep(2);

      if (on_error_throw_exception_) {
         try {
            pingServer(); // will throw exception
            return true;  // no exception, server lives
         }
         catch( ... ) {}
      }
      else {
         if (pingServer() == 0) {
            return true; // ping OK,
         }
      }
      if (timer.duration() > time_out) {
         return false;
      }
   }
   return false;
}

bool ClientInvoker::wait_for_server_death(int time_out) const
{
   DurationTimer timer;
   while(1) {

      if (on_error_throw_exception_) {
         try {
            pingServer(); // will throw exception
         }
         catch( ... ) {
            // server died
            return true;
         }
      }
      else {
         if (pingServer() == 1) {
            return true; // ping failed, server has died,
         }
      }
      if (timer.duration() > time_out) {
         return false; // server still lives
      }

      // Ping ok, server lives, continue pinging, until timeout
      sleep(2);
   }
   return false;
}


int ClientInvoker::load_in_memory_defs( const defs_ptr& clientDefs, bool force) const
{
   /// *Note* server_reply_.client_handle_ is kept until the next call to register_client_handle
   /// The client invoker can be used multiple times, hence keep value of defs, and client handle in server reply
   server_reply_.clear_for_invoke(cli());

   if ( !clientDefs.get() ) {
      server_reply_.set_error_msg("The client definition is empty.");
      if (on_error_throw_exception_) throw std::runtime_error(server_reply_.error_msg());
      return 1;
   }

   // Client defs  has been created in memory.
   // warn about naff expression and unresolved in-limit references to Limit's
   // Don't allow defs to be loaded into server, with trigger parser errors.
   std::string warningMsg;
   if (!clientDefs->check(server_reply_.get_error_msg(), warningMsg)) {
      if (on_error_throw_exception_) throw std::runtime_error(server_reply_.error_msg());
      return 1;
   }

   return invoke( Cmd_ptr( new LoadDefsCmd( clientDefs, force /*force overwrite suite of same name*/) ) );
}


// ==========================================================================
// Python child support
// ==========================================================================
void ClientInvoker::set_child_path(const std::string& path)
{
   child_task_path_ = path;
}
void ClientInvoker::set_child_password(const std::string& pass)
{
   child_task_password_ = pass;
}
void ClientInvoker::set_child_pid(const std::string& pid)
{
   child_task_pid_ = pid;
}
void ClientInvoker::set_child_try_no(unsigned int try_no)
{
   child_task_try_no_ = try_no;
}
void ClientInvoker::set_child_timeout(unsigned int seconds )
{
   clientEnv_.set_child_cmd_timeout(seconds);
}
void ClientInvoker::set_zombie_child_timeout(unsigned int seconds )
{
   clientEnv_.set_zombie_child_cmd_timeout(seconds);
}


void ClientInvoker::check_child_parameters() const
{
   if (clientEnv_.debug()) {
      std::cout << "  child_task_path_ = '" << child_task_path_ << "'\n";
      std::cout << "  child_task_password_ = '" << child_task_password_ << "'\n";
      std::cout << "  child_task_pid_ = '" << child_task_pid_ << "'\n";
      std::cout << "  child_task_try_no_ = " << child_task_try_no_ << "\n";
   }
   if (child_task_path_.empty()) throw std::runtime_error("Child Path not set");
   if (child_task_password_.empty()) throw std::runtime_error("Child password not set");
   if (child_task_pid_.empty())  throw std::runtime_error("Child pid not set");
   if (child_task_try_no_ == 0)  throw std::runtime_error("Child try_no not set");
}

void ClientInvoker::child_init()
{
   check_child_parameters();
   on_error_throw_exception_ = true; // for python always throw exception
   invoke( Cmd_ptr( new InitCmd(child_task_path_, child_task_password_, child_task_pid_, child_task_try_no_  ) ) );
}

void ClientInvoker::child_abort(const std::string& reason )
{
   check_child_parameters();
   on_error_throw_exception_ = true; // for python always throw exception
   invoke( Cmd_ptr( new AbortCmd(child_task_path_, child_task_password_, child_task_pid_, child_task_try_no_,reason  ) ) );
}

void ClientInvoker::child_event(const std::string& event_name_or_number)
{
   check_child_parameters();
   on_error_throw_exception_ = true; // for python always throw exception
   invoke( Cmd_ptr( new EventCmd(child_task_path_, child_task_password_, child_task_pid_, child_task_try_no_,event_name_or_number  ) ) );
}

void ClientInvoker::child_meter(const std::string& meter_name, int meter_value)
{
   if (meter_name.empty())  throw std::runtime_error("Meter name not set");
   check_child_parameters();
   on_error_throw_exception_ = true; // for python always throw exception
   invoke( Cmd_ptr( new MeterCmd(child_task_path_, child_task_password_, child_task_pid_, child_task_try_no_,meter_name,meter_value  ) ) );
}

void ClientInvoker::child_label(const std::string& label_name, const std::string& label_value)
{
   if (label_name.empty()) throw std::runtime_error("Label name not set");
   check_child_parameters();
   on_error_throw_exception_ = true; // for python always throw exception
   invoke( Cmd_ptr( new LabelCmd(child_task_path_, child_task_password_, child_task_pid_, child_task_try_no_,label_name,label_value  ) ) );
}

void ClientInvoker::child_wait(const std::string& expression)
{
   check_child_parameters();
   on_error_throw_exception_ = true; // for python always throw exception
   invoke( Cmd_ptr( new CtsWaitCmd(child_task_path_, child_task_password_, child_task_pid_, child_task_try_no_, expression ) ) );
}

std::string ClientInvoker::child_queue(const std::string& queue_name, const std::string& path_to_node_with_queue)
{
   if (queue_name.empty()) throw std::runtime_error("ClientInvoker::child_queue:  Queue name not set");
   check_child_parameters();
   on_error_throw_exception_ = true; // for python always throw exception
   invoke( Cmd_ptr( new QueueCmd(child_task_path_,child_task_password_,child_task_pid_,child_task_try_no_,queue_name,path_to_node_with_queue)));
   return server_reply_.get_string();
}

void ClientInvoker::child_complete()
{
   check_child_parameters();
   on_error_throw_exception_ = true; // for python always throw exception
   invoke( Cmd_ptr( new CompleteCmd(child_task_path_, child_task_password_, child_task_pid_, child_task_try_no_  ) ) );
}

// ==========================================================================
// class RequestLogger:
// ==========================================================================
RequestLogger::RequestLogger(const ClientInvoker* ci) : ci_(ci){}
RequestLogger::~RequestLogger() {

   // *assumes* destructor of RoundTripRecorder was invoked first, to allow recording of the time rtt_
   if (cmd_.get()) {
      if (ci_->clientEnv_.debug() && ci_->server_reply_.error_msg().empty()) {
         cout << TimeStamp::now() << "ClientInvoker "; cmd_->print(cout); cout << " SUCCEDED " << to_simple_string(ci_->rtt_) << "\n";
      }

      if (Rtt::instance()) {
         std::stringstream ss;
         ss << ci_->client_env_host_port() << " ";
         cmd_->print(ss);
         ss << " " << Rtt::tag() << to_simple_string(ci_->rtt_); // Note: endl added rtt(..)
         ss << " : " << ci_->server_reply_.error_msg();
         rtt(ss.str());
      }

      if (ci_->cli() && cmd_->ping_cmd() && ci_->server_reply_.error_msg().empty()) {
         cout << "ping server(" << ci_->client_env_host_port() << ") succeeded in " << to_simple_string(ci_->rtt_) << "  ~" << ci_->rtt_.total_milliseconds() << " milliseconds\n";
      }
   }
}

// ==========================================================================
// class RoundTripRecorder:
// ==========================================================================
RoundTripRecorder::RoundTripRecorder(const ClientInvoker* ci)
: ci_(ci)
{
   // get the current time from the clock -- one second resolution
   ci_->start_time_ = microsec_clock::universal_time();
   ci_->rtt_ = boost::posix_time::time_duration();
}

RoundTripRecorder::~RoundTripRecorder() {
   ci_->rtt_ = microsec_clock::universal_time() - ci_->start_time_;
}
