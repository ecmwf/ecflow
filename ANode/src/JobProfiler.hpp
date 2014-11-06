#ifndef JOB_PROFILER_HPP_
#define JOB_PROFILER_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #7 $
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//  This class is used to aid profiling of the job generation step.
//  This will be used to identify those suite/familiy/tasks that take the most
//  amount of time, *when* we exceed the jobs generation interval.
//  In particular if we have output that is many megabtyes, it can affect
//  the performance of the server, especially when the server is running
//  on virtual machines
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/noncopyable.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "NodeFwd.hpp"
class JobsParam;

namespace ecf {

class JobProfiler  : private boost::noncopyable  {
public:
   // Note: 1000 milliseconds = 1 second
   JobProfiler(Node*,JobsParam&,size_t threshold /* expected to be milli seconds */);
   ~JobProfiler();

   bool time_taken_for_job_generation_to_long() const;

   static void profile_to_log(const JobsParam& jobsParam);

   static std::string threshold_defaults();
   static void set_suite_threshold(size_t threshold);
   static void set_family_threshold(size_t threshold);
   static void set_task_threshold(size_t threshold);

   static size_t suite_threshold();
   static size_t family_threshold();
   static size_t task_threshold();

private:
   Node* node_;
   JobsParam& jobsParam_;
   size_t index_;
   boost::posix_time::ptime start_time_;
   size_t threshold_;
};

}


#endif
