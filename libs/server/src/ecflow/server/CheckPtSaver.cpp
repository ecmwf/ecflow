/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/server/CheckPtSaver.hpp"

#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/Timer.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/server/BaseServer.hpp"
#include "ecflow/server/ServerEnvironment.hpp"

// #define DEBUG_CHECKPT 1
// #define DEBUG_CHECKPT_SAVE_ALLOWED 1

#ifdef DEBUG_CHECKPT
    #include <boost/date_time/posix_time/time_formatters.hpp> // requires boost date and time lib, for to_simple_string
#endif

using namespace ecf;

//-------------------------------------------------------------------------------------
CheckPtSaver::CheckPtSaver(BaseServer* s, boost::asio::io_context& io, const ServerEnvironment* serverEnv)
    : server_(s),
      timer_(io, boost::posix_time::seconds(0)),
      firstTime_(true),
      running_(false),
      serverEnv_(serverEnv),
      state_change_no_(Ecf::state_change_no()),
      modify_change_no_(Ecf::modify_change_no()) {
#ifdef DEBUG_CHECKPT
    std::cout << "      CheckPtSaver::CheckPtSaver period = " << serverEnv_->checkPtInterval() << "\n";
#endif
}

CheckPtSaver::~CheckPtSaver() {
#ifdef DEBUG_CHECKPT
    std::cout << "      ~CheckPtSaver::CheckPtSaver\n";
#endif
}

void CheckPtSaver::start() {
#ifdef DEBUG_CHECKPT
    std::cout << "      CheckPtSaver::start() check_mode: " << serverEnv_->check_mode_str()
              << " interval = " << serverEnv_->checkPtInterval()
              << " time = " << to_simple_string(boost::posix_time::second_clock::universal_time()) << "\n";
#endif

    // Only save check pt periodically if configuration allows it
    if (serverEnv_->checkMode() != ecf::CheckPt::ON_TIME) {
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
        timer_.expires_from_now(boost::posix_time::seconds(serverEnv_->checkPtInterval()));
        timer_.async_wait(boost::asio::bind_executor(
            server_->io_, [this](const boost::system::error_code& error) { periodicSaveCheckPt(error); }));
    }
}

void CheckPtSaver::stop() { // The server is stopped by cancelling all outstanding asynchronous
                            // operations. Once all operations have finished the io_context::run() call
                            // will exit.
#ifdef DEBUG_CHECKPT
    std::cout << "      CheckPtSaver::stop() check_mode: " << serverEnv_->check_mode_str()
              << " interval = " << serverEnv_->checkPtInterval() << "\n";
#endif
    running_ = false;
}

void CheckPtSaver::terminate() {
    timer_.cancel();
}

bool CheckPtSaver::explicitSave(bool from_server) const {
    try {
#ifdef DEBUG_CHECKPT
        std::cout << "      CheckPtSaver::explicitSave() Saving checkpt file " << serverEnv_->checkPtFilename() << "\n";
#endif
        // In order to detect *SLOW* operations (i.e. disk related), which *AFFECT* server performance,
        // the following timer measures how long we take to store the CheckPoint file.
        // After the Definitions are stored, if a configured warning threshold is passed a warning is issues.
        DurationTimer timer;

        fs::path checkPtFile(serverEnv_->checkPtFilename());
        fs::path oldCheckPtFile(serverEnv_->oldCheckPtFilename());
        CheckPtSaver::storeWithBackup(checkPtFile, oldCheckPtFile, [&](const fs::path& file_path) {
            server_->defs_->write_to_checkpt_file(file_path.string());
        });

        state_change_no_  = Ecf::state_change_no();  // For periodic update only save checkPt if it has changed
        modify_change_no_ = Ecf::modify_change_no(); // For periodic update only save checkPt if it has changed

        if (from_server) {
            // Create new time stamp otherwise we end up using the time stamp from the last command
            Log::instance()->cache_time_stamp();
            std::string msg = Str::SVR_CMD();
            msg += CtsApi::checkPtDefsArg();
            std::stringstream ss;
            ss << msg << " in " << timer.duration() << " seconds";
            log(Log::MSG, ss.str());
        }

        /// If Save take longer than checkpt_save_time_alarm, then set a flag on server
        /// So that user can be aware of it.
        if (static_cast<size_t>(timer.duration()) > server_->serverEnv_.checkpt_save_time_alarm()) {
            server_->defs_->flag().set(ecf::Flag::LATE);
            std::stringstream ss;
            ss << "Check pt save time(" << timer.duration() << ") is greater than alarm time("
               << server_->serverEnv_.checkpt_save_time_alarm()
               << "). Excessive save times can interfere with scheduling!";
            log(Log::WAR, ss.str());
        }
#ifdef DEBUG_CHECKPT
        std::cout << " backup and save took " << durationTimer.duration() << " seconds\n";
#endif
    }
    catch (std::exception& e) {
        std::string msg = "Could not save checkPoint file! ";
        msg += e.what();
        server_->defs_->flag().set(ecf::Flag::CHECKPT_ERROR);
        server_->defs()->server_state().add_or_update_user_variables("ECF_CHECKPT_ERROR", msg);
        LOG(Log::ERR, msg);
        return false;
    }

    return true;
}

void CheckPtSaver::periodicSaveCheckPt(const boost::system::error_code& error) {
#ifdef DEBUG_CHECKPT
    std::cout << "      CheckPtSaver::periodicSaveCheckPt() interval = " << serverEnv_->checkPtInterval()
              << "  time: " << to_simple_string(boost::posix_time::second_clock::universal_time()) << "\n";
#endif
    if (error == boost::asio::error::operation_aborted) {
#ifdef DEBUG_CHECKPT
        std::cout << "      CheckPtSaver::periodicSaveCheckPt : boost::asio::error::operation_aborted : time "
                     "cancelled: Node running("
                  << running_ << ")  interval = " << serverEnv_->checkPtInterval() << "\n";
#endif
        return;
    }
    else if (error) {
        LogFlusher logFlusher;
        LogToCout toCoutAsWell;
        LOG(Log::ERR, "CheckPtSaver::periodicSaveCheckPt " << error.message());
        return;
    }

    if (running_) {
        // state changed
        if (state_change_no_ != Ecf::state_change_no() || modify_change_no_ != Ecf::modify_change_no()) {
            doSave();
        }
    }

    /// Appears that expires_from_now is more accurate then expires_at
    timer_.expires_from_now(boost::posix_time::seconds(serverEnv_->checkPtInterval()));
    timer_.async_wait(boost::asio::bind_executor(
        server_->io_, [this](const boost::system::error_code& error) { periodicSaveCheckPt(error); }));
}

void CheckPtSaver::doSave() const {
#ifdef DEBUG_CHECKPT
    std::cout << "      CheckPtSaver::doSave()\n";
#endif
    // Check to see if configuration allows save
    if (serverEnv_->checkMode() == ecf::CheckPt::NEVER) {
#ifdef DEBUG_CHECKPT
        std::cout << "     CheckPtSaver::doSave() configuration does not allow save \n";
#endif
        return;
    }
    (void)explicitSave(true /* from the server, hence log */);
}

void CheckPtSaver::saveIfAllowed() {
    // Call on each state change. Hence will get many times
#ifdef DEBUG_CHECKPT_SAVE_ALLOWED
    std::cout << "      CheckPtSaver::saveIfAllowed()\n";
#endif

    // Check to see if configuration allows immediate save.
    if (serverEnv_->checkMode() == ecf::CheckPt::ALWAYS) {
        doSave();
    }
}
