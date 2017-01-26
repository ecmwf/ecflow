#ifndef DATEATTR_HPP_
#define DATEATTR_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #24 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Note: calendarChanged() once a Date is free, it stays free
//               It relies on parent cron/repeat to re-queue
//============================================================================

#include <ostream>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/level.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace ecf { class Calendar;} // forward declare class that is in a name space

// Use default copy constructor, assignment operator, destructor
// Value of 0 for day,month,year means *, i.e. means any value
class DateAttr {
public:
   DateAttr(int day, int month, int year); // will throw std::out_of_range for if invalid date
   DateAttr()
   : day_(0), month_(0), year_(0), makeFree_(false),
     state_change_no_(0) {} // for serialisation
   DateAttr(const boost::gregorian::date& date)
   : day_(date.day()), month_(date.month()), year_(date.year()), makeFree_(false),
     state_change_no_(0) {} // for test

   std::ostream& print(std::ostream&) const;
   bool operator==(const DateAttr& rhs) const;
   bool operator<(const DateAttr& rhs) const;
   bool structureEquals(const DateAttr& rhs) const;

   void setFree();   // ensures that isFree() always returns true
   void clearFree(); // resets the free flag
   bool isSetFree() const { return makeFree_; }
   void calendarChanged( const ecf::Calendar& c ); // can set attribute free
   bool isFree(const ecf::Calendar&) const;
   bool checkForRequeue( const ecf::Calendar&) const;
   bool validForHybrid(const ecf::Calendar&) const;
   bool why(const ecf::Calendar&, std::string& theReasonWhy) const;

   // The state_change_no is never reset. Must be incremented if it can affect equality
   unsigned int state_change_no() const { return state_change_no_; }

   std::string name() const { return toString(); } /* ABO */
   std::string toString() const;
   std::string dump() const;

   /// Check the date, will throw std::out_of_range or derivative if invalid
   static void checkDate(int day, int month, int year, bool allow_wild_cards);

   /// Extract the date, if return integer is zero, date was of the *, i.e. any day,month,year
   /// will throw std::runtime_error for parse errors
   /// expect:
   ///    15.11.2009
   ///    15.*.*
   ///    *.1.*
   static void getDate(const std::string& date,int& day,int& month,int& year);
   static DateAttr create(const std::string& dateString);

   // access
   int day() const { return day_; }
   int month() const { return month_; }
   int year() const { return year_; }

private:
   bool is_free(const ecf::Calendar&) const; // ignores makeFree_

private:
   int          day_;
   int          month_;
   int          year_;
   bool         makeFree_; // persisted for use by why() on client side
   unsigned int state_change_no_;  // *not* persisted, only used on server side

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/)
   {
      ar & day_;
      ar & month_;
      ar & year_;
      ar & makeFree_;
   }
};

// This should ONLY be added to objects that are *NOT* serialised through a pointer
BOOST_CLASS_IMPLEMENTATION(DateAttr, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(DateAttr,boost::serialization::track_never);

#endif
