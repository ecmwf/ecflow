#ifndef DURATIONTIMER_HPP_
#define DURATIONTIMER_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #6 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Simple class the reports wall clock time duration
//============================================================================
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace ecf {

class DurationTimer {
public:
	DurationTimer() : start_time_(boost::posix_time::microsec_clock::universal_time()) {}
	~DurationTimer() {}

	int duration() const {
		boost::posix_time::time_duration duration = boost::posix_time::microsec_clock::universal_time() - start_time_;
		return duration.total_seconds();
	}

	boost::posix_time::time_duration elapsed() const { return boost::posix_time::microsec_clock::universal_time() - start_time_;}

private:
 	boost::posix_time::ptime start_time_;
};

}
#endif
