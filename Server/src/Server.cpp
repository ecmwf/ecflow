//============================================================================
// Name        : Server.cpp
// Author      : Avi
// Revision    : $Revision: #173 $
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Server
//
// The port numbers are divided into three ranges:
// o the Well Known Ports, (require root permission)  0   -1023
//  o the Registered Ports,                            1024-49151
//  o Dynamic and/or Private Ports.                    49151-65535
//============================================================================

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/bind.hpp>
#include <iostream>

#include "Server.hpp" // Must come before boost/serialization headers.
                       // defines ECFLOW_MT
#include <boost/thread/thread.hpp> // needed for ECFLOW_MT and debug() to print thread ID
#include "Defs.hpp"
#include "Log.hpp"
#include "System.hpp"
#include "ServerEnvironment.hpp"
#include "Ecf.hpp"
#include "Calendar.hpp"
#include "Version.hpp"
#include "Str.hpp"

using boost::asio::ip::tcp;
namespace fs = boost::filesystem;

using namespace std;
using namespace ecf;


/// Constructor opens the acceptor and starts waiting for the first incoming connection.
Server::Server( ServerEnvironment& serverEnv ) :
   io_service_(),
#ifdef ECF_OPENSSL
   context_(boost::asio::ssl::context::sslv23),
#endif
   signals_(io_service_),
   acceptor_(io_service_),
#ifdef ECFLOW_MT
   strand_(io_service_),
   thread_pool_size_(serverEnv.threads()),
   new_connection_(),
#endif
   defs_(Defs::create()),      // ECFLOW-182
   traverser_   (this,  io_service_, serverEnv ),
   checkPtSaver_(this,  io_service_, &serverEnv ),
   serverState_(SState::HALTED),
   serverEnv_(serverEnv)
{
#ifdef ECFLOW_MT
   std::cout << "Server: thread pool size = " << thread_pool_size_ << endl;
#endif

   if (serverEnv_.debug()) cout << "-->Server::server starting server on port "
                                << serverEnv.port()
#ifdef ECFLOW_MT
                                << " thread pool size = " << thread_pool_size_
#endif
                                << endl;

   // Register to handle the signals.
   // Support for emergency check pointing during system session.
   signals_.add(SIGTERM);
   signals_.async_wait(boost::bind(&Server::sigterm_signal_handler, this));


   // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
   boost::asio::ip::tcp::endpoint endpoint(serverEnv.tcp_protocol(), serverEnv.port());
   acceptor_.open(endpoint.protocol());
   acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
   acceptor_.bind(endpoint);
   acceptor_.listen();   // address is use error, when it comes, bombs out here

#ifdef ECF_OPENSSL
   context_.set_options(
       boost::asio::ssl::context::default_workarounds
       | boost::asio::ssl::context::no_sslv2
       | boost::asio::ssl::context::single_dh_use);
   context_.set_password_callback(boost::bind(&Server::get_password, this));
   context_.use_certificate_chain_file("server.crt");
   context_.use_private_key_file("server.key", boost::asio::ssl::context::pem);
   context_.use_tmp_dh_file("dh1024.pem");
#endif


   // Update stats, this is returned via --stats command option
   stats().host_ = serverEnv.hostPort().first;
   stats().port_ = serverEnv.hostPort().second;
   stats().job_sub_interval_ = serverEnv.submitJobsInterval();
   stats().checkpt_interval_ = serverEnv.checkPtInterval();
   stats().checkpt_save_time_alarm_ = serverEnv.checkpt_save_time_alarm();
   stats().checkpt_mode_ = serverEnv.checkMode();
   stats().up_since_ = to_simple_string(Calendar::second_clock_time());
   stats().version_ = Version::description();
   stats().status_ = static_cast<int>(serverState_);
   stats().ECF_HOME_ = serverEnv.ecf_home();
   stats().ECF_CHECK_ = serverEnv.checkPtFilename();
   stats().ECF_LOG_ = Log::instance()->path();

   // Update log file:
   ecf::log(Log::MSG, "Server initial state is HALTED");

   // The defs_ *MUST* be updated with the server state
   // When we load from the check pt file we call update_defs_server_state();
   if (!load_check_pt_file_on_startup()) {

      // No check pt files loaded, update defs, with server state
      update_defs_server_state();           // works on def_
   }

   /// Setup globals used to detect incremental changes to the definition
   Ecf::set_server(true);

   // Start an accept operation for a new connection.
   start_accept();
}

Server::~Server()
{
   if (serverEnv_.debug()) cout << "<--Server::~server exiting server on port " << serverEnv_.port() << endl;

   defs_.reset();

#ifdef DEBUG
   if ( defs_.use_count() != 0) {
      cout << "Server::~server() defs_.use_count() = " << defs_.use_count() << " something is still hold onto the defs, asserting\n";
   }
#endif
   assert(defs_.use_count() == 0);
}

void Server::run()
{
  // The io_service::run() call will block until all asynchronous operations
  // have finished. While the server is running, there is always at least one
  // asynchronous operation outstanding: the asynchronous accept call waiting
  // for new incoming connections.

#ifdef ECFLOW_MT
  // Create a pool of threads to run all of the io_services.
  std::vector<boost::shared_ptr<boost::thread> > threads;
  for (std::size_t i = 0; i < thread_pool_size_; ++i)
  {
      boost::shared_ptr<boost::thread> thread(new boost::thread(
          boost::bind(&boost::asio::io_service::run, &io_service_)));
      threads.push_back(thread);
  }
  // Wait for all threads in the pool to exit.
  for (std::size_t i = 0; i < threads.size(); ++i)
    threads[i]->join();
#else
  io_service_.run();
#endif
}

void Server::start_accept()
{
#ifdef ECFLOW_MT
   if (serverEnv_.debug()) cout << boost::this_thread::get_id() << "   Server::start_accept()" << endl;
   new_connection_.reset(new CConnection(io_service_, this));
   acceptor_.async_accept(new_connection_->socket(),
                          boost::bind(&Server::handle_accept, this,
                                      boost::asio::placeholders::error));
#else

   if (serverEnv_.debug()) cout << "   Server::start_accept()" << endl;

#ifdef ECF_OPENSSL
   connection_ptr new_conn( new connection( io_service_, context_ ) );
#else
   connection_ptr new_conn( new connection( io_service_ ) );
#endif

   if (serverEnv_.allow_old_client_new_server() !=0 ) {
      new_conn->allow_old_client_new_server(serverEnv_.allow_old_client_new_server());
   }

   acceptor_.async_accept( new_conn->socket_ll(),
                           boost::bind( &Server::handle_accept, this,
                                 boost::asio::placeholders::error,
                                 new_conn ) );
#endif // ECFLOW_MT
}

#ifdef ECFLOW_MT
void Server::handle_accept(const boost::system::error_code& e)
{
   // Check whether the server was stopped by a signal before this completion
   // handler had a chance to run.
   if (!acceptor_.is_open()) {
      if (serverEnv_.debug()) cout << boost::this_thread::get_id() << "   Server::handle_accept:  acceptor is closed, returning\n";
      return;
   }

   if (!e) {
      new_connection_->start();
   }
   else {
      LogToCout toCoutAsWell;
      LOG(Log::ERR, "Server::handle_accept error occurred : " <<  e.message());
   }

   start_accept();
}
#else
void Server::handle_accept( const boost::system::error_code& e, connection_ptr conn )
{
   if (serverEnv_.debug()) cout << "   Server::handle_accept" << endl;

   // Check whether the server was stopped by a signal before this completion
   // handler had a chance to run.
   if (!acceptor_.is_open()) {
      if (serverEnv_.debug()) cout << "   Server::handle_accept:  acceptor is closed, returning" << endl;
      return;
   }

   if ( !e ) {
      // Read and interpret message from the client
      if (serverEnv_.debug()) cout << "   Server::handle_accept" << endl;

#ifdef ECF_OPENSSL
         conn->socket().async_handshake(boost::asio::ssl::stream_base::server,
                                 boost::bind(&Server::handle_handshake, this,
                                       boost::asio::placeholders::error,conn ));

#else
      // Successfully accepted a new connection. Determine what the
      // client sent to us. The connection::async_read() function will
      // automatically. serialise the inbound_request_ data structure for us.
      conn->async_read( inbound_request_,
                     boost::bind( &Server::handle_read, this,
                                boost::asio::placeholders::error,conn ) );
#endif

   }
   else {
      if (serverEnv_.debug()) cout << "   Server::handle_accept " << e.message() << endl;
      if (e != boost::asio::error::operation_aborted) {
         // An error occurred. Log it
         LogToCout toCoutAsWell;
         LOG(Log::ERR, "   Server::handle_accept error occurred  " <<  e.message());
      }
   }

   // Start an accept operation for a new connection.
   // *NOTE* previously we had *ONLY* called this if there was no errors
   //        However this would means that server would run out work.
   //        When there were errors.!
   // Moved here to follow the examples used in ASIO.
   // However can this get into an infinite loop ???
   start_accept();
}
#endif

#ifdef ECF_OPENSSL
void Server::handle_handshake(const boost::system::error_code& e,connection_ptr new_conn )
{
   if (serverEnv_.debug()) cout << "   Server::handle_handshake" << endl;

   if (!e)
   {
      // Successfully accepted a new connection. Determine what the
      // client sent to us. The connection::async_read() function will
      // automatically. serialise the inbound_request_ data structure for us.
      new_conn->async_read( inbound_request_,
                     boost::bind( &Server::handle_read, this,
                                boost::asio::placeholders::error,new_conn ) );
   }
   else
   {
      // An error occurred.
      // o/ If client has been killed/disconnected/timed out
      //       Server::handle_read : End of file
      //
      // o/ If a *new* client talks to an *old* server, with an unrecognised request/command
      //    we will see:
      //       Connection::handle_read_data boost::archive::archive_exception unregistered class
      //       Server::handle_read : Invalid argument
      LogToCout toCoutAsWell;
      LOG(Log::ERR, "Server::handle_handshake: " <<  e.message());
      // delete this;
   }
}
#endif


void Server::handle_read(  const boost::system::error_code& e,connection_ptr conn )
{
   /// Handle completion of a write operation.
   // **********************************************************************************
   // This function *must* finish with write, otherwise it ends up being called recursively
   // ***********************************************************************************
   if ( !e ) {

      // See what kind of message we got from the client
      if (serverEnv_.debug()) std::cout << "   Server::handle_read : client request " << inbound_request_ << endl;

      try {
         // Service the in bound request, handling the request will populate the outbound_response_
         // Note:: Handle request will first authenticate
         outbound_response_.set_cmd( inbound_request_.handleRequest( this ) );
      }
      catch (exception& e) {
         outbound_response_.set_cmd( PreAllocatedReply::error_cmd( e.what()  ));
      }

      // Release >= 4.0.6  More reliable to always respond back. Get more accurate logs
      // However allow old/new client to deal with shutdown of socket:
      // See: void Client::handle_read() See: ECFLOW-157, ECFLOW-169
      //
      //      if (!serverEnv_.reply_back_if_ok()) {
      //
      //         if (!inbound_request_.terminateRequest() && outbound_response_.get_cmd()->isOkCmd()) {
      //
      //            // cleanly close down the connection
      //            if (serverEnv_.debug()) cout << "   Server::handle_read: NOT replying, since request is OK" << endl;
      //
      //            if (shutdown_socket(conn,"Server::handle_read:"))  conn->socket().close();
      //            return;
      //         }
      //      }

      // *Reply* back to the client:
      conn->async_write( outbound_response_,
                          boost::bind(&Server::handle_write,
                                    this,
                                    boost::asio::placeholders::error,
                                    conn ) );
   }
   else {
      // An error occurred.
      // o/ If client has been killed/disconnected/timed out
      //       Server::handle_read : End of file
      //
      // o/ If a *new* client talks to an *old* server, with an unrecognised request/command
      //    we will see:
      //       Connection::handle_read_data boost::archive::archive_exception unregistered class
      //       Server::handle_read : Invalid argument
      LogToCout toCoutAsWell;
      LOG(Log::ERR, "Server::handle_read: " <<  e.message());
   }
}

void Server::handle_write( const boost::system::error_code& e, connection_ptr conn )
{
   // Handle completion of a write operation.
   // Nothing to do. The socket will be closed automatically when the last
   // reference to the connection object goes away.
   if (serverEnv_.debug())
      cout << "   Server::handle_write: client request " << inbound_request_ << " replying with  " << outbound_response_ << endl;

   if (e) {
      ecf::LogToCout logToCout;
      std::stringstream ss; ss << "Server::handle_write: " << e.message() << " : for request " << inbound_request_;
      log(Log::ERR,ss.str());
      return;
   }

   (void)shutdown_socket(conn,"Server::handle_write:");

   // If asked to terminate we do it here rather than in handle_read.
   // So that we have responded to the client.
   // *HOWEVER* only do this if the request was successful.
   //           we do this by checking that the out bound response was ok
   //           i.e a read only user should not be allowed to terminate server.
   if (inbound_request_.terminateRequest() && outbound_response_.get_cmd()->isOkCmd()) {
      if (serverEnv_.debug()) cout << "   <--Server::handle_write exiting server via terminate() port " << serverEnv_.port() << endl;
      terminate();
   }
}

bool Server::shutdown_socket(connection_ptr conn, const std::string& msg) const
{
   // For portable behaviour with respect to graceful closure of a connected socket,
   // call shutdown() before closing the socket.
   //
   //    conn->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both)
   // This *CAN* throw an error if the client side socket is not connected. client may have been killed *OR* timed out
   // i.e "shutdown: Transport endpoint is not connected"
   //
   // Since this can happen, instead of throwing, we use non-throwing version & just report it
   boost::system::error_code ec;
   conn->socket_ll().shutdown(boost::asio::ip::tcp::socket::shutdown_both,ec);
   if (ec) {
      ecf::LogToCout logToCout;
      std::stringstream ss; ss << msg << " socket shutdown both failed: " << ec.message() << " : for request " << inbound_request_;
      log(Log::ERR,ss.str());
      return false;
   }
   return true;
}


void Server::terminate()
{
   // The server is terminated by cancelling all outstanding asynchronous
   // operations. Once all operations have finished the io_service::run() call  will exit.
   if (serverEnv_.debug()) cout << "   Server::terminate(): posting call to Server::handle_terminate" << endl;

   // Post a call to the stop function so that Server::stop() is safe to call from any thread.
   io_service_.post(boost::bind(&Server::handle_terminate, this));
}

void Server::handle_terminate()
{
   if (serverEnv_.debug()) cout << boost::this_thread::get_id() << "   Server::handle_terminate() : cancelling checkpt and traverser timers, and signals" << endl;

   // Cancel signal
   signals_.clear();
   signals_.cancel();

   // Cancel async timers for check pointing and traversal
   traverser_.terminate();
   checkPtSaver_.terminate();

   acceptor_.close();

   // Stop the io_service object's event processing loop. Will cause run to return immediately
   io_service_.stop();
}

// ============================== other privates ===========================================

bool Server::load_check_pt_file_on_startup()
{
   // On start up we want different behaviour.
   // If check pt file exists and we can't load then we want to exit
   // This avoids the server from overwriting the check point file
   // Which may be from a different version. let the user handle it.
   LogToCout logToCout;
   bool checkpt_failed = false;
   if (restore_from_checkpt(serverEnv_.checkPtFilename(),checkpt_failed)) {
      return true;
   }
   bool backup_checkpt_failed = false;
   if (restore_from_checkpt(serverEnv_.oldCheckPtFilename(),backup_checkpt_failed)) {
      return true;
   }

   if (backup_checkpt_failed && !fs::exists(serverEnv_.checkPtFilename())) {
      throw std::runtime_error("Can not start server, please handle the backup checkpoint file first");
   }

   if (checkpt_failed && !fs::exists(serverEnv_.oldCheckPtFilename())) {
      throw std::runtime_error("Can not start server, please handle the checkpoint file first");
   }
   return false;
}

void Server::loadCheckPtFile()
{
   // if the check point file starts with an absolute file load that first
   // otherwise check in ECF_HOME then load it. Repeat for back up check point file
   // The server environment has already asserted that we can *NOT* have an empty check point file
   bool ignore;
   if (restore_from_checkpt(serverEnv_.checkPtFilename(),ignore)) {
      return;
   }

   if (restore_from_checkpt(serverEnv_.oldCheckPtFilename(),ignore)) {
      return;
   }
}

bool Server::restore_from_checkpt(const std::string& filename,bool& failed)
{
   // cout << "Server::restore_from_checkpt " <<  filename;
   failed = false;
   if (fs::exists(filename)) {
      // cout << " file exists\n";
      LOG(Log::MSG, "Loading check point file " << filename << " port = " << serverEnv_.port());

      try {
         defs_->restore_from_checkpt(filename);   // this can throw
         update_defs_server_state();              // works on def_
         //cout << "Server::restore_from_checkpt SUCCEDED found " << defs_->suiteVec().size() << " suites\n";
         return true;
      }
      catch (exception& e) {
         LOG(Log::ERR, "Failed to load check point file " << filename << ", because: " << e.what());
         failed = true;
      }
   }
//   else {
//      cout << " does *not* exist\n";
//   }
   return false;
}

void Server::update_defs_server_state()
{
   /// The Job submission interval, and host port are not persisted, on the DEFS
   /// Hence when restoring from a checkpoint file, Be sure to update server state

   // Do any *one* time setup on the defs

   // Set the server environment *ON* the defs, so that generate variables can be created.
   // gets the environment as read in by the server, and make available for defs
   // ECF_HOME .
   // ECF_CHECK ecf.check
   // ECF_CHECKOLD ecf.check.b
   std::vector<std::pair<std::string,std::string> > envVec;
   serverEnv_.variables(envVec);
   defs_->set_server().add_or_update_server_variables(envVec);

   defs_->set_server().hostPort(  hostPort() );
   defs_->set_server().set_state( serverState_ );

   // let the defs store the job submission interval, & whether we want job generation.testing can disable this
   defs_->set_server().jobSubmissionInterval(  serverEnv_.submitJobsInterval() );
   defs_->set_server().jobGeneration( serverEnv_.jobGeneration() );
   LOG_ASSERT( defs_->server().jobSubmissionInterval() != 0 ,"");

   /// System needs defs to handle process that have died, and need to flagged as aborted
   ecf::System::instance()->setDefs(defs_);
}

void Server::set_server_state(SState::State ss)
{
   serverState_ = ss;
   stats().status_ = static_cast<int>(serverState_);
   defs_->set_server().set_state( serverState_ );
}


/// ======================================================================================
/// AbstractServer function.
/// ======================================================================================

std::pair<std::string,std::string> Server::hostPort() const
{
   return serverEnv_.hostPort();
}

void Server::updateDefs( defs_ptr defs, bool force)
{
   if (serverEnv_.debug()) std::cout << "   Server::updateDefs: Loading new suites" << endl;

   // After the absorb, input defs will be left with NO suites.
   defs_->absorb(defs.get(),force);

   defs_->set_most_significant_state();
   LOG_ASSERT( defs_->server().jobSubmissionInterval() != 0 ,"");
}

void Server::clear_defs()
{
   if (serverEnv_.debug()) cout << "   Server::clear_defs()" << endl;

   defs_->clear();
}

void Server::checkPtDefs(ecf::CheckPt::Mode m, int check_pt_interval, int check_pt_save_time_alarm)
{
   if (serverEnv_.debug())
      cout << "   Server::checkPtDefs() mode(" << m << ") check_pt_interval(" << check_pt_interval << ") check_pt_save_time_alarm(" << check_pt_save_time_alarm << ")" << endl;

   if (m == ecf::CheckPt::UNDEFINED && check_pt_interval == 0 && check_pt_save_time_alarm == 0) {
      checkPtSaver_.explicitSave();  // will always save
   }
   else {
      if ( m != ecf::CheckPt::UNDEFINED ) {
         serverEnv_.set_check_mode( m );
         stats().checkpt_mode_ = serverEnv_.checkMode();
      }
      if ( check_pt_interval > 0) {
         serverEnv_.set_checkpt_interval( check_pt_interval );
         stats().checkpt_interval_ = check_pt_interval;
      }
      if (check_pt_save_time_alarm > 0 ) {
         serverEnv_.set_checkpt_save_time_alarm( check_pt_save_time_alarm );
         stats().checkpt_save_time_alarm_ = check_pt_save_time_alarm;
      }
   }
}

void Server::restore_defs_from_checkpt()
{
   if (serverEnv_.debug()) cout << "   Server::restore_defs_from_checkpt()" << endl;

   if (serverState_ != SState::HALTED ) {
      throw std::runtime_error( "Can not restore from checkpt the server must be halted first");
   }

   if (!defs_->suiteVec().empty()) {
      // suites must be deleted manually first
      throw std::runtime_error( "Can not restore from checkpt the server suites must be deleted first");
   }

   loadCheckPtFile();
}

void Server::nodeTreeStateChanged()
{
   if (serverEnv_.debug()) cout << "   Server::nodeTreeStateChanged()" << endl;

   // will only actually save if configuration allows it
   checkPtSaver_.saveIfAllowed();
}

bool Server::allowTaskCommunication() const
{
   return (serverState_ != SState::HALTED) ? true : false;
}


void Server::shutdown()
{
   ///           User Request    Task Request   Job Scheduling   Check-pointing
   /// RUNNING      yes               yes              yes            yes
   /// SHUTDOWN     yes               yes              no             yes
   /// HALTED       yes               no               no             no
   if (serverEnv_.debug()) cout << "   Server::shutdown. Stop Scheduling new jobs only" << endl;

   // Stop server from creating new jobs. Don't stop the checkPtSaver_ since
   // the jobs communication with server can still change state. Which we want
   // to check point.
   traverser_.stop();

   // Continue check pointing since, we allow tasks communication. This can change node
   // tree state. Which we *must* be able to checkpoint.
   // If we go from HALTED --> SHUTDOWN, then check pointing  needs to be enabled
   checkPtSaver_.start();

   // Will update defs as well to stop job scheduling
   set_server_state(SState::SHUTDOWN);
}

void Server::halted()
{
   ///           User Request    Task Request   Job Scheduling   Check-pointing
   /// RUNNING      yes               yes              yes            yes
   /// SHUTDOWN     yes               yes              no             yes
   /// HALTED       yes               no               no             no
   if (serverEnv_.debug()) cout << "   Server::halted. Stop Scheduling new jobs *and* block task communication. Stop check pointing. Only accept user request" << endl;

   // Stop server from creating new jobs. i.e Job scheduling.
   traverser_.stop();

   // *** CRITICAL*** when the server is halted, we ***MUST NOT*** do any further check pointing
   // In a typical operational scenario where we have a home, and backup servers.
   // The checkpoint file is copied to the backup servers periodically (via a task)
   // hence we want to preserve the state of the last checkpoint.
   // Added after discussion with Axel.
   checkPtSaver_.stop();

   // Stop the task communication with server. Hence nodes can be stuck
   // in submitted/active states. Task based command will continue attempting,
   // communication with the server for up to 24hrs.
   // Will update defs as well to stop job scheduling
   set_server_state(SState::HALTED);
}

void Server::restart()
{
   ///           User Request    Task Request   Job Scheduling   Check-pointing
   /// RUNNING      yes               yes              yes            yes
   /// SHUTDOWN     yes               yes              no             yes
   /// HALTED       yes               no               no             no
   if (serverEnv_.debug()) std::cout << "   Server::restart" << endl;

   // The server state *MUST* be set, *before* traverser_.start(), since that can kick off job traversal.
   // Job Scheduling can only be done under RUNNING state, hence must be before traverser_.start();
   //
   // If we placed set_server_state(SState::RUNNING); after we can miss time slots
   //    See:: SUP 571- Time dependency after halt/checkpoint
   set_server_state(SState::RUNNING);

   traverser_.start();
   checkPtSaver_.start();
}

void Server::traverse_node_tree_and_job_generate(const boost::posix_time::ptime& time_now,bool user_cmd_context ) const
{
   traverser_.traverse_node_tree_and_job_generate(time_now, user_cmd_context);
}

bool Server::reloadWhiteListFile(std::string& errorMsg)
{
   if (serverEnv_.debug()) cout << "   Server::reloadWhiteListFile" << endl;

   return serverEnv_.reloadWhiteListFile(errorMsg);
}

bool Server::authenticateReadAccess(const std::string& user)
{
   return serverEnv_.authenticateReadAccess(user);
}
bool Server::authenticateReadAccess(const std::string& user, const std::string& path)
{
   return serverEnv_.authenticateReadAccess(user,path);
}
bool Server::authenticateReadAccess(const std::string& user, const std::vector<std::string>& paths)
{
   return serverEnv_.authenticateReadAccess(user,paths);
}

bool Server::authenticateWriteAccess(const std::string& user )
{
   return serverEnv_.authenticateWriteAccess(user);
}
bool Server::authenticateWriteAccess(const std::string& user, const std::string& path)
{
   return serverEnv_.authenticateWriteAccess(user,path);
}
bool Server::authenticateWriteAccess(const std::string& user, const std::vector<std::string>& paths)
{
   return serverEnv_.authenticateWriteAccess(user,paths);
}

bool Server::lock(const std::string& user)
{
   if (serverEnv_.debug()) std::cout << "   Server::lock " << user << endl;

   if (userWhoHasLock_.empty()) {
      userWhoHasLock_ = user;
      stats().locked_by_user_ = user;
      shutdown();
      return true;
   }
   else if ( userWhoHasLock_ == user && serverState_ == SState::SHUTDOWN ) {
      // Same user attempting multiple locks
      return true;
   }
   return false;
}
void Server::unlock()
{
   if (serverEnv_.debug()) std::cout << "   Server::unlock " << userWhoHasLock_ << endl;

   userWhoHasLock_.clear();
   stats().locked_by_user_.clear();
   if ( serverState_ == SState::SHUTDOWN ) restart();
}
const std::string& Server::lockedUser() const
{
   return userWhoHasLock_;
}


int Server::poll_interval() const
{
   return serverEnv_.submitJobsInterval();
}

void Server::debug_server_on()
{
   serverEnv_.set_debug(true);
   std::cout << "\nEnable DEBUG, start with DUMP of server environment:\n\n";
   std::cout << serverEnv_.dump() << endl;
}

void Server::debug_server_off()
{
   serverEnv_.set_debug(false);
}

bool Server::debug() const
{
   return serverEnv_.debug();
}

void Server::sigterm_signal_handler()
{
   if (io_service_.stopped()) {
      if (serverEnv_.debug()) cout << "-->Server::sigterm_signal_handler(): io_service is stopped returning " << endl;
      return;
   }

   if (serverEnv_.debug()) cout << "Server::sigterm_signal_handler(): Received SIGTERM : starting check pointing" << endl;
   ecf::log(Log::MSG,"Server::sigterm_signal_handler(): Received SIGTERM : starting check pointing");

   checkPtDefs();

   ecf::log(Log::MSG,"Server::sigterm_signal_handler(): finished check pointing");
   if (serverEnv_.debug()) cout << "Server::sigterm_signal_handler(): finished check pointing" << endl;

   // We need re-wait each time signal handler is called
   signals_.async_wait(boost::bind(&Server::sigterm_signal_handler, this));
}
