#ifndef TIME_DEP_ATTRS_HPP_
#define TIME_DEP_ATTRS_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #231 $
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
#include <ostream>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>         // no need to include <vector>

#include "TimeAttr.hpp"
#include "TodayAttr.hpp"
#include "DateAttr.hpp"
#include "DayAttr.hpp"
#include "CronAttr.hpp"
#include "NodeFwd.hpp"
#include "Aspect.hpp"

class TimeDepAttrs  {
public:
   TimeDepAttrs(Node* node) : node_(node) {}
   TimeDepAttrs(const TimeDepAttrs&); // users must call set_node() afterwards
   TimeDepAttrs() : node_(NULL) {}

   // needed by node serialisation
   void set_node(Node* n) { node_ = n; }

   void begin();

   /// If a job takes longer than it slots, then that slot is missed, and next slot is used
   /// Note we do *NOT* reset for requeue as we want to advance the valid time slots.
   /// *NOTE* Update calendar will *free* time dependencies *even* time series. They rely
   /// on this function to clear the time dependencies so they *HOLD* the task.
   ///
   /// If we have done an interactive run or complete, *dont* increment next_time_slot_
   void requeue(bool reset_next_time_slot);

   void miss_next_time_slot();
   void freeHoldingDateDependencies();
   void freeHoldingTimeDependencies();

   void calendarChanged(const ecf::Calendar& c);

   // standard functions: ==============================================
   std::ostream& print(std::ostream&) const;
   bool operator==(const TimeDepAttrs& rhs) const;
   bool checkInvariants(std::string& errorMsg) const;

   bool timeDependenciesFree() const;
   bool time_today_cron_is_free() const; /* used by viewer */

   // Access functions: ======================================================
   const std::vector<ecf::TimeAttr>&   timeVec()  const { return timeVec_; }
   const std::vector<ecf::TodayAttr>&  todayVec() const { return todayVec_; }
   const std::vector<DateAttr>&        dates()    const { return dates_; }
   const std::vector<DayAttr>&         days()     const { return days_; }
   const std::vector<ecf::CronAttr>&   crons()    const { return crons_; }

   // Add functions: ===============================================================
   void addTime( const ecf::TimeAttr& );
   void addToday( const ecf::TodayAttr& );
   void addDate( const DateAttr& );
   void addDay( const DayAttr& );
   void addCron( const ecf::CronAttr& );

   // Delete functions: can throw std::runtime_error ===================================
   // if name argument is empty, delete all attributes of that type
   // Can throw std::runtime_error of the attribute can not be found
   void deleteTime(const std::string& name );
   void delete_time( const ecf::TimeAttr&  );
   void deleteToday(const std::string& name);
   void delete_today(const ecf::TodayAttr&);
   void deleteDate(const std::string& name);
   void delete_date(const DateAttr&);
   void deleteDay(const std::string& name);
   void delete_day(const DayAttr&);
   void deleteCron(const std::string& name);
   void delete_cron(const ecf::CronAttr&);

   // Change functions: ================================================================
   /// returns true the change was made else false, Can throw std::runtime_error for parse errors

   // mementos functions:
   /// Collect all the state changes, so that only small subset is returned to client
   bool set_memento(const NodeTodayMemento* ,std::vector<ecf::Aspect::Type>& aspects);
   bool set_memento(const NodeTimeMemento* ,std::vector<ecf::Aspect::Type>& aspects);
   bool set_memento(const NodeDayMemento* ,std::vector<ecf::Aspect::Type>& aspects);
   bool set_memento(const NodeCronMemento* ,std::vector<ecf::Aspect::Type>& aspects);
   bool set_memento(const NodeDateMemento* ,std::vector<ecf::Aspect::Type>& aspects);

   void why(std::vector<std::string>& theReasonWhy,const std::string& prefix) const;
   bool testTimeDependenciesForRequeue() const;
   void resetRelativeDuration();


/// For use by python interface,
   std::vector<ecf::TimeAttr>::const_iterator time_begin() const { return timeVec_.begin();}
   std::vector<ecf::TimeAttr>::const_iterator time_end() const { return timeVec_.end();}
   std::vector<ecf::TodayAttr>::const_iterator today_begin() const { return todayVec_.begin();}
   std::vector<ecf::TodayAttr>::const_iterator today_end() const { return todayVec_.end();}
   std::vector<DateAttr>::const_iterator date_begin() const { return dates_.begin();}
   std::vector<DateAttr>::const_iterator date_end() const { return dates_.end();}
   std::vector<DayAttr>::const_iterator day_begin() const { return days_.begin();}
   std::vector<DayAttr>::const_iterator day_end() const { return days_.end();}
   std::vector<ecf::CronAttr>::const_iterator cron_begin() const { return crons_.begin();}
   std::vector<ecf::CronAttr>::const_iterator cron_end() const { return crons_.end();}

   void clear(); /// Clear *ALL* internal attributes

   /// Under the hybrid calendar some time dependent attributes may not be applicable
   /// i.e if day,date,cron attributes does correspond to 24 hours of today, then we
   /// need make them as complete.
   void markHybridTimeDependentsAsComplete();

   /// If we have just day/date then we have a resolution of 1 hour
   /// Otherwise if we have time/today/cron with minutes, then resolution is 1 minute
   void get_time_resolution_for_simulation(boost::posix_time::time_duration& resol) const;
   void get_max_simulation_duration(boost::posix_time::time_duration& duration) const;

private:
   Node*        node_; // *NOT* persisted must be set by the parent class

   std::vector<ecf::TimeAttr>  timeVec_;
   std::vector<ecf::TodayAttr> todayVec_;
   std::vector<DateAttr>       dates_;
   std::vector<DayAttr>        days_;
   std::vector<ecf::CronAttr>  crons_;

private:
   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & timeVec_;
      ar & todayVec_;
      ar & dates_;
      ar & days_;
      ar & crons_;
   }
};

#endif
