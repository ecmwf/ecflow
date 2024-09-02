/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_server_NodeTreeTraverser_HPP
#define ecflow_server_NodeTreeTraverser_HPP

///
/// \brief This class will traverse the node tree periodically, It is tied to a server.
/// This implementation uses a strand to ensure sequential processing of the node dependency traversal
/// in the presence of multiple threads, without the need of explicit locking. i.e mutex's
///
/// For testing we make the distinction between the poll period, and calendar update interval
/// Some suites require a real time calendar. Hence we must make sure that
/// poll interval is in sync with real time.
///

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

class BaseServer;
class ServerEnvironment;
// #define DEBUG_TRAVERSER 1

class NodeTreeTraverser {
public:
    NodeTreeTraverser(BaseServer* s, boost::asio::io_context& io, const ServerEnvironment& serverEnv);
    // Disable copy (and move) semantics
    NodeTreeTraverser(const NodeTreeTraverser&)                  = delete;
    const NodeTreeTraverser& operator=(const NodeTreeTraverser&) = delete;

    ~NodeTreeTraverser();

    /// If first time Starts traversing Node tree and resolving dependencies.
    /// This essentially starts Jobs scheduling. Calling start more than once does nothing.
    /// On subsequent calls to start resume is called. Which may cause immediate
    /// job generation. (i.e for those nodes whose node are free of time dependencies)
    /// Timer will be aligned to the minute boundary
    void start();

    /// Suspends job scheduling (for real time calendars/suite)
    /// Shutdown & suspend (are very similar) shutdown operate at the top/level.
    /// Both should have effect of stopping job submission and not the event loop
    ///
    /// During a system session, a node can be placed into suspend mode.
    /// In suspend mode, no jobs are submitted. (**** However node which are
    /// free to run are marked****).
    ///
    /// When the session is resumed, those node than were marked, have
    /// the task's submitted. ( This means that job dependent on a time dependency
    /// does not need to be held for the following day)
    void stop();

    /// terminate: This will cancel the timer and any pending async operation
    /// If timer has already expired, then the handler function will be
    /// passed an operation aborted error code.
    /// Called when the server is exiting. We must be sure to cancel all
    /// async handlers or the server, will not stop.
    void terminate();

    /// This can be called at the end of a *USER* command(force,alter,requeue,etc), hence time_now may be >= poll_time
    /// If this is the case, we will defer job generation
    void traverse_node_tree_and_job_generate(const boost::posix_time::ptime& time_now, bool user_cmd_context) const;

private:
    void traverse(const boost::system::error_code& error);
    void do_traverse();
    void start_timer();
    void update_suite_calendar_and_traverse_node_tree(const boost::posix_time::ptime& time_now);

    BaseServer* server_;
    const ServerEnvironment& serverEnv_;
    boost::asio::deadline_timer timer_;
    boost::posix_time::ptime last_time_;        // ensure poll is in sync
    boost::posix_time::ptime next_poll_time_;   // Keep as sync as possible with hard real times
    boost::posix_time::time_duration interval_; // Job submission interval
#ifdef DEBUG_TRAVERSER
    int count_;
#endif
    bool firstTime_;
    bool running_;
};

#endif /* ecflow_server_NodeTreeTraverser_HPP */
