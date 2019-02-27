//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include <sstream>

#include "AutoArchiveAttr.hpp"
#include "Calendar.hpp"
#include "Indentor.hpp"
#include "Log.hpp"
#include "Serialization.hpp"


#ifdef DEBUG
#include <boost/date_time/posix_time/time_formatters.hpp>
#endif

using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

namespace ecf {

void AutoArchiveAttr::print(std::string& os) const
{
   Indentor in;
   Indentor::indent(os) ; write(os);  os += "\n";
}

std::string AutoArchiveAttr::toString() const
{
   std::string ret;
   write(ret);
   return ret;
}

void AutoArchiveAttr::write(std::string& ret) const
{
   ret += "autoarchive ";
   if (days_) {
      ret += boost::lexical_cast<std::string>(time_.hour()/24) ;
      return;
   }
   if (relative_) ret += "+";
   time_.print(ret);
}


bool AutoArchiveAttr::operator==(const AutoArchiveAttr& rhs) const
{
   if (relative_ != rhs.relative_) return false;
   if (days_ != rhs.days_) return false;
   return time_.operator==(rhs.time_);
}

bool AutoArchiveAttr::isFree(const ecf::Calendar& calendar,const boost::posix_time::time_duration& suiteDurationAtComplete) const
{
   //                                                               suiteTime()
   //  suiteDurationAtComplete        autoarchive time              calendar duration
   //        |                             |                             |
   //        V                             V                             V
   // ----------------------------------------------------------------------------------> time
   //        ^                                                           ^
   //        |--------elapsed time---------------------------------------|
   //
   //

   if ( relative_ ) {
      time_duration timeElapsedAfterComplete = calendar.duration() - suiteDurationAtComplete;
      LOG_ASSERT(!timeElapsedAfterComplete.is_negative(),"should always be positive or some things gone wrong");
      if (timeElapsedAfterComplete >= time_.duration()) {
         return true;
      }
   }
   else {
      // real time
//#ifdef DEBUG
//    cout << "real time time_(" << to_simple_string(time_.duration())
//         << ") calendar.suiteTime().time_of_day(" << to_simple_string(calendar.suiteTime().time_of_day()) << ")\n";
//#endif
      if (calendar.suiteTime().time_of_day() >= time_.duration()) {
         return true;
      }
   }

   return false;
}


template<class Archive>
void AutoArchiveAttr::serialize(Archive & ar, std::uint32_t const version )
{
   ar( CEREAL_NVP(time_));
   CEREAL_OPTIONAL_NVP(ar, relative_, [this](){return !relative_;}); // conditionally save
   CEREAL_OPTIONAL_NVP(ar, days_,     [this](){return days_; });     // conditionally save
}
CEREAL_TEMPLATE_SPECIALIZE_V(AutoArchiveAttr);

}
