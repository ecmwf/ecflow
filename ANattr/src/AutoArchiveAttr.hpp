#ifndef AUTOARCHIVEATTR_HPP_
#define AUTOARCHIVEATTR_HPP_
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

#include "TimeSlot.hpp"
#include "NState.hpp"

namespace ecf { class Calendar;} // forward declare class

namespace ecf {

// Use compiler ,  destructor, assignment, copy constructor
class AutoArchiveAttr  {
public:
   AutoArchiveAttr()= default;
   AutoArchiveAttr(int hour, int minute, bool relative, bool idle=false) : time_(hour, minute), relative_(relative),idle_(idle) {}
   AutoArchiveAttr(const TimeSlot& ts,   bool relative, bool idle=false) : time_(ts), relative_(relative),idle_(idle) {}
   AutoArchiveAttr(int days,bool idle = false)  : time_( TimeSlot(days*24,0) ),  days_(true),idle_(idle) {}

   void print(std::string&) const;
   bool operator==(const AutoArchiveAttr& rhs) const;
   bool operator<(const AutoArchiveAttr& rhs) const { return time_ < rhs.time();}
   bool isFree(const ecf::Calendar&, const std::pair<NState,boost::posix_time::time_duration>& last_state_and_change_duration) const;

   std::string toString() const;

   const TimeSlot& time() const { return time_;}
   bool relative() const { return relative_; }
   bool days() const { return days_; }
   bool idle() const { return idle_; }

private:
   void write(std::string&) const;

private:
   TimeSlot time_;
   bool relative_{true};
   bool days_{false};
   bool idle_{false};

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version );
};

}
#endif
