#ifndef CHECKPTSAVER_HPP_
#define CHECKPTSAVER_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : CheckPtSaver.cpp
// Author      : Avi
// Revision    : $Revision: #14 $
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//
// This class will save the defs file periodically. The period is obtained from
// ServerEnvironment. The save of the check point file is controlled by
// the settings in ServerEnvironment
//
// The checkpoint files is the defs file, with state. However its saved in
// boost serialisation format(i.e can be text,binary,portable binary)
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class ServerEnvironment;
class BaseServer;

class CheckPtSaver {
    CheckPtSaver(const CheckPtSaver&)                  = delete;
    const CheckPtSaver& operator=(const CheckPtSaver&) = delete;

public:
    CheckPtSaver(BaseServer* s, boost::asio::io_service& io, const ServerEnvironment*);
    ~CheckPtSaver();

    /// Start periodical save of the checkpoint file
    void start();

    /// Stop periodical save of the checkPoint file
    void stop();

    /// terminate: This will cancel the timer and any pending async operation
    /// If timer has already expired, then the handler function will be
    /// passed an operation aborted error code.
    /// Called when the server is exiting. We must be sure to cancel all
    /// async handlers or the server, will not stop.
    void terminate();

    /// Will always save the check pt file. Independent of any server environment
    /// The input argument is used in logging. When check pointing via user command
    /// we log the request. However we also check point automatically via the server
    /// This allows us to distinguish the two cases in the log file:
    /// Return true if no errors, otherwise false
    bool explicitSave(bool from_server = false) const;

    /// This function is called after node state changes. Check for save
    /// CheckPt::ON_TIME - will do nothing since we will save periodically
    /// CheckPt::NEVER   - will return immediately
    /// CheckPt::ALWAYS  - will save immediately, may cause performance issues with large Node trees
    void saveIfAllowed();

    /**
     * Stores a set of data in the current file location, ensuring a backup file retains the previous version.
     *
     * @tparam STORE the type of the function that stores the data
     * @param current the file path to the current data, which will become the backup
     * @param backup the file path to the previous backup, to be discarded
     * @param store the function that actually performs the storage
     */
    template <typename STORE>
    static void storeWithBackup(const fs::path& current, const fs::path& backup, STORE store) {
        // 1. Save the Definitions to a temporary CheckPoint file
        //
        // In case an issue occurs while saving the CheckPoint file (e.g. thrown
        // exception, or server crash), the procedure is always interrupted without
        // changes to the current+old CheckPoint files in order to facilitate recovery.
        //
        fs::path temporary = fs::unique_path(current.string() + "%%%%-%%%%-%%%%-%%%%");
        store(temporary.string());

        // 2. Backup the current CheckPoint files
        //
        // Important: The current checkpoint is only backed up if it exists
        // and has size greater than zero bytes. This approach avoid backing up
        // empty files, resulting from a full file system, and overriding relevant
        // old content that might be otherwise available.
        //
        // The backup effectively replaces the old with the current CheckPoint file.
        //
        if (fs::exists(current) && fs::file_size(current) > 0) {
            fs::remove(backup);
            fs::rename(current, backup);
        }

        // 3. Promote the temporary to current CheckPoint file
        fs::rename(temporary, current);
    }

private:
    /// save the node tree in the server to a checkPt file.
    /// this is controlled by the configuration. If the configuration does not
    /// allow a save, does nothing
    void doSave() const;

    /// Called periodically to save checkPoint file
    /// We use error parameter, since when we cancel the timer via, terminate
    /// we do NOT want to do an explicit save *PLUS* we want to return without
    /// starting another async operation. Otherwise the server will not return
    ///
    /// This will call doSave() but *ONLY* if there has been a state change
    /// This avoids writing out a checkpt file, unnecessarily & filling up log file
    void periodicSaveCheckPt(const boost::system::error_code& error);

    BaseServer* server_;
    boost::asio::deadline_timer timer_;
    bool firstTime_;
    bool running_;
    const ServerEnvironment* serverEnv_;
    mutable unsigned int state_change_no_;  // detect state change in defs
    mutable unsigned int modify_change_no_; // detect state change in defs
};

#endif
