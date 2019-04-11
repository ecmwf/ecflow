//============================================================================
// Name        : BaseServer.cpp
// Author      : Avi
// Revision    : $Revision: #173 $
//
// Copyright 2009-2019 ECMWF.
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

#include <iostream>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "BaseServer.hpp"  // Must come before boost/serialization headers.
#include "Defs.hpp"
#include "Log.hpp"
#include "System.hpp"
#include "ServerEnvironment.hpp"
#include "Ecf.hpp"
#include "Calendar.hpp"
#include "Version.hpp"
#include "Str.hpp"
#include "File.hpp"
#include "ExprDuplicate.hpp"

using boost::asio::ip::tcp;
namespace fs = boost::filesystem;

using namespace std;
using namespace ecf;


/// Constructor opens the acceptor and starts waiting for the first incoming connection.
BaseServer::BaseServer( ServerEnvironment& serverEnv ) :
   io_service_(),
   signals_(io_service_),
   acceptor_(io_service_),
   defs_(Defs::create()),      // ECFLOW-182
   traverser_   (this,  io_service_, serverEnv ),
   checkPtSaver_(this,  io_service_, &serverEnv ),
   serverState_(SState::HALTED),
   serverEnv_(serverEnv)
{
   if (serverEnv_.debug()) cout << "-->Server::server starting server on port "
                                << serverEnv.port()
                                << endl;
   LogFlusher logFlusher;

   // Register to handle the signals.
   // Support for emergency check pointing during system session.
   signals_.add(SIGTERM);
   signals_.async_wait([this](boost::system::error_code /*ec*/, int /*signo*/) { sigterm_signal_handler();});


   // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
   boost::asio::ip::tcp::endpoint endpoint(serverEnv.tcp_protocol(), serverEnv.port());
   acceptor_.open(endpoint.protocol());
   acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
   acceptor_.bind(endpoint);
   acceptor_.listen();   // address is use error, when it comes, bombs out here

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
}

BaseServer::~BaseServer()
{
   if (serverEnv_.debug()) cout << "<--BaseServer::~Baseserver exiting server on port " << serverEnv_.port() << endl;

   // Defs destructor may get called after we have returned from main. See ECFLOW-1291
   // In this case the static memory used in ExprDuplicate will not be reclaimed. hence do it here:
   // Duplicate AST are held in a static map. Delete them, to avoid valgrind and ASAN from complaining
   ExprDuplicate reclaim_cloned_ast_memory;

   defs_.reset();

#ifdef DEBUG
   if ( defs_.use_count() != 0) {
      cout << "Server::~server() defs_.use_count() = " << defs_.use_count() << " something is still hold onto the defs, asserting\n";
   }
#endif
   assert(defs_.use_count() == 0);
}

void BaseServer::run()
{
  // The io_service::run() call will block until all asynchronous operations
  // have finished. While the server is running, there is always at least one
  // asynchronous operation outstanding: the asynchronous accept call waiting
  // for new incoming connections.
  io_service_.run();
}

void BaseServer::terminate()
{
   // The server is terminated by cancelling all outstanding asynchronous
   // operations. Once all operations have finished the io_service::run() call  will exit.
   if (serverEnv_.debug()) cout << "   Server::terminate(): posting call to Server::handle_terminate" << endl;

   // Post a call to the stop function so that Server::stop() is safe to call from any thread.
   io_service_.post( [this]() {handle_terminate();} ); // boost::bind(&Server::handle_terminate, this));
}

void BaseServer::handle_terminate()
{
   // if (serverEnv_.debug()) cout << boost::this_thread::get_id() << "   Server::handle_terminate() : cancelling checkpt and traverser timers, and signals" << endl;
   if (serverEnv_.debug()) cout << "   Server::handle_terminate() : cancelling checkpt and traverser timers, and signals" << endl;

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

bool BaseServer::load_check_pt_file_on_startup()
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

void BaseServer::loadCheckPtFile()
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

bool BaseServer::restore_from_checkpt(const std::string& filename,bool& failed)
{
   // cout << "BaseServer::restore_from_checkpt " << filename;
   if (fs::exists(filename)) {
      // cout << " file exists\n";

      std::string s = "Loading check point file "; s += filename; s += " port:"; s+= serverEnv_.the_port();
      log(Log::MSG,s);

      try {
         defs_->restore(filename);                      // this can throw
         update_defs_server_state();                    // works on def_
         LOG(Log::MSG, "Loading of *DEFS* check point file SUCCEDED. Loaded "<< defs_->suiteVec().size() << " suites");

         // fast forward any time attributes, if suite clock is *ONLY* 1 hour behind real time
         // This may remove autocancelled nodes, may autoarchive, may set late attribute,
         // may expire time based attributes which will start jobs when server starts
         if (defs_->catch_up_to_real_time()) {
            log(Log::MSG, "Suite time attributes updated to catch up with real time. Down time was less than 1 hour");
         }
         return true;
      }
      catch (exception& e) {
         LOG(Log::ERR, "Failed to load *DEFS* check point file " << filename << ", because: " << e.what());
      }
   }
//   else {
//      cout << " does *not* exist\n";
//   }
   return false;
}

void BaseServer::update_defs_server_state()
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

   defs_->set_server().hostPort(  serverEnv_.hostPort()  );
   defs_->set_server().set_state( serverState_ );

   // let the defs store the job submission interval, & whether we want job generation.testing can disable this
   defs_->set_server().jobSubmissionInterval(  serverEnv_.submitJobsInterval() );
   defs_->set_server().jobGeneration( serverEnv_.jobGeneration() );
   LOG_ASSERT( defs_->server().jobSubmissionInterval() != 0 ,"");

   // Since we have reloaded Defs, make sure clients, re-sync by resetting change and modify numbers on server
   defs_->set_state_change_no(Ecf::state_change_no());
   defs_->set_modify_change_no(Ecf::modify_change_no());

   /// System needs defs to handle process that have died, and need to flagged as aborted
   ecf::System::instance()->setDefs(defs_);
}

void BaseServer::set_server_state(SState::State ss)
{
   serverState_ = ss;
   stats().status_ = static_cast<int>(serverState_);
   defs_->set_server().set_state( serverState_ );
}


/// ======================================================================================
/// AbstractServer function.
/// ======================================================================================

std::pair<std::string,std::string> BaseServer::hostPort() const
{
   return serverEnv_.hostPort();
}

void BaseServer::updateDefs( defs_ptr defs, bool force)
{
   if (serverEnv_.debug()) std::cout << "   BaseServer::updateDefs: Loading new suites" << endl;

   // After the absorb, input defs will be left with NO suites.
   defs_->absorb(defs.get(),force);

   defs_->set_most_significant_state();
   LOG_ASSERT( defs_->server().jobSubmissionInterval() != 0 ,"");
}

void BaseServer::clear_defs()
{
   if (serverEnv_.debug()) cout << "   BaseServer::clear_defs()" << endl;

   defs_->clear();
}

void BaseServer::checkPtDefs(ecf::CheckPt::Mode m, int check_pt_interval, int check_pt_save_time_alarm)
{
   if (serverEnv_.debug())
      cout << "   BaseServer::checkPtDefs() mode(" << m << ") check_pt_interval(" << check_pt_interval << ") check_pt_save_time_alarm(" << check_pt_save_time_alarm << ")" << endl;

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

void BaseServer::restore_defs_from_checkpt()
{
   if (serverEnv_.debug()) cout << "   BaseServer::restore_defs_from_checkpt()" << endl;

   if (serverState_ != SState::HALTED ) {
      throw std::runtime_error( "Can not restore from checkpt the server must be halted first");
   }

   if (!defs_->suiteVec().empty()) {
      // suites must be deleted manually first
      throw std::runtime_error( "Can not restore from checkpt the server suites must be deleted first");
   }

   loadCheckPtFile();
}

void BaseServer::nodeTreeStateChanged()
{
   if (serverEnv_.debug()) cout << "   BaseServer::nodeTreeStateChanged()" << endl;

   // will only actually save if configuration allows it
   checkPtSaver_.saveIfAllowed();
}

bool BaseServer::allowTaskCommunication() const
{
   return (serverState_ != SState::HALTED) ? true : false;
}


void BaseServer::shutdown()
{
   ///           User Request    Task Request   Job Scheduling   Check-pointing
   /// RUNNING      yes               yes              yes            yes
   /// SHUTDOWN     yes               yes              no             yes
   /// HALTED       yes               no               no             no
   if (serverEnv_.debug()) cout << "   BaseServer::shutdown. Stop Scheduling new jobs only" << endl;

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

void BaseServer::halted()
{
   ///           User Request    Task Request   Job Scheduling   Check-pointing
   /// RUNNING      yes               yes              yes            yes
   /// SHUTDOWN     yes               yes              no             yes
   /// HALTED       yes               no               no             no
   if (serverEnv_.debug()) cout << "   BaseServer::halted. Stop Scheduling new jobs *and* block task communication. Stop check pointing. Only accept user request" << endl;

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

void BaseServer::restart()
{
   ///           User Request    Task Request   Job Scheduling   Check-pointing
   /// RUNNING      yes               yes              yes            yes
   /// SHUTDOWN     yes               yes              no             yes
   /// HALTED       yes               no               no             no
   if (serverEnv_.debug()) std::cout << "   BaseServer::restart" << endl;

   // The server state *MUST* be set, *before* traverser_.start(), since that can kick off job traversal.
   // Job Scheduling can only be done under RUNNING state, hence must be before traverser_.start();
   //
   // If we placed set_server_state(SState::RUNNING); after we can miss time slots
   //    See:: SUP 571- Time dependency after halt/checkpoint
   set_server_state(SState::RUNNING);

   traverser_.start();
   checkPtSaver_.start();
}

void BaseServer::traverse_node_tree_and_job_generate(const boost::posix_time::ptime& time_now,bool user_cmd_context ) const
{
   traverser_.traverse_node_tree_and_job_generate(time_now, user_cmd_context);
}

bool BaseServer::reloadWhiteListFile(std::string& errorMsg)
{
   if (serverEnv_.debug()) cout << "   BaseServer::reloadWhiteListFile" << endl;
   return serverEnv_.reloadWhiteListFile(errorMsg);
}

bool BaseServer::reloadPasswdFile(std::string& errorMsg)
{
   if (serverEnv_.debug()) cout << "   BaseServer::reloadPasswdFile" << endl;
   return serverEnv_.reloadPasswdFile(errorMsg);
}

bool BaseServer::authenticateReadAccess(const std::string& user,const std::string& passwd)
{
   return serverEnv_.authenticateReadAccess(user,passwd);
}
bool BaseServer::authenticateReadAccess(const std::string& user,const std::string& passwd, const std::string& path)
{
   return serverEnv_.authenticateReadAccess(user,passwd,path);
}
bool BaseServer::authenticateReadAccess(const std::string& user,const std::string& passwd, const std::vector<std::string>& paths)
{
   return serverEnv_.authenticateReadAccess(user,passwd,paths);
}

bool BaseServer::authenticateWriteAccess(const std::string& user )
{
   return serverEnv_.authenticateWriteAccess(user);
}
bool BaseServer::authenticateWriteAccess(const std::string& user, const std::string& path)
{
   return serverEnv_.authenticateWriteAccess(user,path);
}
bool BaseServer::authenticateWriteAccess(const std::string& user, const std::vector<std::string>& paths)
{
   return serverEnv_.authenticateWriteAccess(user,paths);
}

bool BaseServer::lock(const std::string& user)
{
   if (serverEnv_.debug()) std::cout << "   BaseServer::lock " << user << endl;

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
void BaseServer::unlock()
{
   if (serverEnv_.debug()) std::cout << "   BaseServer::unlock " << userWhoHasLock_ << endl;

   userWhoHasLock_.clear();
   stats().locked_by_user_.clear();
   if ( serverState_ == SState::SHUTDOWN ) restart();
}
const std::string& BaseServer::lockedUser() const
{
   return userWhoHasLock_;
}


int BaseServer::poll_interval() const
{
   return serverEnv_.submitJobsInterval();
}

void BaseServer::debug_server_on()
{
   serverEnv_.set_debug(true);
   std::cout << "\nEnable DEBUG, start with DUMP of server environment:\n\n";
   std::cout << serverEnv_.dump() << endl;
}

void BaseServer::debug_server_off()
{
   serverEnv_.set_debug(false);
}

bool BaseServer::debug() const
{
   return serverEnv_.debug();
}

void BaseServer::sigterm_signal_handler()
{
   if (io_service_.stopped()) {
      if (serverEnv_.debug()) cout << "-->BaseServer::sigterm_signal_handler(): io_service is stopped returning " << endl;
      return;
   }

   if (serverEnv_.debug()) cout << "BaseServer::sigterm_signal_handler(): Received SIGTERM : starting check pointing" << endl;
   ecf::log(Log::MSG,"BaseServer::sigterm_signal_handler(): Received SIGTERM : starting check pointing");

   defs_->flag().set(ecf::Flag::ECF_SIGTERM);
   checkPtDefs();

   ecf::log(Log::MSG,"BaseServer::sigterm_signal_handler(): finished check pointing");
   if (serverEnv_.debug()) cout << "BaseServer::sigterm_signal_handler(): finished check pointing" << endl;

   // We need re-wait each time signal handler is called
   signals_.async_wait([this](boost::system::error_code /*ec*/, int /*signo*/) { sigterm_signal_handler();});
}
