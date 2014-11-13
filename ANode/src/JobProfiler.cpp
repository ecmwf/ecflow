//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #18 $
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

#include <boost/lexical_cast.hpp>
#include "JobProfiler.hpp"
#include "JobsParam.hpp"
#include "Task.hpp"
#include "Str.hpp"
#include "Log.hpp"

using namespace ecf;
using namespace std;

// Connection and client timeout issues can be replicated  by adding
//   - sleep(1) in EcfFile , i.e when creating the job output
//

static size_t task_threshold_ = 2000;

namespace ecf {

int JobProfiler::task_threshold_default() { return 2000;}

// =================================================================================
JobProfiler::JobProfiler(Task* node,JobsParam& jobsParam, size_t threshold)
: node_(node),
  jobsParam_(jobsParam),
  start_time_(boost::posix_time::microsec_clock::universal_time()),
  threshold_(threshold)
{
	if (!jobsParam_.next_poll_time().is_special() && start_time_ >= jobsParam_.next_poll_time()) {
		jobsParam_.set_timed_out_of_job_generation(start_time_);
	}
}

JobProfiler::JobProfiler( JobsParam& jobsParam)
: node_(0),
  jobsParam_(jobsParam),
  start_time_(boost::posix_time::microsec_clock::universal_time()),
  threshold_(0)
{
	if (!jobsParam_.next_poll_time().is_special() && start_time_ >= jobsParam_.next_poll_time()) {
		jobsParam_.set_timed_out_of_job_generation(start_time_);
	}
}

JobProfiler::~JobProfiler()
{
   if (node_) {
      boost::posix_time::time_duration duration = boost::posix_time::microsec_clock::universal_time() - start_time_;
      size_t time_taken = duration.total_milliseconds();

      // When testing we set submitJobsInterval to < 0
      if (jobsParam_.submitJobsInterval() < 0 ) {
         time_taken = threshold_ + 1;
      }

      if ( time_taken > threshold_)  {
         std::stringstream ss;
         ss << "Job generation for task " << node_->absNodePath() << " took " << time_taken << "ms, Exceeds ECF_TASK_THRESHOLD(" << threshold_ << "ms)";
         log(Log::WAR,ss.str());
      }
   }
}

void JobProfiler::set_task_threshold(size_t threshold){task_threshold_ = threshold;}
size_t JobProfiler::task_threshold() { return task_threshold_; }

}
