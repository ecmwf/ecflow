#ifndef ABSTRACTSERVER_HPP_
#define ABSTRACTSERVER_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #45 $ 
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

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/shared_ptr.hpp>

#include "Stats.hpp"
#include "ZombieCtrl.hpp"
#include "SState.hpp"
#include "CheckPt.hpp"

class Defs;

/// This class is provided so that the cmd can issue requests to the server
/// without depending on the server implementation
///
class AbstractServer {
public:
	virtual ~AbstractServer() {}

	/// returns the current state of the server
	/// The following table shows the effect of state, on server behaviour:
	///
	///           User Request    Task Request   Job Scheduling   Check-pointing
	/// RUNNING      yes               yes              yes            yes
	/// SHUTDOWN     yes               yes              no             yes
	/// HALTED       yes               no               no             no
	virtual SState::State state() const = 0;

	/// returns the server host and port number
	virtual std::pair<std::string,std::string> hostPort() const = 0;

	/// returns the defs held by the server. This should always exist. ECFLOW-182
	virtual defs_ptr defs() const = 0;

	/// Update the defs help by the server. This allows multiple suites to loaded
	/// into the server. Note the input defs will be drained of its suites/externs as they
	/// will get transferred to the server. If the server already has suites of the same
	/// name, then an error message is created. This can be overridden with the force option
	virtual void updateDefs(defs_ptr, bool force) = 0;

	/// Remove all suites,externs,client handles, ready for a new start
 	virtual void clear_defs() = 0;

 	/// Forces the defs file in the server to be written to disk *IF* no args provided.
 	/// Otherwise updated mode OR check_pt interval OR check pt alarm
 	virtual void checkPtDefs(ecf::CheckPt::Mode m = ecf::CheckPt::UNDEFINED,
 	                            int check_pt_interval = 0,
 	                            int check_pt_save_time_alarm = 0) = 0;

 	/// Ask the server to restore the  defs from the checkpt file. If that fails the back check pt is tried
 	virtual void restore_defs_from_checkpt()  = 0;

	/// This function should be called, when the node tree changes state
	virtual void nodeTreeStateChanged() = 0;

	/// returns true if the server allows task communication
	/// Task-->server communication is stopped by halted()
	virtual bool allowTaskCommunication() const = 0;

	/// Stops job scheduling. Check point is enabled. (i,e if we were previously halted)
	/// However any client request can still communicate with the server
	/// Does NOT affect check pointing. Since any request can make changes to node tree
   /// Places server in SHUTDOWN state.
   ///           User Request    Task Request   Job Scheduling   Check-pointing
   /// RUNNING      yes               yes              yes            yes
   /// SHUTDOWN     yes               yes              no             yes
   /// HALTED       yes               no               no             no
	virtual void shutdown() = 0;

   /// Stop job scheduling *AND*  task communication with server. Failed task request are logged
	/// Hence nodes can be stuck in submitted/active states.
	/// Task based command will continue attempting, communication with the server for up to 24hrs.
	///
   /// When the server is halted, we do *NOT* do any further check pointing
   /// In a typical operational scenario where we have a home, and backup servers.
   /// The checkpoint file is copied to the backup servers periodically (via a task)
   /// hence we want to preserve the state of the last checkpoint. By prevent any state
	/// changes to the node tree.
	///
   /// Hence halted(), will completely stop the server. Server will only respond
	/// to user requests. (tasks requests are blocked)
   /// Places server in HALTED state.
   ///           User Request    Task Request   Job Scheduling   Check-pointing
   /// RUNNING      yes               yes              yes            yes
   /// SHUTDOWN     yes               yes              no             yes
   /// HALTED       yes               no               no             no
	virtual void halted() = 0;

	/// Start scheduling tasks and respond to all requests. Check pointing is enabled
	/// Places server in RUNNING state.
   ///           User Request    Task Request   Job Scheduling   Check-pointing
   /// RUNNING      yes               yes              yes            yes
   /// SHUTDOWN     yes               yes              no             yes
   /// HALTED       yes               no               no             no
	virtual void restart() = 0;

	/// Ask the server to reload file the hold list of users and their access rights
	/// The white list file is specified by the environment variable ECF_LISTS
	/// This allows/disallows user access on the live server
	/// Return true if file is reloaded ok, else false and error message if:
	/// 	a/ File does not exist
	///  	b/ File is empty
	///  	c/ Errors in parsing file
	/// If errors arise the exist user still stay in affect
	virtual bool reloadWhiteListFile(std::string& errorMsg)  = 0;

	/// There are several kinds of authentifications:
	///     a/ None
	///     b/ List mode.   ASCII file based on ECF_LISTS is defined. referred as white list file
	///     c/ Secure mode. binary file based ECF_PASSWD is defined. Referred to as black list file
	/// At the moment we will only implement options a/ and b/
	//
	/// Returns true if the given user has access to the server, false otherwise
	virtual bool authenticateReadAccess(const std::string& user) = 0;

	/// Returns true if user has matching write access privileges.
	virtual bool authenticateWriteAccess(const std::string& user ) = 0;

	/// Shutdown the server and let 'user' has have exclusive lock on it.
	/// If the lock succeeds return true, (This will end up calling the shutdown()
	/// command on the server). If already locked does nothing and return's false
 	virtual bool lock(const std::string& user) = 0;

	/// Unlock's the server., and restarts job scheduling
	virtual void unlock() = 0;

	/// Return the user that has exclusive lock, else an empty string
	virtual const std::string& lockedUser() const = 0;

	/// Return Controller for zombies
	ZombieCtrl& zombie_ctrl() { return zombie_ctrl_; }

	/// returns the statistical class
	Stats& stats() { return stats_;}

	/// Call to update the number of request, & returns stats
	Stats& update_stats() { stats_.update(); return stats_; }

	/// Update for number of requests per second
	void update_stats(int poll_interval) { stats_.update_stats(poll_interval); }

	// Instead of immediate node tree traversal at the end of child command
	// we use 'increment_job_generation_count' to defer job generation to server
	// The server will will check job generation count at poll time.
	// This approach radically reduces the number of times we traverse the node tree
	// and hence improves server throughput.
	void increment_job_generation_count() { job_gen_count_++;}
	void reset_job_generation_count() { job_gen_count_ = 0; }
	int get_job_generation_count() const { return job_gen_count_; }

	/// This job generation is special, in that it will time out, job generation time >= next poll.
	/// This can be called at the end of a *USER* command(force,alter,requeue,etc), hence time_now may be >= next_poll_time
	/// If this is the case, we will defer job generation
	virtual void traverse_node_tree_and_job_generate(const boost::posix_time::ptime& time_now,bool user_cmd_context) const = 0;

   /// returns the number of seconds at which we should check time dependencies
   /// this includes evaluating trigger dependencies and submit the corresponding jobs.
   /// This is set at 60 seconds. But will vary for debug/test purposes only.
   /// For Testing the state change queued->submitted->active duration < submitJobsInterval
   /// If this state change happens at the job submission boundary then
   /// time series can get a skew.
	virtual int poll_interval() const  = 0;

	/// enable and disable debug output from the server
	virtual void debug_server_on() = 0;
	virtual void debug_server_off() = 0;
	virtual bool debug() const = 0;

protected:
	AbstractServer() : job_gen_count_(0) {}

private:
	int job_gen_count_;
	ZombieCtrl zombie_ctrl_;
	Stats stats_;
};

#endif
