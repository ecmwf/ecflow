//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #65 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
//============================================================================

#include <boost/date_time/posix_time/time_formatters.hpp>  // requires boost date and time lib, for to_simple_string

#include "Calendar.hpp"
#include "CalendarUpdateParams.hpp"
#include "Log.hpp"
#include "Ecf.hpp"
#include "Extract.hpp"
#include "Serialization.hpp"
#include "cereal_boost_time.hpp"

using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

//#define DEBUG_CALENDAR 1;

namespace ecf {

Calendar::Calendar(){}

Calendar::Calendar(const Calendar& rhs)
{
	assign(rhs);
}

Calendar& Calendar::operator=( const Calendar& rhs)
{
	assign(rhs);
	return *this;
}

bool Calendar::operator==( const Calendar& rhs) const
{
	// Only used for testing the persistence of calendar
   // Note: We specifically ignore initLocalTime_  and lastTime_ since they are initialised with the current time.
   //       Otherwise For migration testing, it will fail

	if (ctype_ != rhs.ctype_) {
#ifdef DEBUG
      if (Ecf::debug_equality())  std::cout << "Calendar::operator== ctypes don't match\n";
#endif
	   return false;
	}
	if (initTime_ !=rhs.initTime_) {
#ifdef DEBUG
      if (Ecf::debug_equality())  std::cout << "Calendar::operator== initTime_ don't match\n";
#endif
	   return false;
	}
	if (suiteTime_ !=rhs.suiteTime_) {
#ifdef DEBUG
      if (Ecf::debug_equality())  std::cout << "Calendar::operator== suiteTime_  don't match\n";
#endif
	   return false;
	}
	if (duration_ !=rhs.duration_) {
#ifdef DEBUG
      if (Ecf::debug_equality())  std::cout << "Calendar::operator== duration_  don't match\n";
#endif
	   return false;
	}
	if (dayChanged_ !=rhs.dayChanged_) {
#ifdef DEBUG
      if (Ecf::debug_equality())  std::cout << "Calendar::operator== dayChanged_  don't match\n";
#endif
	   return false;
	}
	if (increment_ !=rhs.increment_) {
#ifdef DEBUG
      if (Ecf::debug_equality())  std::cout << "Calendar::operator== increment_  don't match\n";
#endif
	   return false;
	}

	return true;
}

void Calendar::assign( const Calendar& rhs)
{
	ctype_ = rhs.ctype_;
 	initTime_ = rhs.initTime_;
 	suiteTime_ = rhs.suiteTime_;
	duration_ = rhs.duration_;
 	dayChanged_ = rhs.dayChanged_;
  	initLocalTime_ =  rhs.initLocalTime_;
 	lastTime_ =  rhs.lastTime_;
	increment_ = rhs.increment_;

	day_of_week_ = rhs.day_of_week_;   // Cache
	day_of_year_ = rhs.day_of_year_;   // Cache
	day_of_month_ = rhs.day_of_month_; // Cache
	month_ = rhs.month_;               // Cache
	year_ = rhs.year_;                 // Cache
}

void Calendar::init(Clock_t clock)
{
   ctype_ = clock;
}

void Calendar::init(const boost::posix_time::ptime& time, Clock_t clock)
{
   init(clock);
   begin(time);
}

/// Start the Calendar.  Parameter time can include gain.
void Calendar::begin(const boost::posix_time::ptime& the_time)
{
   duration_ = time_duration(0,0,0,0);
   increment_ = time_duration(0,1,0,0); // This will get overwritten on update
                                                // But allows some tests to run
   suiteTime_ = the_time; // includes gain _IF_ it was specified
   initTime_ = the_time;  // includes gain
   dayChanged_ = false;
   initLocalTime_ = second_clock_time();  // for real time clock
   lastTime_ = initLocalTime_;            // for real time clock

   // Cache the most common requests
   update_cache();
}

void Calendar::update( const ecf::CalendarUpdateParams & calUpdateParams )
{
	assert(!suiteTime_.is_special()); // begin has not been called.

	// Get the day of week before we update calendar, then same after to determine if the day changed
	boost::gregorian::date currentdate = suiteTime_.date();
 	int theDayOfWeek = currentdate.day_of_week().as_number();

 	// However there are two ways of incremented/updating calendar.
 	if ( !calUpdateParams.forTest() ) {

 	   if (calUpdateParams.serverPollPeriod().total_seconds() < 60) {
 	      // 0/ We are still testing. User wants to speed up calendar.
 	      //    i.e. if server poll period is 2 seconds, we increment calendar by 1 minute

 	      time_duration one_minute(0,1,0,0);
 	      duration_ += one_minute;
 	      suiteTime_ += one_minute;
 	      increment_ = one_minute;

#ifdef DEBUG_CALENDAR
         std::cout << "Calendar::update:  if (calUpdateParams.serverPollPeriod().total_seconds() < 60) { \n";
#endif
 	   }
 	   else {
 	      // 1. Always Maintain phase with system clock. The calUpdateParams.timeNow()
 	      //    time was constructed from a system call in the server.
 	      //
 	      // Take a difference, which means we can ignore dates
 	      const ptime& time_now =  calUpdateParams.timeNow();
 	      assert(!time_now.is_special()); // This should have been set
 	      duration_ = time_period( initLocalTime_, time_now ).length();
 	      increment_ = time_now - lastTime_;
 	      suiteTime_ += increment_;
 	      lastTime_ = time_now;
#ifdef DEBUG_CALENDAR
 	      std::cout << "Calendar::update:  if ( !calUpdateParams.forTest() ) { \n";
#endif
 	   }
	}
	else {
		// 2. Update calendar based on server poll period/ Job submission interval
		// _OR_ For TESTING allow calendar to be speeded up.
	   // Note: for simulation serverPollPeriod could be 1 hour
		duration_ += calUpdateParams.serverPollPeriod();
		suiteTime_ += calUpdateParams.serverPollPeriod();
		increment_ = calUpdateParams.serverPollPeriod();
#ifdef DEBUG_CALENDAR
      std::cout << "Calendar::update:  calUpdateParams.serverPollPeriod() = " << calUpdateParams.serverPollPeriod() << "\n";
#endif
	}

	// Day change required for both REAL and HYBRID. See TimeDependencies.ddoc for reason
 	// This must be done before change date back. (i.e in hybrid case, below)
 	int new_day_of_week = suiteTime_.date().day_of_week().as_number();
  	if (theDayOfWeek != new_day_of_week)  dayChanged_ = true;
 	else                                  dayChanged_ = false;

	// With the hybrid calendar the date does not change
	if ( ctype_ == Calendar::HYBRID) {
 		if (suiteTime_.date() != initTime_.date()) {

#ifdef DEBUG_CALENDAR
			cout << "HYBRID: (suiteTime_.date() != initTime_.date()) suiteTime_ = " << to_simple_string(suiteTime_) << "\n";
#endif

			time_duration td = suiteTime_.time_of_day();

 			suiteTime_ = ptime( initTime_.date(), td);

#ifdef DEBUG_CALENDAR
			cout << "suiteTime_ = " << to_simple_string(suiteTime_) << "\n";
#endif
 		}
 	}

	// update_cache *MUST* be last, so we can take into account hybrid. Date does NOT change.
	// See ECFLOW-458
   update_cache();

#ifdef DEBUG_CALENDAR
	cout << "   Calendar::update serverPollPeriod = " << to_simple_string(calUpdateParams.serverPollPeriod()) << " " << toString() << endl;
#endif
}


void Calendar::update_duration_only( boost::posix_time::ptime& time_now )
{
   // 1. Always Maintain phase with system clock. The calUpdateParams.timeNow()
   //    time was constructed from a system call in the server.
   //
   // Take a difference, which means we can ignore dates
   boost::posix_time::time_duration dur = time_period( initLocalTime_, time_now ).length();
   if (dur > duration_) {
      duration_ =  dur;
   }
}

void Calendar::update_cache() const
{
   // begin() has not been called yet
   if (suiteTime_.is_special()) return;

   boost::gregorian::date newDate = suiteTime_.date();
   day_of_week_  = newDate.day_of_week().as_number();
   day_of_year_  = newDate.day_of_year();
   day_of_month_ = newDate.day();
   month_        = newDate.month();
   year_         = newDate.year();
}
int Calendar::day_of_week() const   { if (day_of_week_ == -1) update_cache(); return day_of_week_;}
int Calendar::day_of_year() const   { if (day_of_week_ == -1) update_cache(); return day_of_year_;}
int Calendar::day_of_month() const  { if (day_of_week_ == -1) update_cache(); return day_of_month_;}
int Calendar::month() const         { if (day_of_week_ == -1) update_cache(); return month_;}
int Calendar::year() const          { if (day_of_week_ == -1) update_cache(); return year_;}


void Calendar::update(const boost::posix_time::time_duration& serverPollPeriod)
{
	CalendarUpdateParams p(serverPollPeriod);
	update( p );
}

void Calendar::update(const boost::posix_time::ptime& time_now)
{
	// Used for test even though for_test is false, as we want to test that path in UNIT tests
   // Tests: path 1. shown above. Note: we pass minutes(1), to ensure path 1. is taken
	CalendarUpdateParams p( time_now, minutes(1), true /* server running */, false/* for Test*/ );
	update( p );
}

boost::gregorian::date Calendar::date() const
{
	return suiteTime_.date();
}

/// for debug
void Calendar::dump(const std::string& title) const
{
	LOG(Log::LOG,title
	                << " duration_("   << to_simple_string(duration_)
	                << ") initTime_("  << to_simple_string(initTime_)
	                << ") suiteTime_(" << to_simple_string(suiteTime_) << ")"
	);
}

std::string Calendar::toString() const
{
	std::stringstream ss;
	ss << "hybrid(" << hybrid()
     << ") duration_("   << to_simple_string(duration_)
     << ") initTime_("  << to_simple_string(initTime_)
     << ") suiteTime_(" << to_simple_string(suiteTime_)
     << ") dayChanged_(" << dayChanged_ << ")";
     ss << " increment_(" << to_simple_string(increment_) << ")";

   if (day_of_week_ == 0) ss << " SUNDAY";
   else if (day_of_week_ == 1) ss << " MONDAY";
   else if (day_of_week_ == 2) ss << " TUESDAY";
   else if (day_of_week_ == 3) ss << " WEDNESDAY";
   else if (day_of_week_ == 4) ss << " THURSDAY";
   else if (day_of_week_ == 5) ss << " FRIDAY";
   else if (day_of_week_ == 6) ss << " SATURDAY";

   return ss.str();
}


void Calendar::write_state(std::string& ret) const
{
   if ( initTime_.is_special() ) return;

   bool increment__changed = (!increment_.is_special() && increment_.total_seconds() != 0);

   // cType is obtained from the suite clock attribute, and not persisted
   ret += " initTime:";      ret += to_simple_string(initTime_);
   ret += " suiteTime:";     ret += to_simple_string(suiteTime_);
   ret += " duration:";      ret += to_simple_string(duration_);
   ret += " initLocalTime:"; ret += to_simple_string(initLocalTime_);
   ret += " lastTime:";      ret += to_simple_string(lastTime_);
   if (increment__changed) { ret += " calendarIncrement:"; ret += to_simple_string(increment_); }

   if (dayChanged_) ret += " dayChanged:1" ;
}

void Calendar::read_state(const std::string& line,const std::vector<std::string>& lineTokens)
{
   // initTime:2012-Jul-16 16:19:35 suiteTime:2012-Jul-16 16:19:35 duration:00:00:00 dayChanged:0 initLocalTime:2012-Jul-16 16:19:35 lastTime:2012-Jul-16 16:19:35 calendarIncrement:00:00:00
//   std::cout << "Calendar::read_state:"  << line << "\n";
   std::string time;
   size_t line_tokens_size = lineTokens.size();
   for(size_t i = 0; i < line_tokens_size; i++) {
      time.clear();
      const std::string& line_token = lineTokens[i];
      if (line_token.find("initTime:") != std::string::npos ) {
         if (!Extract::split_get_second(line_token,time)) throw std::runtime_error( "Calendar::read_state failed: (initTime)");
         if (i + 1 < line_tokens_size) { time += " "; time += lineTokens[i+1]; }
         else throw std::runtime_error( "Calendar::read_state failed: 1");
         initTime_ = time_from_string(time);
      }
      else if (line_token.find("suiteTime:") != std::string::npos ) {
         if (!Extract::split_get_second(line_token,time)) throw std::runtime_error( "Calendar::read_state failed: (suiteTime)");
         if (i + 1 < line_tokens_size) { time += " "; time += lineTokens[i+1]; }
         else throw std::runtime_error( "Calendar::read_state failed: 1");
         suiteTime_ = time_from_string(time);
      }
      else if (line_token.find("initLocalTime:") != std::string::npos ) {
         if (!Extract::split_get_second(line_token,time)) throw std::runtime_error( "Calendar::read_state failed: (initLocalTime)");
         if (i + 1 < line_tokens_size) { time += " "; time += lineTokens[i+1]; }
         else throw std::runtime_error( "Calendar::read_state failed: 1");
         initLocalTime_ = time_from_string(time);
      }
      else if (line_token.find("lastTime:") != std::string::npos ) {
         if (!Extract::split_get_second(line_token,time)) throw std::runtime_error( "Calendar::read_state failed: (lastTime)");
         if (i + 1 < line_tokens_size) { time += " "; time += lineTokens[i+1]; }
         else throw std::runtime_error( "Calendar::read_state failed: 1");
         lastTime_ = time_from_string(time);
      }
      else if (line_token.find("duration:") != std::string::npos ) {
         if (!Extract::split_get_second(line_token,time)) throw std::runtime_error( "Calendar::read_state failed: (duration)");
         duration_ =  duration_from_string(time);
      }
      else if (line_token.find("calendarIncrement:") != std::string::npos ) {
         if (!Extract::split_get_second(line_token,time)) throw std::runtime_error( "Calendar::read_state failed: (calendarIncrement)");
         increment_ = duration_from_string(time);
      }
      else if (line_token == "dayChanged:1") dayChanged_ = true;
   }
}

bool Calendar::checkInvariants(std::string& errorMsg) const
{
	if (!duration_.is_special()) {
		if (duration_.is_negative()) {
			errorMsg += "Calendar::checkInvariants duration is negative "+ toString() + "\n";
			return false;
		}
	}
	return true;
}

boost::posix_time::ptime Calendar::second_clock_time()
{
	/// Chose UTC since it s compatible with boost deadline timer
 	return second_clock::universal_time();  // UTC
}




template<class Archive>
void Calendar::serialize(Archive & ar, std::uint32_t const /*version*/)
{
   if (Archive::is_saving::value) {
      if ( initTime_.is_special() ) {
         // Initialise the ptimes to avoid serialisation exceptions
         // The serialisation of ptime makes use of exceptions, especially
         // when dealing with a date that has *not* been initialised.
         // To avoid this we take a small hit to initialise the calendar with
         // time now. This will get overridden with suite clock at begin
         begin(second_clock_time());
      }
   }
   ar( CEREAL_NVP(initTime_) );
   CEREAL_OPTIONAL_NVP(ar,suiteTime_ ,   [this](){return suiteTime_     != initTime_;});
   CEREAL_OPTIONAL_NVP(ar,initLocalTime_,[this](){return initLocalTime_ != initTime_;});
   CEREAL_OPTIONAL_NVP(ar,lastTime_,     [this](){return lastTime_      != initTime_;});
   CEREAL_OPTIONAL_NVP(ar,dayChanged_,   [this](){return dayChanged_;});
   CEREAL_OPTIONAL_NVP(ar,duration_ ,    [this](){return duration_  != boost::posix_time::time_duration(0,0,0,0);});
   CEREAL_OPTIONAL_NVP(ar,increment_,    [this](){return increment_ != boost::posix_time::time_duration(0,1,0,0);});

   if (Archive::is_loading::value) {
      if ( lastTime_.is_special())      lastTime_ = initTime_;
      if ( initLocalTime_.is_special()) initLocalTime_ = initTime_;
      if ( suiteTime_.is_special())     suiteTime_ = initTime_;
   }
}
CEREAL_TEMPLATE_SPECIALIZE_V(Calendar);
}
