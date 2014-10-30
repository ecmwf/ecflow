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
#include "Node.hpp"
#include "Str.hpp"

using namespace ecf;
using namespace std;

// To debug the output enable this, and then run the Node test
//#define DEBUG_ME 1
//#define DEBUG_OUTPUT 1

// Connection and client timeout issues can be replicated  by adding
//   - sleep(1) in EcfFile , i.e when creating the job output
//
// Interpreting the output:
//   - Please note we are measuring the total time taken by node and *ALL* of its children
//     hence just adding the times, will not always add up to the total, shown on the suite/family
//   - Hence in the log file below, we can see the main contribution time comes from tasks.
//
//MSG:[15:08:18 30.10.2014] SUITE:/verify - 2s
//MSG:[15:08:18 30.10.2014]  FAMILY:/verify/06 - 1s
//MSG:[15:08:18 30.10.2014]   FAMILY:/verify/06/bc - 1s
//MSG:[15:08:18 30.10.2014]    TASK:/verify/06/bc/bc_upperair - job size:2514 - 1s
//MSG:[15:08:18 30.10.2014]  FAMILY:/verify/18 - 1s
//MSG:[15:08:18 30.10.2014]   FAMILY:/verify/18/bc - 1s
//MSG:[15:08:18 30.10.2014]    TASK:/verify/18/bc/bc_upperair - job size:2514 - 1s

namespace ecf {

// initialise globals
int JobProfiler::counter_ = -1;
// Profiling could be controlled by a command ?

// =================================================================================
JobProfiler::JobProfiler(Node* node,JobsParam& jobsParam)
: node_(node),
  jobsParam_(jobsParam),
  index_(jobsParam.start_profile()),
  start_time_(boost::posix_time::microsec_clock::universal_time())
{
   counter_ += 1;
}

JobProfiler::~JobProfiler()
{
   boost::posix_time::time_duration duration = boost::posix_time::microsec_clock::universal_time() - start_time_;

#ifdef DEBUG_OUTPUT
   int time_taken = duration.total_milliseconds();
#else
   int time_taken = duration.total_seconds();
#endif

#ifdef DEBUG_ME
   update(time_taken);
#else
   if ( time_taken > 0)  update(time_taken);
   else                 jobsParam_.set_to_profile(index_,Str::EMPTY(),0); // clear any additions, caused by task, ie. job size
#endif

   counter_ -= 1;
}

void JobProfiler::update(int time_taken)
{
   // This class can be called hierarchically, so produce nicely indented output
   std::string text;
   for(int i = 0; i < counter_; i++) text += ' ';
   text += node_->debugNodePath();
   text += " - ";

   // check if any addition were made, typically for tasks we will add job size.
   const std::string& additions = jobsParam_.get_text_at_profile(index_);
   if (!additions.empty()) {
      text += additions;
      text += " - ";
   }

   text += boost::lexical_cast<std::string>( time_taken );
   text += "s";
   jobsParam_.set_to_profile(index_,text,time_taken);
}

}
