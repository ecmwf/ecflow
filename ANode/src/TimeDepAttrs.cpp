/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #281 $
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <assert.h>
#include <boost/foreach.hpp>

#include "Defs.hpp"
#include "Suite.hpp"

#include "TimeDepAttrs.hpp"
#include "Str.hpp"
#include "Log.hpp"
#include "Ecf.hpp"
#include "Memento.hpp"

using namespace ecf;
using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

///////////////////////////////////////////////////////////////////////////////////////////

void TimeDepAttrs::begin()
{
   // Let time base attributes use, relative duration if applicable
   // reset requires calendar, to update next time slot, which is used in why command
   const Calendar& calendar = node_->suite()->calendar();
   for(size_t i = 0; i < todayVec_.size(); i++)  { todayVec_[i].reset(calendar);}
   for(size_t i = 0; i < timeVec_.size(); i++)   {  timeVec_[i].reset(calendar);}
   for(size_t i = 0; i < crons_.size(); i++)     {    crons_[i].reset(calendar);}

   for(size_t i = 0; i < days_.size(); i++)      {  days_[i].clearFree(); }
   for(size_t i = 0; i < dates_.size(); i++)     { dates_[i].clearFree(); }
}

void TimeDepAttrs::requeue(bool reset_next_time_slot) {

   /// If a job takes longer than it slots, then that slot is missed, and next slot is used
   /// Note we do *NOT* reset for requeue as we want to advance to the next time slot
   /// *NOTE* Update calendar will *free* time dependencies *even* time series. They rely
   /// on this function to clear the time dependencies so they *HOLD* the task.
   const Calendar& calendar = node_->suite()->calendar();
   for(size_t i = 0; i < todayVec_.size(); i++)  { todayVec_[i].requeue(calendar,reset_next_time_slot);}
   for(size_t i = 0; i < timeVec_.size(); i++)   {  timeVec_[i].requeue(calendar,reset_next_time_slot);}
   for(size_t i = 0; i < crons_.size(); i++)     {    crons_[i].requeue(calendar,reset_next_time_slot);}

   for(size_t i = 0; i < days_.size(); i++)      {  days_[i].clearFree(); }
   for(size_t i = 0; i < dates_.size(); i++)     { dates_[i].clearFree(); }
}

void TimeDepAttrs::calendarChanged(const ecf::Calendar& c )
{
   // For time/today/cron attributes if the time is free, it *remains* free until re-queued
   // However if we have day/date dependencies, that do NOT match, then we should *NOT* free
   // any time/today/cron attributes.
   //
   //   task t
   //     day Monday
   //     time 10:00
   //
   // Hence if we are on Sunday we do *NOT* want to free the time on SUNDAY
   // (Otherwise we will end up running the task at Monday Midnight
   //  and not Monday at 10.00)
   //
   bool have_day = false;
   bool at_least_one_day_free = false;
   for(size_t i = 0; i < days_.size(); i++){
      have_day = true;
      days_[i].calendarChanged(c);
      if (!at_least_one_day_free) at_least_one_day_free = days_[i].isFree(c);
   }

   bool have_date = false;
   bool at_least_one_date_free = false;
   for(size_t i = 0; i < dates_.size(); i++) {
      have_date = true;
      dates_[i].calendarChanged(c);
      if (!at_least_one_date_free) at_least_one_date_free = dates_[i].isFree(c);
   }

   if (have_day || have_date ) {
      if ( at_least_one_day_free || at_least_one_date_free)  {
         for(size_t i = 0; i < crons_.size(); i++)    {    crons_[i].calendarChanged(c); }
         for(size_t i = 0; i < todayVec_.size(); i++) { todayVec_[i].calendarChanged(c); }
         for(size_t i = 0; i < timeVec_.size(); i++)  {  timeVec_[i].calendarChanged(c); }
      }
   }
   else {
      // No Day or Date, If time matches  calendarChanged(c) will free time dependencies
      for(size_t i = 0; i < crons_.size(); i++)    {    crons_[i].calendarChanged(c); }
      for(size_t i = 0; i < todayVec_.size(); i++) { todayVec_[i].calendarChanged(c); }
      for(size_t i = 0; i < timeVec_.size(); i++)  {  timeVec_[i].calendarChanged(c); }
   }
}

void TimeDepAttrs::markHybridTimeDependentsAsComplete()
{
   // If hybrid clock and then we may have day/date/cron time dependencies
   // which mean that node will be stuck in the QUEUED state, i.e since the
   // date/day does not change with the hybrid clock.
   // hence Mark these Nodes as complete
   const Calendar& calendar = node_->suite()->calendar();
   if (node_->state() != NState::COMPLETE && calendar.hybrid()) {
      if ( !dates_.empty() || !days_.empty() || !crons_.empty()) {

         int noOfTimeDependencies = 0;
         if (!dates_.empty())    noOfTimeDependencies++;
         if (!days_.empty())     noOfTimeDependencies++;
         if (!crons_.empty())    noOfTimeDependencies++;

         bool oneDateIsFree = false;
         bool oneDayIsFree = false;
         bool oneCronIsFree = false;

         for(size_t i=0;i<dates_.size();i++) { if (dates_[i].validForHybrid(calendar)) { if (noOfTimeDependencies == 1) { node_->setStateOnly(NState::QUEUED); return;}oneDateIsFree = true;break;}}
         for(size_t i=0;i<days_.size();i++)  { if (days_[i].validForHybrid(calendar))  { if (noOfTimeDependencies == 1) { node_->setStateOnly(NState::QUEUED); return;}oneDayIsFree = true;break;}}
         for(size_t i=0;i<crons_.size();i++) { if (crons_[i].validForHybrid(calendar)) { if (noOfTimeDependencies == 1) { node_->setStateOnly(NState::QUEUED); return;}oneCronIsFree = true;break;}}

         if ( oneDateIsFree || oneDayIsFree ||  oneCronIsFree) {
            if ( noOfTimeDependencies > 1 ) {
               // when we have multiple time dependencies they results *MUST* be anded for the node to be free.
               if (!dates_.empty() && !oneDateIsFree) { node_->setStateOnly(NState::COMPLETE); return;}
               if (!days_.empty()  && !oneDayIsFree)  { node_->setStateOnly(NState::COMPLETE); return;}
               if (!crons_.empty() && !oneCronIsFree) { node_->setStateOnly(NState::COMPLETE); return;}

               // We will only get here, if we have a multiple time dependencies any there is one free in each category
               node_->setStateOnly(NState::QUEUED);
               return;
            }
         }

         node_->setStateOnly(NState::COMPLETE);
      }
   }
}

void TimeDepAttrs::resetRelativeDuration()
{
   for(size_t i = 0; i < crons_.size();    i++)  {   crons_[i].resetRelativeDuration(); }
   for(size_t i = 0; i < todayVec_.size(); i++)  { todayVec_[i].resetRelativeDuration();}
   for(size_t i = 0; i < timeVec_.size();  i++)  {  timeVec_[i].resetRelativeDuration(); }
}

// #define DEBUG_REQUEUE 1
bool TimeDepAttrs::testTimeDependenciesForRequeue() const
{
   // Check for re-queue required for all time related attributes
   const Calendar& calendar = node_->suite()->calendar();

#ifdef DEBUG_REQUEUE
   LogToCout logtocout;
   LOG(Log::DBG,"TimeDepAttrs::testTimeDependenciesForRequeue() " << node_->debugNodePath() << " calendar " << calendar.toString());
#endif


   // When we have a mixture of cron *with* other time based attributes
   // The cron *takes* priority.  Crons should always return true, for checkForRequeue
   BOOST_FOREACH(const CronAttr& cron, crons_ ) {
      if (cron.checkForRequeue(calendar)) {  // will always return true
#ifdef DEBUG_REQUEUE
         LOG(Log::DBG,"   TimeDepAttrs::testTimeDependenciesForRequeue() " << node_->debugNodePath() << " for cron");
#endif
         return true;
      }
   }


   if (!timeVec_.empty()) {
      TimeSlot the_min,the_max; // Needs to handle multiple single slot time attributes
      BOOST_FOREACH(const ecf::TimeAttr& time, timeVec_) { time.min_max_time_slots(the_min,the_max);}
      BOOST_FOREACH(const ecf::TimeAttr& time, timeVec_) {
         if (time.checkForRequeue(calendar,the_min,the_max)) {
#ifdef DEBUG_REQUEUE
            LOG(Log::DBG,"   TimeDepAttrs::testTimeDependenciesForRequeue() " << node_->debugNodePath() << " for time " << time.toString());
#endif
            return true;
         }
      }
   }


   if (!todayVec_.empty()) {
      TimeSlot the_min,the_max; // Needs to handle multiple single slot today attributes
      BOOST_FOREACH(const ecf::TodayAttr& today,todayVec_)  { today.min_max_time_slots(the_min,the_max);}
      BOOST_FOREACH(const ecf::TodayAttr& today,todayVec_) {
         if (today.checkForRequeue(calendar,the_min,the_max)) {
#ifdef DEBUG_REQUEUE
            LOG(Log::DBG,"   TimeDepAttrs::testTimeDependenciesForRequeue() " << node_->debugNodePath() << " for today " << today.toString());
#endif
            return true;;
         }
      }
   }


   // **********************************************************************
   // If we get here there are **NO** time/today/cron dependencies which are free
   // We now need to determine if this node has a future time dependency which
   // should re-queue this node
   // *********************************************************************
   BOOST_FOREACH(const DateAttr& date, dates_ ) {
      if (date.checkForRequeue(calendar)) {
#ifdef DEBUG_REQUEUE
         LOG(Log::DBG,"   TimeDepAttrs::testTimeDependenciesForRequeue() " << node_->debugNodePath() << " for date " << date.toString());
#endif
         return true;
      }
   }

   BOOST_FOREACH(const DayAttr& day, days_ ) {
      if (day.checkForRequeue(calendar)) {
#ifdef DEBUG_REQUEUE
         LOG(Log::DBG,"   TimeDepAttrs::testTimeDependenciesForRequeue() " << node_->debugNodePath() << " for day " << day.toString());
#endif
         return true;
      }
   }

#ifdef DEBUG_REQUEUE
   LOG(Log::DBG,"   TimeDepAttrs::testTimeDependenciesForRequeue() " << node_->debugNodePath() << " HOLDING ");
#endif
   return false;
}


void TimeDepAttrs::miss_next_time_slot()
{
   // Note: when we have multiple time dependencies.
   // We need find valid next time dependency:
   //   time 10:00
   //   time 11:00
   //   time 12:00 14:00 00:30
   // Also we could have a mix:
   //   time  10:00
   //   today 10:30
   //   time 11:00
   //   time 12:00 14:00 00:30

   // for the moment assume, they have been added sequentially,
   // hence only first non expired time is updated to miss next time slot
   for(size_t i=0;i<timeVec_.size();i++) {
      if (timeVec_[i].time_series().is_valid()) {
         timeVec_[i].miss_next_time_slot();
         break;
      }
   }
   for(size_t i=0;i<todayVec_.size();i++){
      if (todayVec_[i].time_series().is_valid()) {
         todayVec_[i].miss_next_time_slot();
         break;
      }
   }
   for(size_t i=0;i<crons_.size();i++) {
      if (crons_[i].time_series().is_valid()) {
         crons_[i].miss_next_time_slot();
         break;
      }
   }
}


void TimeDepAttrs::freeHoldingDateDependencies()
{
   // Multiple time dependencies of the same type are *ORed*
   // Multiple time dependencies of different types are *ANDed*
   //
   // Hence since we have multiple time dependencies of the same
   // type here, we need free only one of them
   const Calendar& calendar = node_->suite()->calendar();
   for(size_t i=0;i<dates_.size();i++)    {
      if (!dates_[i].isFree(calendar))  {
         dates_[i].setFree();
         break;
      }
   }
}

void TimeDepAttrs::freeHoldingTimeDependencies()
{
   // Multiple time dependencies of the same type are *ORed*
   // Multiple time dependencies of different types are *ANDed*
   //
   // If we have multiple time dependencies of different types
   // we need only free one in each category
   const Calendar& calendar = node_->suite()->calendar();
   for(size_t i=0;i<timeVec_.size();i++)  {
      if (!timeVec_[i].isFree(calendar))  {
         timeVec_[i].setFree();
         timeVec_[i].miss_next_time_slot();
         break;
      }
   }
   for(size_t i=0;i<todayVec_.size();i++)  {
      if (!todayVec_[i].isFree(calendar)) {
         todayVec_[i].setFree();
         todayVec_[i].miss_next_time_slot();
         break;
      }
   }
   for(size_t i=0;i<days_.size();i++)  {
      if (!days_[i].isFree(calendar)) {
         days_[i].setFree();
         break;
      }
   }
   for(size_t i=0;i<crons_.size();i++)  {
      if (!crons_[i].isFree(calendar))  {
         crons_[i].setFree();
         crons_[i].miss_next_time_slot();
         break;
      }
   }
}


bool TimeDepAttrs::timeDependenciesFree() const
{
   if (!timeVec_.empty() || !todayVec_.empty() || !dates_.empty() || !days_.empty() || !crons_.empty()) {

      int noOfTimeDependencies = 0;
      if (!timeVec_.empty())  noOfTimeDependencies++;
      if (!todayVec_.empty()) noOfTimeDependencies++;
      if (!dates_.empty())    noOfTimeDependencies++;
      if (!days_.empty())     noOfTimeDependencies++;
      if (!crons_.empty())    noOfTimeDependencies++;

      bool oneDateIsFree = false;
      bool oneDayIsFree = false;
      bool oneTodayIsFree = false;
      bool oneTimeIsFree = false;
      bool oneCronIsFree = false;

      const Calendar& calendar = node_->suite()->calendar();
      for(size_t i=0;i<timeVec_.size();i++)  { if (timeVec_[i].isFree(calendar))  {if ( noOfTimeDependencies == 1) return true;oneTimeIsFree = true;break;}}
      for(size_t i=0;i<crons_.size();i++)    { if (crons_[i].isFree(calendar))    {if ( noOfTimeDependencies == 1) return true;oneCronIsFree = true;break;}}
      for(size_t i=0;i<dates_.size();i++)    { if (dates_[i].isFree(calendar))    {if ( noOfTimeDependencies == 1) return true;oneDateIsFree = true;break;}}
      for(size_t i=0;i<days_.size();i++)     { if (days_[i].isFree(calendar))     {if ( noOfTimeDependencies == 1) return true;oneDayIsFree = true;break;}}

      if (!todayVec_.empty()) {
         // : single Today: (single-time)   is free, if calendar time >= today_time
         // : single Today: (range)         is free, if calendar time == (one of the time ranges)
         // : multi Today : (single | range)is free, if calendar time == (one of the time ranges | tody_time)
         if (todayVec_.size() == 1 ) {
            // Single Today Attribute: could be single slot or range
            if (todayVec_[0].isFree(calendar)) { if ( noOfTimeDependencies == 1) return true;oneTodayIsFree = true;}
         }
         else {
            // Multiple Today Attributes, each could single, or range
            for(size_t i=0;i<todayVec_.size();i++) {
               if (todayVec_[i].isFreeMultipleContext(calendar)) {if ( noOfTimeDependencies == 1) return true;oneTodayIsFree = true;break;}
            }
         }
      }


      if ( oneDateIsFree || oneDayIsFree || oneTodayIsFree ||  oneTimeIsFree || oneCronIsFree) {
         if ( noOfTimeDependencies > 1 ) {
            // *When* we have multiple time dependencies of *different types* then the results
            // *MUST* be anded for the node to be free.
            if (!dates_.empty() && !oneDateIsFree) return false;
            if (!days_.empty() && !oneDayIsFree) return false;
            if (!todayVec_.empty() && !oneTodayIsFree) return false;
            if (!timeVec_.empty() && !oneTimeIsFree) return false;
            if (!crons_.empty() && !oneCronIsFree) return false;

            // We will only get here, if we have a multiple time dependencies and they are free
            return true;
         }
      }
   }

   return false;
}

bool TimeDepAttrs::time_today_cron_is_free() const
{
   if (!timeVec_.empty() || !todayVec_.empty() || !crons_.empty()) {

      int noOfTimeDependencies = 0;
      if (!timeVec_.empty())  noOfTimeDependencies++;
      if (!todayVec_.empty()) noOfTimeDependencies++;
      if (!crons_.empty())    noOfTimeDependencies++;

      bool oneTodayIsFree = false;
      bool oneTimeIsFree = false;
      bool oneCronIsFree = false;

      const Calendar& calendar = node_->suite()->calendar();
      for(size_t i=0;i<timeVec_.size();i++)  { if (timeVec_[i].isFree(calendar))  {if ( noOfTimeDependencies == 1) return true;oneTimeIsFree = true;break;}}
      for(size_t i=0;i<crons_.size();i++)    { if (crons_[i].isFree(calendar))    {if ( noOfTimeDependencies == 1) return true;oneCronIsFree = true;break;}}

      if (!todayVec_.empty()) {
         // : single Today: (single-time)   is free, if calendar time >= today_time
         // : single Today: (range)         is free, if calendar time == (one of the time ranges)
         // : multi Today : (single | range)is free, if calendar time == (one of the time ranges | tody_time)
         if (todayVec_.size() == 1 ) {
            // Single Today Attribute: could be single slot or range
            if (todayVec_[0].isFree(calendar)) { if ( noOfTimeDependencies == 1) return true;oneTodayIsFree = true;}
         }
         else {
            // Multiple Today Attributes, each could single, or range
            for(size_t i=0;i<todayVec_.size();i++) {
               if (todayVec_[i].isFreeMultipleContext(calendar)) {if ( noOfTimeDependencies == 1) return true;oneTodayIsFree = true;break;}
            }
         }
      }


      if ( oneTodayIsFree ||  oneTimeIsFree || oneCronIsFree) {
         if ( noOfTimeDependencies > 1 ) {
            // *When* we have multiple time dependencies of *different types* then the results
            // *MUST* be anded for the node to be free.
            if (!todayVec_.empty() && !oneTodayIsFree) return false;
            if (!timeVec_.empty() && !oneTimeIsFree) return false;
            if (!crons_.empty() && !oneCronIsFree) return false;

            // We will only get here, if we have a multiple time dependencies and they are free
            return true;
         }
      }
   }

   return false;
}

std::ostream& TimeDepAttrs::print(std::ostream& os) const
{
   BOOST_FOREACH(const ecf::TimeAttr& t, timeVec_)  { t.print(os);    }
   BOOST_FOREACH(const ecf::TodayAttr& t,todayVec_) { t.print(os);    }
   BOOST_FOREACH(const DateAttr& date, dates_)      { date.print(os); }
   BOOST_FOREACH(const DayAttr& day, days_)         { day.print(os);  }
   BOOST_FOREACH(const CronAttr& cron, crons_)      { cron.print(os); }
   return os;
}

bool TimeDepAttrs::operator==(const TimeDepAttrs& rhs) const
{
   if (timeVec_.size() != rhs.timeVec_.size()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "TimeDepAttrs::operator==  (timeVec_.size() != rhs.timeVec_.size()) " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   for(unsigned i = 0; i < timeVec_.size(); ++i) {
      if (!(timeVec_[i] == rhs.timeVec_[i] )) {
#ifdef DEBUG
         if (Ecf::debug_equality()) {
            std::cout << "TimeDepAttrs::operator==  (!(timeVec_[i] == rhs.timeVec_[i] ))  " << node_->debugNodePath() << "\n";
         }
#endif
         return false;
      }
   }

   if (todayVec_.size() != rhs.todayVec_.size()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "TimeDepAttrs::operator==  (todayVec_.size() != rhs.todayVec_.size()) " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   for(unsigned i = 0; i < todayVec_.size(); ++i) {
      if (!(todayVec_[i] == rhs.todayVec_[i] )) {
#ifdef DEBUG
         if (Ecf::debug_equality()) {
            std::cout << "TimeDepAttrs::operator==  (!(todayVec_[i] == rhs.todayVec_[i] ))  " << node_->debugNodePath() << "\n";
         }
#endif
         return false;
      }
   }

   if (dates_.size() != rhs.dates_.size()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "TimeDepAttrs::operator==   (dates_.size() != rhs.dates_.size()) " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   for(unsigned i = 0; i < dates_.size(); ++i) {
      if (!(dates_[i] == rhs.dates_[i]) ) {
#ifdef DEBUG
         if (Ecf::debug_equality()) {
            std::cout << "TimeDepAttrs::operator==   (!(dates_[i] == rhs.dates_[i]) " << node_->debugNodePath() << "\n";
         }
#endif
         return false;
      }
   }

   if (days_.size() != rhs.days_.size()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "TimeDepAttrs::operator==   (days_.size() != rhs.days_.size()) " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   for(unsigned i = 0; i < days_.size(); ++i) {
      if (!(days_[i] == rhs.days_[i]) ) {
#ifdef DEBUG
         if (Ecf::debug_equality()) {
            std::cout << "TimeDepAttrs::operator==   (!(days_[i] == rhs.days_[i]) " << node_->debugNodePath() << "\n";
         }
#endif
         return false;
      }
   }

   if (crons_.size() != rhs.crons_.size()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "TimeDepAttrs::operator==   (crons_.size() != rhs.crons_.size()) " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   for(unsigned i = 0; i < crons_.size(); ++i) {
      if (!(crons_[i] == rhs.crons_[i]) ) {
#ifdef DEBUG
         if (Ecf::debug_equality()) {
            std::cout << "TimeDepAttrs::operator==   (!(crons_[i] == rhs.crons_[i]) " << node_->debugNodePath() << "\n";
         }
#endif
         return false;
      }
   }

   return true;
}


//#define DEBUG_WHY 1
void TimeDepAttrs::why(std::vector<std::string>& vec,const std::string& prefix) const
{
#ifdef DEBUG_WHY
   std::cout << "   TimeDepAttrs::why " << node_->debugNodePath() << " checking time dependencies\n";
#endif
   // postfix  = <attr-type dependent> <next run time > < optional current state>
   std::string postFix;
   const Calendar& c = node_->suite()->calendar();
   for(size_t i = 0; i < days_.size(); i++)    { postFix.clear(); if (days_[i].why(c,postFix))   { vec.push_back(prefix + postFix); }}
   for(size_t i = 0; i < dates_.size(); i++)   { postFix.clear(); if (dates_[i].why(c,postFix))  { vec.push_back(prefix + postFix); }}
   for(size_t i = 0; i < todayVec_.size(); i++){ postFix.clear(); if (todayVec_[i].why(c,postFix)){ vec.push_back(prefix + postFix); }}
   for(size_t i = 0; i < timeVec_.size(); i++) { postFix.clear(); if (timeVec_[i].why(c,postFix)) { vec.push_back(prefix + postFix); }}
   for(size_t i = 0; i < crons_.size(); i++)   { postFix.clear(); if (crons_[i].why(c,postFix))  { vec.push_back(prefix + postFix); }}

}

bool TimeDepAttrs::checkInvariants(std::string& errorMsg) const
{
   if (node_ == NULL) {
      errorMsg +="TimeDepAttrs::checkInvariants node_ not set";
      return false;
   }
   BOOST_FOREACH(const ecf::TimeAttr& t, timeVec_)  { if (!t.checkInvariants(errorMsg)) return false; }
   BOOST_FOREACH(const ecf::TodayAttr& t,todayVec_) { if (!t.checkInvariants(errorMsg)) return false; }
   BOOST_FOREACH(const CronAttr& cron, crons_ )     { if (!cron.checkInvariants(errorMsg)) return false; }
   return true;
}


void TimeDepAttrs::clear()
{
   timeVec_.clear();
   todayVec_.clear();
   dates_.clear();
   days_.clear();
   crons_.clear();
}


void TimeDepAttrs::addTime(const ecf::TimeAttr& t)
{
   timeVec_.push_back(t);
   node_->state_change_no_ = Ecf::incr_state_change_no();
}

void TimeDepAttrs::addToday(const ecf::TodayAttr& t)
{
   todayVec_.push_back(t);
   node_->state_change_no_ = Ecf::incr_state_change_no();
}

void TimeDepAttrs::addDate( const DateAttr& d)
{
   dates_.push_back( d );
   node_->state_change_no_ = Ecf::incr_state_change_no();
}

void TimeDepAttrs::addDay( const DayAttr& d)
{
   days_.push_back( d );
   node_->state_change_no_ = Ecf::incr_state_change_no();
}

void TimeDepAttrs::addCron( const CronAttr& d)
{
   if (d.time().isNULL()) {
      throw std::runtime_error("TimeDepAttrs::addCron: The cron is in-complete, no time specified");
   }
   if (d.time().hasIncrement() && !node_->repeat_.empty()) {
      std::stringstream ss;
      ss << "TimeDepAttrs::addCron: Node " << node_->absNodePath() << " already has a repeat. Inappropriate to add two looping structures at the same level\n";
      throw std::runtime_error(ss.str());
   }
   crons_.push_back( d );
   node_->state_change_no_ = Ecf::incr_state_change_no();
}


void TimeDepAttrs::deleteTime(const std::string& name )
{
   if (name.empty()) {
      timeVec_.clear();  // delete all
      node_->state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
      std::cout << "TimeDepAttrs::deleteTime\n";
#endif
      return;
   }
   TimeAttr attr( TimeSeries::create(name) ); // can throw if parse fails
   delete_time(attr);                         // can throw if search fails
}
void TimeDepAttrs::delete_time( const ecf::TimeAttr& attr )
{
   size_t theSize = timeVec_.size();
   for(size_t i = 0; i < theSize; i++) {
      // Dont use '==' since that compares additional state like makeFree_
      if (timeVec_[i].structureEquals(attr)) {
         timeVec_.erase( timeVec_.begin() + i );
         node_->state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
         std::cout << "TimeDepAttrs::delete_time\n";
#endif
         return;
      }
   }
   throw std::runtime_error("TimeDepAttrs::delete_time: Can not find time attribute: ");
}


void TimeDepAttrs::deleteToday(const std::string& name)
{
   if (name.empty()) {
      todayVec_.clear();
      node_->state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
      std::cout << "TimeDepAttrs::deleteToday\n";
#endif
      return;
   }

   TodayAttr attr( TimeSeries::create(name) ); // can throw if parse fails
   delete_today(attr);                         // can throw if search fails
}
void TimeDepAttrs::delete_today(const ecf::TodayAttr& attr)
{
   size_t theSize = todayVec_.size();
   for(size_t i = 0; i < theSize; i++) {
      // Dont use '==' since that compares additional state like makeFree_
      if (todayVec_[i].structureEquals(attr)) {
         todayVec_.erase( todayVec_.begin() + i );
         node_->state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
         std::cout << "TimeDepAttrs::delete_today\n";
#endif
         return;
      }
   }
   throw std::runtime_error("TimeDepAttrs::delete_today: Can not find today attribute: " + attr.toString());
}

void TimeDepAttrs::deleteDate(const std::string& name)
{
   if (name.empty()) {
      dates_.clear();
      node_->state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
      std::cout << "TimeDepAttrs::deleteDate\n";
#endif
      return;
   }

   DateAttr attr( DateAttr::create(name) ); // can throw if parse fails
   delete_date(attr);                       // can throw if search fails
}
void TimeDepAttrs::delete_date(const DateAttr& attr)
{
   for(size_t i = 0; i < dates_.size(); i++) {
      // Dont use '==' since that compares additional state like makeFree_
      if (attr.structureEquals(dates_[i]) ) {
         dates_.erase( dates_.begin() + i );
         node_->state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
         std::cout << "TimeDepAttrs::delete_date\n";
#endif
         return;
      }
   }
   throw std::runtime_error("TimeDepAttrs::delete_date: Can not find date attribute: " + attr.toString());
}


void TimeDepAttrs::deleteDay(const std::string& name)
{
   if (name.empty()) {
      days_.clear();
      node_->state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
      std::cout << "TimeDepAttrs::deleteDay\n";
#endif
      return;
   }

   DayAttr attr( DayAttr::create(name) ); // can throw if parse fails.
   delete_day(attr);                      // can throw if search fails
}
void TimeDepAttrs::delete_day(const DayAttr& attr)
{
   for(size_t i = 0; i < days_.size(); i++) {
      // Dont use '==' since that compares additional state like makeFree_
      if (attr.structureEquals(days_[i]) ) {
         days_.erase( days_.begin() + i );
         node_->state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
         std::cout << "TimeDepAttrs::delete_day\n";
#endif
         return;
      }
   }
   throw std::runtime_error("TimeDepAttrs::delete_day: Can not find day attribute: " + attr.toString());
}

void TimeDepAttrs::deleteCron(const std::string& name)
{
   if (name.empty()) {
      crons_.clear();
      node_->state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
      std::cout << "TimeDepAttrs::deleteCron\n";
#endif
      return;
   }

   CronAttr attr = CronAttr::create(name); // can throw if parse fails
   delete_cron(attr);                      // can throw if search fails
}

void TimeDepAttrs::delete_cron(const ecf::CronAttr& attr)
{
   for(size_t i = 0; i < crons_.size(); i++) {
      // Dont use '==' since that compares additional state like makeFree_
      if (attr.structureEquals(crons_[i]) ) {
         crons_.erase( crons_.begin() + i );
         node_->state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
         std::cout << "TimeDepAttrs::deleteCron\n";
#endif
         return ;
      }
   }
   throw std::runtime_error("TimeDepAttrs::delete_cron: Can not find cron attribute: " + attr.toString());
}

// =================================================================================


bool TimeDepAttrs::set_memento( const NodeTodayMemento* memento ,std::vector<ecf::Aspect::Type>& aspects) {

#ifdef DEBUG_MEMENTO
   std::cout << "TimeDepAttrs::set_memento(const NodeTodayMemento* memento) " << node_->debugNodePath() << "\n";
#endif

   for(size_t i = 0; i < todayVec_.size(); ++i) {
      // We need to ignore state changes in TodayAttr, (ie we don't use equality operator)
      // otherwise today will never compare
      if ( todayVec_[i].structureEquals(memento->attr_) ) {
         todayVec_[i] = memento->attr_;  // need to copy over time series state
         return true;
      }
   }
   return false;
}

bool TimeDepAttrs::set_memento( const NodeTimeMemento* memento,std::vector<ecf::Aspect::Type>& aspects ) {

#ifdef DEBUG_MEMENTO
   std::cout << "TimeDepAttrs::set_memento(const NodeTimeMemento* memento) " << node_->debugNodePath() << "\n";
#endif

   for(size_t i = 0; i < timeVec_.size(); ++i) {
      // We need to ignore state changes in TimeAttr, (ie we don't use equality operator)
      // otherwise time will never compare
      if ( timeVec_[i].structureEquals(memento->attr_) ) {
         timeVec_[i] = memento->attr_;    // need to copy over time series state
         return true;
      }
   }
   return false;
}

bool TimeDepAttrs::set_memento( const NodeCronMemento* memento,std::vector<ecf::Aspect::Type>& aspects ) {

#ifdef DEBUG_MEMENTO
   std::cout << "TimeDepAttrs::set_memento(const NodeCronMemento* memento) " << node_->debugNodePath() << "\n";
#endif

   for(size_t i = 0; i < crons_.size(); ++i) {
      // We need to ignore state changes (ie we don't use equality operator)
      // otherwise attributes will never compare
      if ( crons_[i].structureEquals(memento->attr_) ) {
         crons_[i] = memento->attr_;   // need to copy over time series state
         return true;
      }
   }
   return false;
}

bool TimeDepAttrs::set_memento( const NodeDayMemento* memento ,std::vector<ecf::Aspect::Type>& aspects) {

#ifdef DEBUG_MEMENTO
   std::cout << "TimeDepAttrs::set_memento(const NodeDayMemento* memento) " << node_->debugNodePath() << "\n";
#endif

   for(size_t i = 0; i < days_.size(); ++i) {
      // We need to ignore state changes (ie we don't use equality operator)
      // otherwise attributes will never compare
      if ( days_[i].structureEquals(memento->attr_) ) {
         if (memento->attr_.isSetFree()) days_[i].setFree();
         else                            days_[i].clearFree();
         return true;
      }
   }
   return false;
}

bool TimeDepAttrs::set_memento( const NodeDateMemento* memento,std::vector<ecf::Aspect::Type>& aspects ) {

#ifdef DEBUG_MEMENTO
   std::cout << "TimeDepAttrs::set_memento(const NodeDateMemento* memento) " << node_->debugNodePath() << "\n";
#endif

   for(size_t i = 0; i < dates_.size(); ++i) {
      // We need to ignore state changes (ie we don't use equality operator)
      // otherwise attributes will never compare
      if ( dates_[i].structureEquals(memento->attr_) ) {
         if (memento->attr_.isSetFree()) dates_[i].setFree();
         else                            dates_[i].clearFree();
         return true;
      }
   }
   return false;
}

