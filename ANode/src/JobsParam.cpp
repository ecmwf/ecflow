//============================================================================
// Name        : time
// Author      : Avi
// Revision    : $Revision: #14 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include "JobsParam.hpp"


bool JobsParam::check_for_job_generation_timeout()
{
   if (timed_out_of_job_generation_) {
      return true;
   }

   boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::universal_time();
   if (!next_poll_time_.is_special() && start_time >= next_poll_time_) {
      set_timed_out_of_job_generation(start_time);
      return true;
   }
   return false;
}

void JobsParam::clear() {
   errorMsg_.clear();
   debugMsg_.clear();
   submitted_.clear();
   user_edit_file_.clear();
   user_edit_variables_.clear();
}

