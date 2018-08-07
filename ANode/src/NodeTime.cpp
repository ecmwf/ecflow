/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #281 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <cassert>
#include <boost/foreach.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "SuiteChanged.hpp"

using namespace ecf;
using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

///////////////////////////////////////////////////////////////////////////////////////////

void Node::do_requeue_time_attrs(bool reset_next_time_slot, bool reset_relative_duartion)
{
   // must be done before the re-queue
   if (reset_relative_duartion) {
      for(size_t i = 0; i < crons_.size();    i++)  {   crons_[i].resetRelativeDuration(); }
      for(size_t i = 0; i < todays_.size(); i++)  { todays_[i].resetRelativeDuration();}
      for(size_t i = 0; i < times_.size();  i++)  {  times_[i].resetRelativeDuration(); }
   }

   /// If a job takes longer than it slots, then that slot is missed, and next slot is used
   /// Note we do *NOT* reset for requeue as we want to advance to the next time slot
   /// *NOTE* Update calendar will *free* time dependencies *even* time series. They rely
   /// on this function to clear the time dependencies so they *HOLD* the task.
   const Calendar& calendar = suite()->calendar();
   for(size_t i = 0; i < todays_.size(); i++)  { todays_[i].requeue(calendar,reset_next_time_slot);}
   for(size_t i = 0; i < times_.size(); i++)   {  times_[i].requeue(calendar,reset_next_time_slot);}
   for(size_t i = 0; i < crons_.size(); i++)     {    crons_[i].requeue(calendar,reset_next_time_slot);}

   for(size_t i = 0; i < days_.size(); i++)      {  days_[i].clearFree(); }
   for(size_t i = 0; i < dates_.size(); i++)     { dates_[i].clearFree(); }
}

void Node::calendar_changed_timeattrs(const ecf::Calendar& c )
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

   if (days_.empty() && dates_.empty() ) {

      // No Day or Date, If time matches  calendarChanged(c) will free time dependencies
      for(size_t i = 0; i < times_.size(); i++)  {  times_[i].calendarChanged(c); }
      for(size_t i = 0; i < todays_.size(); i++) { todays_[i].calendarChanged(c); }
      for(size_t i = 0; i < crons_.size(); i++)    {    crons_[i].calendarChanged(c); }
   }
   else {

      bool at_least_one_day_free = false;
      for(size_t i = 0; i < days_.size(); i++){
         days_[i].calendarChanged(c);
         if (!at_least_one_day_free) at_least_one_day_free = days_[i].isFree(c);
      }

      bool at_least_one_date_free = false;
      for(size_t i = 0; i < dates_.size(); i++) {
         dates_[i].calendarChanged(c);
         if (!at_least_one_date_free) at_least_one_date_free = dates_[i].isFree(c);
      }

      if ( at_least_one_day_free || at_least_one_date_free)  {
         for(size_t i = 0; i < times_.size(); i++)  {  times_[i].calendarChanged(c); }
         for(size_t i = 0; i < todays_.size(); i++) { todays_[i].calendarChanged(c); }
         for(size_t i = 0; i < crons_.size(); i++)    {    crons_[i].calendarChanged(c); }
      }
   }
}

void Node::markHybridTimeDependentsAsComplete()
{
   // If hybrid clock and then we may have day/date/cron time dependencies
   // which mean that node will be stuck in the QUEUED state, i.e since the
   // date/day does not change with the hybrid clock.
   // hence Mark these Nodes as complete
   const Calendar& calendar = suite()->calendar();
   if (state() != NState::COMPLETE && calendar.hybrid()) {
      if ( !dates_.empty() || !days_.empty() || !crons_.empty()) {

         int noOfTimeDependencies = 0;
         if (!dates_.empty())    noOfTimeDependencies++;
         if (!days_.empty())     noOfTimeDependencies++;
         if (!crons_.empty())    noOfTimeDependencies++;

         bool oneDateIsFree = false;
         bool oneDayIsFree = false;
         bool oneCronIsFree = false;

         for(size_t i=0;i<dates_.size();i++) { if (dates_[i].validForHybrid(calendar)) { if (noOfTimeDependencies == 1) { setStateOnly(NState::QUEUED); return;}oneDateIsFree = true;break;}}
         for(size_t i=0;i<days_.size();i++)  { if (days_[i].validForHybrid(calendar))  { if (noOfTimeDependencies == 1) { setStateOnly(NState::QUEUED); return;}oneDayIsFree = true;break;}}
         for(size_t i=0;i<crons_.size();i++) { if (crons_[i].validForHybrid(calendar)) { if (noOfTimeDependencies == 1) { setStateOnly(NState::QUEUED); return;}oneCronIsFree = true;break;}}

         if ( oneDateIsFree || oneDayIsFree ||  oneCronIsFree) {
            if ( noOfTimeDependencies > 1 ) {
               // when we have multiple time dependencies they results *MUST* be anded for the node to be free.
               if (!dates_.empty() && !oneDateIsFree) { setStateOnly(NState::COMPLETE); return;}
               if (!days_.empty()  && !oneDayIsFree)  { setStateOnly(NState::COMPLETE); return;}
               if (!crons_.empty() && !oneCronIsFree) { setStateOnly(NState::COMPLETE); return;}

               // We will only get here, if we have a multiple time dependencies any there is one free in each category
               setStateOnly(NState::QUEUED);
               return;
            }
         }

         setStateOnly(NState::COMPLETE);
      }
   }
}

// #define DEBUG_REQUEUE 1
bool Node::testTimeDependenciesForRequeue() const
{
   // Check for re-queue required for all time related attributes
   const Calendar& calendar = suite()->calendar();

#ifdef DEBUG_REQUEUE
   LogToCout logtocout;
   LOG(Log::DBG,"TimeDepAttrs::testTimeDependenciesForRequeue() " << debugNodePath() << " calendar " << calendar.toString());
#endif


   // When we have a mixture of cron *with* other time based attributes
   // The cron *takes* priority.  Crons should always return true, for checkForRequeue
   BOOST_FOREACH(const CronAttr& cron, crons_ ) {
      if (cron.checkForRequeue(calendar)) {  // will always return true
#ifdef DEBUG_REQUEUE
         LOG(Log::DBG,"   TimeDepAttrs::testTimeDependenciesForRequeue() " << debugNodePath() << " for cron");
#endif
         return true;
      }
   }


   if (!times_.empty()) {
      TimeSlot the_min,the_max; // Needs to handle multiple single slot time attributes
      BOOST_FOREACH(const ecf::TimeAttr& time, times_) { time.min_max_time_slots(the_min,the_max);}
      BOOST_FOREACH(const ecf::TimeAttr& time, times_) {
         if (time.checkForRequeue(calendar,the_min,the_max)) {
#ifdef DEBUG_REQUEUE
            LOG(Log::DBG,"   TimeDepAttrs::testTimeDependenciesForRequeue() " << debugNodePath() << " for time " << time.toString());
#endif
            return true;
         }
      }
   }


   if (!todays_.empty()) {
      TimeSlot the_min,the_max; // Needs to handle multiple single slot today attributes
      BOOST_FOREACH(const ecf::TodayAttr& today,todays_)  { today.min_max_time_slots(the_min,the_max);}
      BOOST_FOREACH(const ecf::TodayAttr& today,todays_) {
         if (today.checkForRequeue(calendar,the_min,the_max)) {
#ifdef DEBUG_REQUEUE
            LOG(Log::DBG,"   TimeDepAttrs::testTimeDependenciesForRequeue() " << debugNodePath() << " for today " << today.toString());
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
         LOG(Log::DBG,"   TimeDepAttrs::testTimeDependenciesForRequeue() " << debugNodePath() << " for date " << date.toString());
#endif
         return true;
      }
   }

   BOOST_FOREACH(const DayAttr& day, days_ ) {
      if (day.checkForRequeue(calendar)) {
#ifdef DEBUG_REQUEUE
         LOG(Log::DBG,"   TimeDepAttrs::testTimeDependenciesForRequeue() " << debugNodePath() << " for day " << day.toString());
#endif
         return true;
      }
   }

#ifdef DEBUG_REQUEUE
   LOG(Log::DBG,"   TimeDepAttrs::testTimeDependenciesForRequeue() " << debugNodePath() << " HOLDING ");
#endif
   return false;
}


void Node::miss_next_time_slot()
{
   // Why do we need to set NO_REQUE_IF_SINGLE_TIME_DEP flag ?
   // This is required when we have time based attributes, which we want to miss.
   //    time 10:00
   //    time 12:00
   // Essentially this avoids an automated job run, *IF* the job was run manually for a given time slot.
   // If we call this function before 10:00, we want to miss the next time slot (i.e. 10:00)
   // and want to *requeue*, for 12:00 time slot. However at re-queue, we need to ensure
   // we do *not* reset the 10:00 time slot. hence by setting NO_REQUE_IF_SINGLE_TIME_DEP
   // we allow requeue to query this flag, and hence avoid resetting the time based attribute
   // Note: requeue will *always* clear NO_REQUE_IF_SINGLE_TIME_DEP afterwards.
   //
   // In the case above when we reach the last time slot, there is *NO* automatic requeue, and
   // hence, *no* clearing of NO_REQUE_IF_SINGLE_TIME_DEP flag.
   // This will then be up to any top level parent that has a Repeat/cron to force a requeue
   // when all the children are complete. *or* user does a manual re-queue
   //
   // Additionally if the job *aborts*, we clear NO_REQUE_IF_SINGLE_TIME_DEP if it was set.
   // Otherwise if manually run again, we will miss further time slots.
   if ( has_time_dependencies()) {

      /// Handle abort
      /// The flag: NO_REQUE_IF_SINGLE_TIME_DEP is *only* set when doing an interactive force complete or run command.
      /// What happens if the job aborts during the run command ?
      ///     time 10:00
      ///     time 11:00
      /// If at 9.00am we used the run command, we want to miss the 10:00 time slot.
      /// However if the run at 9.00 fails, and we run again, we also miss 11:00 time slot.
      /// During the run the flag is still set.
      /// Hence *ONLY* miss the next time slot *IF* Flag::NO_REQUE_IF_SINGLE_TIME_DEP is NOT set
      if (!flag().is_set(Flag::NO_REQUE_IF_SINGLE_TIME_DEP)) {

         SuiteChanged0 changed(shared_from_this());
         flag().set(Flag::NO_REQUE_IF_SINGLE_TIME_DEP);

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
          for(size_t i=0;i<times_.size();i++) {
             if (times_[i].time_series().is_valid()) {
                times_[i].miss_next_time_slot();
                break;
             }
          }
          for(size_t i=0;i<todays_.size();i++){
             if (todays_[i].time_series().is_valid()) {
                todays_[i].miss_next_time_slot();
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
   }
}

void Node::freeHoldingDateDependencies()
{
   // Multiple time dependencies of the same type are *ORed*
   // Multiple time dependencies of different types are *ANDed*
   //
   // Hence since we have multiple time dependencies of the same
   // type here, we need free only one of them
   const Calendar& calendar = suite()->calendar();
   for(size_t i=0;i<dates_.size();i++)    {
      if (!dates_[i].isFree(calendar))  {
         dates_[i].setFree();
         break;
      }
   }
}

void Node::freeHoldingTimeDependencies()
{
   // Multiple time dependencies of the same type are *ORed*
   // Multiple time dependencies of different types are *ANDed*
   //
   // If we have multiple time dependencies of different types
   // we need only free one in each category
   const Calendar& calendar = suite()->calendar();
   for(size_t i=0;i<times_.size();i++)  {
      if (!times_[i].isFree(calendar))  {
         times_[i].setFree();
         times_[i].miss_next_time_slot();
         break;
      }
   }
   for(size_t i=0;i<todays_.size();i++)  {
      if (!todays_[i].isFree(calendar)) {
         todays_[i].setFree();
         todays_[i].miss_next_time_slot();
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

bool Node::has_time_dependencies() const
{
   if (!times_.empty())   return true;
   if (!todays_.empty())  return true;
   if (!crons_.empty())     return true;
   if (!dates_.empty())     return true;
   if (!days_.empty())      return true;
   return false;
}


bool Node::timeDependenciesFree() const
{
   int noOfTimeDependencies = 0;
   if (!times_.empty())  noOfTimeDependencies++;
   if (!todays_.empty()) noOfTimeDependencies++;
   if (!dates_.empty())    noOfTimeDependencies++;
   if (!days_.empty())     noOfTimeDependencies++;
   if (!crons_.empty())    noOfTimeDependencies++;

   // if no time dependencies we are free
   if (noOfTimeDependencies == 0) return true;

   bool oneDateIsFree = false;
   bool oneDayIsFree = false;
   bool oneTodayIsFree = false;
   bool oneTimeIsFree = false;
   bool oneCronIsFree = false;

   const Calendar& calendar = suite()->calendar();
   for(size_t i=0;i<times_.size();i++){ if (times_[i].isFree(calendar)){if ( noOfTimeDependencies == 1) return true;oneTimeIsFree = true;break;}}
   for(size_t i=0;i<crons_.size();i++)  { if (crons_[i].isFree(calendar))  {if ( noOfTimeDependencies == 1) return true;oneCronIsFree = true;break;}}
   for(size_t i=0;i<dates_.size();i++)  { if (dates_[i].isFree(calendar))  {if ( noOfTimeDependencies == 1) return true;oneDateIsFree = true;break;}}
   for(size_t i=0;i<days_.size();i++)   { if (days_[i].isFree(calendar))   {if ( noOfTimeDependencies == 1) return true;oneDayIsFree = true;break;}}

   if (!todays_.empty()) {
      // : single Today: (single-time)   is free, if calendar time >= today_time
      // : single Today: (range)         is free, if calendar time == (one of the time ranges)
      // : multi Today : (single | range)is free, if calendar time == (one of the time ranges | tody_time)
      if (todays_.size() == 1 ) {
         // Single Today Attribute: could be single slot or range
         if (todays_[0].isFree(calendar)) { if ( noOfTimeDependencies == 1) return true;oneTodayIsFree = true;}
      }
      else {
         // Multiple Today Attributes, each could single, or range
         for(size_t i=0;i<todays_.size();i++) {
            if (todays_[i].isFreeMultipleContext(calendar)) {if (noOfTimeDependencies == 1) return true;oneTodayIsFree = true;break;}
         }
      }
   }


   if ( oneDateIsFree || oneDayIsFree || oneTodayIsFree ||  oneTimeIsFree || oneCronIsFree) {
      if ( noOfTimeDependencies > 1 ) {
         // *When* we have multiple time dependencies of *different types* then the results
         // *MUST* be anded for the node to be free.
         if (!dates_.empty() && !oneDateIsFree) return false;
         if (!days_.empty() && !oneDayIsFree) return false;
         if (!todays_.empty() && !oneTodayIsFree) return false;
         if (!times_.empty() && !oneTimeIsFree) return false;
         if (!crons_.empty() && !oneCronIsFree) return false;

         // We will only get here, if we have a multiple time dependencies and they are free
         return true;
      }
   }

   return false;
}

bool Node::time_today_cron_is_free() const
{
   if (!times_.empty() || !todays_.empty() || !crons_.empty()) {

      int noOfTimeDependencies = 0;
      if (!times_.empty())  noOfTimeDependencies++;
      if (!todays_.empty()) noOfTimeDependencies++;
      if (!crons_.empty())    noOfTimeDependencies++;

      bool oneTodayIsFree = false;
      bool oneTimeIsFree = false;
      bool oneCronIsFree = false;

      const Calendar& calendar = suite()->calendar();
      for(size_t i=0;i<times_.size();i++)  { if (times_[i].isFree(calendar))  {if ( noOfTimeDependencies == 1) return true;oneTimeIsFree = true;break;}}
      for(size_t i=0;i<crons_.size();i++)    { if (crons_[i].isFree(calendar))    {if ( noOfTimeDependencies == 1) return true;oneCronIsFree = true;break;}}

      if (!todays_.empty()) {
         // : single Today: (single-time)   is free, if calendar time >= today_time
         // : single Today: (range)         is free, if calendar time == (one of the time ranges)
         // : multi Today : (single | range)is free, if calendar time == (one of the time ranges | tody_time)
         if (todays_.size() == 1 ) {
            // Single Today Attribute: could be single slot or range
            if (todays_[0].isFree(calendar)) { if ( noOfTimeDependencies == 1) return true;oneTodayIsFree = true;}
         }
         else {
            // Multiple Today Attributes, each could single, or range
            for(size_t i=0;i<todays_.size();i++) {
               if (todays_[i].isFreeMultipleContext(calendar)) {if ( noOfTimeDependencies == 1) return true;oneTodayIsFree = true;break;}
            }
         }
      }


      if ( oneTodayIsFree ||  oneTimeIsFree || oneCronIsFree) {
         if ( noOfTimeDependencies > 1 ) {
            // *When* we have multiple time dependencies of *different types* then the results
            // *MUST* be anded for the node to be free.
            if (!todays_.empty() && !oneTodayIsFree) return false;
            if (!times_.empty() && !oneTimeIsFree) return false;
            if (!crons_.empty() && !oneCronIsFree) return false;

            // We will only get here, if we have a multiple time dependencies and they are free
            return true;
         }
      }
   }

   return false;
}

void Node::get_time_resolution_for_simulation(boost::posix_time::time_duration& resol) const
{
   for(size_t i = 0; i < times_.size(); i++){
      const TimeSeries& time_series = times_[i].time_series();
      if (time_series.start().minute() != 0 )  { resol = minutes(1); return; }
      if (time_series.hasIncrement()) {
         if (time_series.finish().minute() != 0 ) { resol = minutes(1); return; }
         if (time_series.incr().minute() != 0 )   { resol = minutes(1); return; }
      }
   }

   for(size_t i = 0; i < todays_.size(); i++){
      const TimeSeries& time_series = todays_[i].time_series();
      if (time_series.start().minute() != 0 )     { resol = minutes(1); return; }
      if (time_series.hasIncrement()) {
         if (time_series.finish().minute() != 0 ) { resol = minutes(1); return; }
         if (time_series.incr().minute() != 0 )   { resol = minutes(1); return; }
      }
   }

   for(size_t i = 0; i < crons_.size(); i++){
      const TimeSeries& time_series = crons_[i].time_series();
      if (time_series.start().minute() != 0 )     { resol = minutes(1); return; }
      if (time_series.hasIncrement()) {
         if (time_series.finish().minute() != 0 ) { resol = minutes(1); return; }
         if (time_series.incr().minute() != 0 )   { resol = minutes(1); return; }
      }
   }
}

void Node::get_max_simulation_duration(boost::posix_time::time_duration& duration) const
{
   // don't override a higher value of duration
   if ((!times_.empty() || !todays_.empty()) && duration < hours(24)) duration = hours(24); // day
   if (!days_.empty()  && duration < hours(168))     duration = hours(168);                     // week
   if (!dates_.empty() && duration < hours(24*7*31)) duration = hours(24*7*31);                 // month
   if (!crons_.empty())  duration = hours(8760);                                                // year
   if (!repeat_.empty()) duration = hours(8760);                                                // year
}
