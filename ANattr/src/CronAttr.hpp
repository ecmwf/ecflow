#ifndef CRONATTR_HPP_
#define CRONATTR_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $ 
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

#include "TimeSeries.hpp"

namespace ecf { class Calendar;} // forward declare class

namespace ecf {

// *****************************************************************
// A cron attribute with a single time slot is repeated indefinitely
// ******************************************************************
// Use compiler ,  destructor, assignment, copy constructor,
class CronAttr  {
public:
   CronAttr();
   explicit CronAttr(const std::string& time_series);
   CronAttr(const TimeSlot& s, const TimeSlot& f, const TimeSlot& i) : timeSeries_(s,f,i) {}
   explicit CronAttr(const TimeSeries& ts ) : timeSeries_(ts) {}
   CronAttr(int h, int m, bool relative = false) : timeSeries_(h,m,relative) {}

   void print(std::string&) const;
   bool operator==(const CronAttr& rhs) const;
   bool operator<(const CronAttr& rhs) const { return timeSeries_ < rhs.timeSeries_; }
   bool structureEquals(const CronAttr& rhs) const;

   void addWeekDays( const std::vector<int>& w);
   void add_last_week_days_of_month(const std::vector<int>& w);
   void addDaysOfMonth( const std::vector<int>& d);
   void add_last_day_of_month() { last_day_of_month_ = true;}
   void addMonths( const std::vector<int>& m);

   void addTimeSeries( const TimeSlot& s, const TimeSlot& f, const TimeSlot& i) { timeSeries_ = TimeSeries(s,f,i);}
   void addTimeSeries( const TimeSeries& ts ) { timeSeries_ = ts;}
   void add_time_series( int h, int m, bool relative = false ) { timeSeries_ = TimeSeries(h,m,relative);}

   // Once a cron is free its stays free, until re-queue is called
   void calendarChanged( const ecf::Calendar& c ); // can set attribute free
   void resetRelativeDuration();

   void reset_only();
   void reset(const ecf::Calendar& c);
   void requeue(const ecf::Calendar& c,bool reset_next_time_slot = true);

   void miss_next_time_slot();
   void setFree();   // ensures that isFree() always returns true
   bool isSetFree() const { return free_; }
   bool isFree( const ecf::Calendar&) const;
   bool checkForRequeue( const ecf::Calendar&) const;
   bool validForHybrid(const ecf::Calendar&) const;
   bool why(const ecf::Calendar&, std::string& theReasonWhy) const;
   bool last_day_of_the_month() const { return last_day_of_month_;}

   // The state_change_no is never reset. Must be incremented if it can affect equality
   // Note: changes in state of timeSeries_, i.e affect the equality operator (used in test)
   //       must be captured. i.e things like relative duration & next_time_slot are
   //       reported by the Why command, & hence need to be synced.
   unsigned int state_change_no() const { return state_change_no_; }

   bool checkInvariants(std::string& errormsg) const;

   //Query:
   const TimeSeries& time() const { return timeSeries_;}
   const TimeSeries& time_series() const { return timeSeries_;}
   bool last_day_of_month() const { return last_day_of_month_;}
   std::vector<int>::const_iterator week_days_begin() const { return weekDays_.begin();}
   std::vector<int>::const_iterator week_days_end()   const { return weekDays_.end();  }
   std::vector<int>::const_iterator last_week_days_of_month_begin()   const { return last_week_days_of_month_.begin();}
   std::vector<int>::const_iterator last_week_days_end_of_month_end() const { return last_week_days_of_month_.end();  }
   std::vector<int>::const_iterator days_of_month_begin() const { return daysOfMonth_.begin();}
   std::vector<int>::const_iterator days_of_month_end()   const { return daysOfMonth_.end();  }
   std::vector<int>::const_iterator months_begin() const { return months_.begin();}
   std::vector<int>::const_iterator months_end()   const { return months_.end();  }

   std::string name() const { return toString(); } /* ABO */
   std::string toString() const;
   std::string dump() const;

   /// parse the line tokens an create a cron attribute. Can throw std::runtime_error
   /// The index parameter allows us to parse:
   /// cron -w 0,1 10:00      // index = 1
   /// -w 0,1 10:00           // index = 0
   /// Expect to parse:
   /// 	cron 23:00                 # run every day at 23:00
   /// 	cron 10:00 20:00 01:00     # run every hour between 10am and 8pm
   ///   cron -w 0,1 10:00          # run every sunday and monday at 10am
   ///   cron -w 0,1L  10:00        # run every sunday and last monday of the month at 10am
   ///   cron -d 10,11,12 12:00     # run 10th, 11th and 12th of each month at noon
   ///   cron -d 10,L     12:00     # run 10th, and last day of the month at noon
   /// 	cron -m 1,2,3 12:00        # run on Jan,Feb and March every day at noon.
   ///   cron -w 0   -m 5,6,7,8 10:00 20:00 01:00 # run every sunday, between May-Aug, every hour between 10am and 8pm
   ///   cron -d 1,L -m 5,6,7,8 10:00 20:00 01:00 # run on first and last day of month, between May-Aug, every hour between 10am and 8pm
   static void parse( CronAttr&, const std::vector<std::string >& lineTokens, size_t index, bool parse_state = false );
   static CronAttr create(const std::string& cronString);

private:
   void write(std::string&) const;

private:
   void clearFree(); // resets the free flag
   bool is_day_of_week_day_of_month_and_month_free( const ecf::Calendar&) const;

   bool week_day_matches(int) const;
   bool last_week_day_of_month_matches(const ecf::Calendar& ) const;
   bool day_of_month_matches(int,const ecf::Calendar& c) const;
   bool month_matches(int) const;

   boost::gregorian::date  next_date(const ecf::Calendar& calendar) const;

   TimeSeries       timeSeries_;
   std::vector<int> weekDays_;
   std::vector<int> last_week_days_of_month_;
   std::vector<int> daysOfMonth_;
   std::vector<int> months_;
   unsigned int     w_{0};                // NOT implemented, weekday nearest the given day of the month 15W
   unsigned int     state_change_no_{0};  // *not* persisted, only used on server side
   bool             last_day_of_month_{false};
   bool             free_{false};         // persisted for use by why() on client side

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version );
};
}

#endif /* CRONATTR_HPP_ */
