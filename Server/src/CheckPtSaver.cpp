//============================================================================
// Name        : CheckPtSaver.cpp
// Author      : Avi
// Revision    : $Revision: #43 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <fstream>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/bind.hpp"

#include "CheckPtSaver.hpp"
#include "Server.hpp"
#include "ServerEnvironment.hpp"
#include "Defs.hpp"
#include "Log.hpp"
#include "DurationTimer.hpp"
#include "CtsApi.hpp"
#include "Str.hpp"
#include "Ecf.hpp"
#include "File.hpp"

//#define DEBUG_CHECKPT 1
//#define DEBUG_CHECKPT_SAVE_ALLOWED 1

#ifdef DEBUG_CHECKPT
#include <boost/date_time/posix_time/time_formatters.hpp>  // requires boost date and time lib, for to_simple_string
#endif

namespace fs = boost::filesystem;
using namespace ecf;


//-------------------------------------------------------------------------------------
CheckPtSaver::CheckPtSaver(
         Server* s,
         boost::asio::io_service& io,
         const ServerEnvironment* serverEnv )
: server_( s ),
  timer_( io, boost::posix_time::seconds( 0 ) ),
  firstTime_(true),
  running_(false),
  serverEnv_(serverEnv),
  state_change_no_(Ecf::state_change_no()),
  modify_change_no_(Ecf::modify_change_no())
{
#ifdef DEBUG_CHECKPT
	std::cout << "      CheckPtSaver::CheckPtSaver period = " << serverEnv_->checkPtInterval() << "\n";
#endif
}

CheckPtSaver::~CheckPtSaver() {
#ifdef DEBUG_CHECKPT
	std::cout << "      ~CheckPtSaver::CheckPtSaver\n";
#endif
}

void CheckPtSaver::start()
{
#ifdef DEBUG_CHECKPT
	std::cout << "      CheckPtSaver::start() check_mode: "
	          << serverEnv_->check_mode_str() << " interval = " << serverEnv_->checkPtInterval()
	          << " time = " << to_simple_string(boost::posix_time::second_clock::universal_time()) << "\n";
#endif

	// Only save check pt periodically if configuration allows it
	if (serverEnv_->checkMode() != ecf::CheckPt::ON_TIME ) {
#ifdef DEBUG_CHECKPT
		std::cout << "      CheckPtSaver::start() aborted as configuration does not allow it" << std::endl;
#endif
		return;
	}

	running_ = true;

	// * important * The time should only be started *ONCE*. Otherwise we will end up with
	//               with explicit save each time server is halted/started.
	if (firstTime_) {
	   firstTime_ = false;
	   timer_.expires_from_now(  boost::posix_time::seconds( serverEnv_->checkPtInterval() ) );
#ifdef ECFLOW_MT
	   timer_.async_wait( server_->strand_.wrap( boost::bind( &CheckPtSaver::periodicSaveCheckPt,this,boost::asio::placeholders::error ) ) );
#else
	   timer_.async_wait( server_->io_service_.wrap( boost::bind( &CheckPtSaver::periodicSaveCheckPt,this,boost::asio::placeholders::error ) ) );
#endif
	}
}

void CheckPtSaver::stop()
{ // The server is stopped by cancelling all outstanding asynchronous
   // operations. Once all operations have finished the io_service::run() call
   // will exit.
#ifdef DEBUG_CHECKPT
	std::cout << "      CheckPtSaver::stop() check_mode: " << serverEnv_->check_mode_str() <<  " interval = " << serverEnv_->checkPtInterval() << "\n";
#endif
 	running_ = false;
}

void CheckPtSaver::terminate()
{
   timer_.cancel();
}


void CheckPtSaver::explicitSave(bool from_server) const
{
	if ( server_->defs_ ) {

		try {
#ifdef DEBUG_CHECKPT
			std::cout << "      CheckPtSaver::explicitSave() Saving checkpt file " << serverEnv_->checkPtFilename() << "\n";
#endif
			// Time how long we take to checkpt, Help to recognise *SLOW* disk, which can *AFFECT* server performance
			DurationTimer durationTimer;

			// Backup checkpoint file if it exists & is non zero
			// Avoid an empty file as a backup file, could results from a full file system
			// i.e move ecf_checkpt_file --> ecf_backup_checkpt_file
			fs::path checkPtFile(serverEnv_->checkPtFilename());
			if (fs::exists(checkPtFile) && fs::file_size(checkPtFile) != 0) {

				fs::path oldCheckPtFile(serverEnv_->oldCheckPtFilename());
				fs::remove(oldCheckPtFile);
				fs::rename( checkPtFile, oldCheckPtFile );
			}

			// write to ecf_checkpt_file, if file system is full this could result in an empty file. ?
			//
			// To optimise check pointing, we minimise system calls, i.e we can write check point as a string,
			// and save string to a file with a single write. This is faster than calling:
			//    server_->defs_->save_as_checkpt( serverEnv_->checkPtFilename() );
			// This solution however does require *MORE* memory.
			std::string checkpt_as_string,error_msg;
			server_->defs_->save_checkpt_as_string(checkpt_as_string);
			if (!File::create(serverEnv_->checkPtFilename(),checkpt_as_string,error_msg)) {
            throw std::runtime_error(error_msg);
			}

			state_change_no_ = Ecf::state_change_no();    // For periodic update only save checkPt if it has changed
			modify_change_no_ = Ecf::modify_change_no();  // For periodic update only save checkPt if it has changed


         if (from_server) {
            // Create new time stamp otherwise we end up using the time stamp from the last command
            if (Log::instance()) Log::instance()->cache_time_stamp();
            std::string msg = Str::SVR_CMD(); msg += CtsApi::checkPtDefsArg();
            std::stringstream ss; ss << msg << " in " << durationTimer.duration() << " seconds";
            log(Log::MSG,ss.str());
         }

         /// If Save take longer than checkpt_save_time_alarm, then set a flag on server
         /// So that user can be aware of it.
         if (static_cast<size_t>(durationTimer.duration()) > server_->serverEnv_.checkpt_save_time_alarm() ) {
            server_->defs_->flag().set(ecf::Flag::LATE);
            std::stringstream ss;
            ss << "Check pt save time(" << durationTimer.duration() << ") is greater than alarm time("
               << server_->serverEnv_.checkpt_save_time_alarm() << "). Excessive save times can interfere with scheduling!";
            log(Log::WAR,ss.str());
         }
#ifdef DEBUG_CHECKPT
			std::cout << " backup and save took " <<  durationTimer.duration() << " seconds\n";
#endif
		}
		catch (std::exception& e) {
	 		LOG(Log::ERR,"Could not save checkPoint file! " << e.what() << " File system full?");
	 	}
	}
	else {
#ifdef DEBUG_CHECKPT
		std::cout << "      CheckPtSaver::explicitSave() Node tree not loaded, can not save check pt file\n";
#endif
	}
}

void CheckPtSaver::periodicSaveCheckPt(const boost::system::error_code& error )
{
#ifdef DEBUG_CHECKPT
      std::cout << "      CheckPtSaver::periodicSaveCheckPt() interval = " << serverEnv_->checkPtInterval() << "  time: " << to_simple_string(boost::posix_time::second_clock::universal_time()) << "\n";
#endif
   if (error == boost::asio::error::operation_aborted) {
#ifdef DEBUG_CHECKPT
   std::cout << "      CheckPtSaver::periodicSaveCheckPt : boost::asio::error::operation_aborted : time cancelled: Node running(" << running_ << ")  interval = " << serverEnv_->checkPtInterval() << "\n";
#endif
      return;
   }
   else if (error) {
      LogToCout toCoutAsWell;
      LOG(Log::ERR, "CheckPtSaver::periodicSaveCheckPt "  <<  error.message());
      return;
   }

	if (running_) {
	   // state changed
	   if (state_change_no_  != Ecf::state_change_no() || modify_change_no_ != Ecf::modify_change_no()) {
	      doSave();
	   }
	}

 	/// Appears that expires_from_now is more accurate then expires_at
	timer_.expires_from_now(  boost::posix_time::seconds( serverEnv_->checkPtInterval() ) );
#ifdef ECFLOW_MT
   timer_.async_wait( server_->strand_.wrap( boost::bind( &CheckPtSaver::periodicSaveCheckPt,this,boost::asio::placeholders::error ) ) );
#else
   timer_.async_wait( server_->io_service_.wrap( boost::bind( &CheckPtSaver::periodicSaveCheckPt,this,boost::asio::placeholders::error ) ) );
#endif
}


void CheckPtSaver::doSave() const
{
#ifdef DEBUG_CHECKPT
		std::cout << "      CheckPtSaver::doSave()\n";
#endif
	// Check to see if configuration allows save
	if (serverEnv_->checkMode() == ecf::CheckPt::NEVER ) {
#ifdef DEBUG_CHECKPT
		std::cout << "     CheckPtSaver::doSave() configuration does not allow save \n";
#endif
		return;
	}
	explicitSave(true/* from the server, hence log */);
}

void CheckPtSaver::saveIfAllowed()
{
	// Call on each state change. Hence will get many times
#ifdef DEBUG_CHECKPT_SAVE_ALLOWED
	std::cout << "      CheckPtSaver::saveIfAllowed()\n";
#endif

	// Check to see if configuration allows immediate save.
	if (serverEnv_->checkMode() == ecf::CheckPt::ALWAYS ) {
		doSave();
 	}
}
