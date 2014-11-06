//============================================================================
// Name        : NodeTreeTraverser.cpp
// Author      : Avi
// Revision    : $Revision: #101 $ 
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

#include <iostream>
#include "boost/bind.hpp"

#include "ServerEnvironment.hpp"
#include "NodeTreeTraverser.hpp"
#include "Server.hpp"
#include "Defs.hpp"
#include "JobsParam.hpp"
#include "Jobs.hpp"
#include "Log.hpp"
#include "CalendarUpdateParams.hpp"
#include "Calendar.hpp"

using namespace ecf;
using namespace boost::posix_time;

//#define DEBUG_TRAVERSER 1
//#define DEBUG_POLL 1

// ***********************************************************************
// It was noticed that having a poll of 60 seconds was not very accurate
// even when the server was not active, could be out by as much as 25 seconds
// Hence we poll every second, and check it against the minute boundary
// ************************************************************************

NodeTreeTraverser::NodeTreeTraverser( Server* s,
 										boost::asio::io_service& io,
										const ServerEnvironment& serverEnv)
: server_( s ),
  serverEnv_(serverEnv),
  timer_( io, boost::posix_time::seconds( 0 ) ),
  interval_(0,0,serverEnv_.submitJobsInterval(),0),
  count_( 0 ),
  firstTime_( true),
  running_(false)
{
#ifdef DEBUG_TRAVERSER
	std::cout << "NodeTreeTraverser::NodeTreeTraverser period = " << serverEnv_.submitJobsInterval() << "\n";
#endif
}

NodeTreeTraverser::~NodeTreeTraverser() {
#ifdef DEBUG_TRAVERSER
	std::cout << "~NodeTreeTraverser::NodeTreeTraverser\n";
#endif
}

void NodeTreeTraverser::start()
{
#ifdef DEBUG_TRAVERSER
	{LogToCout toCoutAsWell;LOG(Log::DBG, "NodeTreeTraverser::start() server_state(" << SState::to_string(server_->state()) << ") running(" << running_ << ")  count(" << count_ << ") firstTime_(" << firstTime_ << ")" );}
#endif

	if (!running_) {
		count_ = 0;
		running_ = true;
 		if (firstTime_) {
 		 	// If the server is stopped/started we want to avoid skewing the calendar, since each time
 		 	// we call traverse, the calendar is updated with the server poll. hence we must make sure
 		 	// we only start it once here
 			last_time_ = Calendar::second_clock_time();
         next_poll_time_ = last_time_;
 			firstTime_ = false;

 			/// ==========================================================================
 			/// Make sure we *ALIGN* the poll period exactly *** to the minute*** boundary
         /// ==========================================================================
 			if ( 60 == serverEnv_.submitJobsInterval() ) {

 			   time_duration time_of_day = last_time_.time_of_day();
 			   int seconds_to_minute_boundary =  60 - time_of_day.seconds();

#ifdef DEBUG_TRAVERSER
            std::cout << "  NodeTreeTraverser::start: time_of_day(" << to_simple_string(time_of_day) << ") seconds_to_minute_boundary(" << seconds_to_minute_boundary << ")\n";
#endif

 			   if ( seconds_to_minute_boundary != 0) {

#ifdef DEBUG_TRAVERSER
               std::cout << "  NodeTreeTraverser::start: Do an immediate job generation. Since we don't want to wait for minute boundary, when starting.\n";
#endif
 	            update_suite_calendar_and_traverse_node_tree(last_time_);

 	            // Make sure subsequent polls are *ALIGNED* to minute boundary
 			      next_poll_time_ = last_time_ + seconds(seconds_to_minute_boundary);
 			      timer_.expires_from_now(  boost::posix_time::seconds( 1 ) );

#ifdef DEBUG_TRAVERSER
 	            std::cout << "  NodeTreeTraverser::start: next_poll_time_(" << to_simple_string(next_poll_time_) << ")\n";
#endif
 			   }
 			}
#ifdef ECFLOW_MT
 			timer_.async_wait( server_->strand_.wrap( boost::bind( &NodeTreeTraverser::traverse, this, boost::asio::placeholders::error ) ) );
#else
 			timer_.async_wait( server_->io_service_.wrap( boost::bind( &NodeTreeTraverser::traverse, this, boost::asio::placeholders::error ) ) );
#endif
 		}
	}
}

void NodeTreeTraverser::stop()
{
#ifdef DEBUG_TRAVERSER
   {LogToCout toCoutAsWell; LOG(Log::DBG, "   NodeTreeTraverser::stop() count(" << count_ << ")");}
#endif

 	running_ = false;
}

void NodeTreeTraverser::terminate()
{
#ifdef DEBUG_TRAVERSER
   {LogToCout toCoutAsWell; LOG(Log::DBG, "   NodeTreeTraverser::terminate() count(" << count_ << ")");}
#endif

   timer_.cancel();
}

void NodeTreeTraverser::do_traverse()
{
   // since we poll every second, if less than next poll(every 60 seconds) continue.
   ptime time_now = Calendar::second_clock_time();
   if (time_now < next_poll_time_) {

      // minimise the number of node tree traversal, to once every second, but only *IF* required
      // Note: if we are a few seconds to the poll time, but job generation takes a while
      //       we can get warning about the interval took to long. See below:

      // LOG(Log::DBG,"get_job_generation_count() = " << server_->get_job_generation_count());
      if (server_->get_job_generation_count() > 0) {
         traverse_node_tree_and_job_generate( time_now );
      }

      start_timer(); // timer fires *EVERY* second
      return;
   }
   // time_now >= next_poll_time_


	// We have SOFT real time, we poll every second, BUT only update the suite calendar at the job submission
   // interval. However we can not guarantee to hit exactly at the next poll time
	// *** traverse node tree and increment next_poll_time_ ***
	time_duration duration = time_now - last_time_;
	int diff_from_last_time = duration.total_seconds();
	int submitJobsIntervalInSeconds = serverEnv_.submitJobsInterval();
#ifdef DEBUG_TRAVERSER
	int real_diff = diff_from_last_time - submitJobsIntervalInSeconds;
	std::stringstream ss;
	ss << "   NodeTreeTraverser::traverse() diff_from_last_time:" << diff_from_last_time << " running:" << running_ << " count:" << count_ << " real_diff:" << real_diff << "  time_now:" << to_simple_string(time_now);
	if ( diff_from_last_time == 0) ss << ": FIRST time: ";
#endif

	/// Update server stat's. ie records number of requests for each poll period
	server_->update_stats(diff_from_last_time);

	/// The poll times will *vary* since we are trying to keep up with the hard real time.
	if ( diff_from_last_time > submitJobsIntervalInSeconds ) {

	   /// This will happen from time to time, hence only report, for real wayward times
	   int diff = diff_from_last_time - submitJobsIntervalInSeconds;
	   if (diff > (submitJobsIntervalInSeconds * 0.25)) {
//#ifdef DEBUG
	      LogToCout toCoutAsWell;
//#endif
	      LOG(Log::WAR, ": interval is (" << submitJobsIntervalInSeconds << " seconds)  but took (" << diff_from_last_time  <<  " seconds)" );
	   }
	}



	/// Remove any stale zombies
	server_->zombie_ctrl().remove_stale_zombies(time_now);

#ifdef DEBUG_TRAVERSER
   time_duration traverse_duration =  Calendar::second_clock_time() - time_now;
   ss << " Traverse duration:" << traverse_duration.total_seconds();
#endif


   // We poll *EVERY second but update the next_poll_time_ to be consistent with the job submission interval
   // Hence the next poll times *will* vary( SOFT REAL TIME ).
   // Note: On server start, we modified the next_poll_time_ to hit the minute boundary.
   //
   //         FIRST          time_now(skip)                                   time_now(traverse node tree)
   //           |                |         L                                      |         S
   //           V                V------------------------|                       V-----------------|
   // ==========0====================0====================0====================0====================0
   //           ^                    ^                    ^                    ^                    ^
   //           |                    |                    |                    |                    |
   //         last_time_       next_poll_time_       next_poll_time_      next_poll_time_      next_poll_time_
   //         next_poll_time_
   //                                ^                                         ^
   //                                |                                         |
   //                             last_time_                                last_time_
   //                             reset to previous next_poll_time_, to avoid yo-yoing, messages about poll being long/short
   //
   // Update next_poll_time_: WE  *ONLY* get here if time_now >= next_poll_time

   if (time_now > next_poll_time_) {

      /// Continue updating next_poll_time_ by interval_ until it is greater time_now
#ifdef DEBUG_TRAVERSER
      {  ss << ": Shorten the poll time : Current time(" << to_simple_string(time_now) << ") > current poll_time(" << to_simple_string(next_poll_time_) << ")";
         time_duration diff =  time_now - next_poll_time_;
         ss << " by " << diff.total_seconds() << " seconds: "; }
#endif

      while (next_poll_time_ <= time_now) { next_poll_time_ += interval_;}
   }
   else {

      /// Hit the poll time: Should get here when traverse called for the *FIRST* time ( since time_now == next_poll_time_)
#ifdef DEBUG_TRAVERSER
      ss << ": On poll time: ";
#endif

      next_poll_time_ += interval_;
   }

   // At begin time for very large suites, slow disk, and in test, during job generation we can miss the next poll time(i.e a,b)
   // This means that on the next poll time because last_time was not updated, to be immediately
   // behind the next poll time by 'interval_' seconds an erroneous report is logged about missing the poll time
   //
   //         FIRST                                                               Time_now
   //           |                                                                    |
   //           V                    a                    b                    c     X
   // ==========0====================0====================0====================0====================0
   //           ^                    ^                    ^                    ^                    ^
   //           |                    |                    |                    |                    |
   //         last_time_       next_poll_time_       next_poll_time_      next_poll_time_      next_poll_time_
   //
   // Hence we need to ensure that last_time is always less that next_poll_time_ by interval_
   // In the diagram above missed poll time a and b, then we need to set last_time_ to 'c' and *NOT* 'a' | 'b'
   last_time_ = next_poll_time_ - interval_;


#ifdef DEBUG_TRAVERSER
	{ ss << " Next Poll at:" << to_simple_string(next_poll_time_);LogToCout toCoutAsWell; LOG(Log::DBG,ss.str()); }
#endif


   // Start node tree traversal. 
	// ************************************************************************************************
   // ** This relies on next_poll_time_ being set first, to ensure job generation does not take longer
	// ************************************************************************************************
   update_suite_calendar_and_traverse_node_tree(time_now);

	start_timer();  // timer fires *EVERY* second
}

void NodeTreeTraverser::start_timer()
{
   /// Appears that expires_from_now is more accurate then expires_at i.e timer_.expires_at( timer_.expires_at() + boost::posix_time::seconds( poll_at ) );
   timer_.expires_from_now(  boost::posix_time::seconds( 1 ) );
#ifdef ECFLOW_MT
   timer_.async_wait( server_->strand_.wrap( boost::bind( &NodeTreeTraverser::traverse,this,boost::asio::placeholders::error ) ) );
#else
   timer_.async_wait( server_->io_service_.wrap( boost::bind( &NodeTreeTraverser::traverse,this,boost::asio::placeholders::error ) ) );
#endif
}

void NodeTreeTraverser::traverse(const boost::system::error_code& error )
{
	if (error == boost::asio::error::operation_aborted) {
#ifdef DEBUG_TRAVERSER
   { LogToCout toCoutAsWell; LOG(Log::DBG, "NodeTreeTraverser::traverse Timer was cancelled" ); }
#endif
  	 	return;
	}
	else if (error) {
	 	LogToCout toCoutAsWell;
	 	std::string msg = "NodeTreeTraverser::traverse error: "; msg += error.message();
  	 	ecf::log(Log::ERR, msg);
  	 	return;
 	}

	do_traverse();
}


void NodeTreeTraverser::update_suite_calendar_and_traverse_node_tree(const boost::posix_time::ptime& time_now)
{
   // *****************************************************************************
   // JOB SUBMISSION SEEMS TO WORK BEST IF THE CALENDAR INCREMENT HAPPENS FIRST
   // HOWEVER THIS MEANS WE MAY MISS THE VERY FIRST JOB SUBMISSION. ??
   // Note: events,meters and task completion kick of another job submission
   //       There seems to be greater stability in terms of testing as it allows
   //       process to complete, before the calendar is incremented.
   // *****************************************************************************
   if ( server_->defs_ ) {

      // This functions gets called every 60 seconds or so, update calendar && time
      // dependent variables in case any jobs depend on them. By default the calendar
      // update interval is the same as submitJobsInterval for non-real calendars,
      // however for testing both real/non-real calendars the calendar increment can be
      // changed to speed up calendar. This is done by setting the calendar increment
      // on the suite(i.e in the defs file)  which will the _override_ this setting.
      //
      // Additionally by passing in the flag running_, it allow suites which want to
      // stop the calendar updates, when the server is stopped to do so.
      // For real time calendars we make one system call here, instead of many times in each suite
      //
      // In the case where defs/node tree is suspended updateCalendar will continue
      // to mark those time dep' are free, as free. This information is then used
      // during the resume
      ++count_;
      CalendarUpdateParams calParams(time_now, interval_/* calendar increment */, running_ );
      server_->defs_->updateCalendar( calParams );

      traverse_node_tree_and_job_generate(time_now);
   }
}

void NodeTreeTraverser::traverse_node_tree_and_job_generate(const boost::posix_time::ptime& start_time)
{
   if ( running_ && server_->defs_) {
       server_->reset_job_generation_count();

       // Pass submit jobs interval, so that we can check jobs submission occurs within the allocated time.
       // By default job generation is enabled, however for testing, allow job generation to be disabled.
       JobsParam jobsParam(serverEnv_.submitJobsInterval(), serverEnv_.jobGeneration());

       // If job generation takes longer than the time to *reach* next_poll_time_, then time out.
       // Hence we start out with 60 seconds, and time for job generation should decrease. Until reset back to 60
       // Should allow greater child communication.
       // By setting set_poll_time, we enable timeout of job generation.
       // Note: There are other place where we may not want to timeout job generation.
       jobsParam.set_poll_time(next_poll_time_);

#ifdef DEBUG_JOB_SUBMISSION
       jobsParam.logDebugMessage(" from NodeTreeTraverser::traverse_node_tree_and_job_generate()");
#endif
       Jobs jobs(server_->defs_);
       if (!jobs.generate(jobsParam)) { ecf::log(Log::ERR, jobsParam.getErrorMsg()); }
       if (jobsParam.timed_out_of_job_generation()) {

          // Implies time now >= next_poll_time_,
          // It could be that we started job generation a few seconds before the poll time,
          // Hence to avoid excessive warnings, Only warn if time_now > next_poll_time_ and  forgive about 10  seconds
          ptime time_now = Calendar::second_clock_time();
          if (time_now > next_poll_time_ ) {
//             time_duration duration = time_now - next_poll_time_;
//             if ( duration.total_seconds() > 10) {
//#ifdef DEBUG
          LogToCout toCoutAsWell;
//#endif
                std::stringstream ss;
                ss << "Job generation *timed* out: start time:" << start_time << "  time_now:" << time_now << "  poll_time:" << next_poll_time_;
                ecf::log(Log::WAR,ss.str());
//             }
          }
       }
    }
}

