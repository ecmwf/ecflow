//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #65 $ 
//
// Copyright 2009-2012 ECMWF. 
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

using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

//#define DEBUG_CALENDAR 1;

namespace ecf {

Calendar::Calendar()
: ctype_(Calendar::HYBRID),
  dayChanged_(false),
  startStopWithServer_(false),
  day_of_week_(-1),
  day_of_year_(-1),
  day_of_month_(-1),
  month_(-1),
  year_(-1)
{
}

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
	if (startStopWithServer_ != rhs.startStopWithServer_) {
#ifdef DEBUG
      if (Ecf::debug_equality())  std::cout << "Calendar::operator== startStopWithServer_  don't match\n";
#endif
	   return false;
	}
	if (calendarIncrement_ !=rhs.calendarIncrement_) {
#ifdef DEBUG
      if (Ecf::debug_equality())  std::cout << "Calendar::operator== calendarIncrement_  don't match\n";
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
 	startStopWithServer_ =  rhs.startStopWithServer_;
  	initLocalTime_ =  rhs.initLocalTime_;
 	lastTime_ =  rhs.lastTime_;
	calendarIncrement_ = rhs.calendarIncrement_;

	day_of_week_ = rhs.day_of_week_;   // Cache
	day_of_year_ = rhs.day_of_year_;   // Cache
	day_of_month_ = rhs.day_of_month_; // Cache
	month_ = rhs.month_;               // Cache
	year_ = rhs.year_;                 // Cache
}

void Calendar::init(Clock_t clock, bool startStopWithServer)
{
   ctype_ = clock;
   startStopWithServer_ = startStopWithServer;
}

/// Start the Calendar.  Parameter time can include gain.
void Calendar::begin(const boost::posix_time::ptime& the_time)
{
   duration_ = time_duration(0,0,0,0);
   calendarIncrement_ = time_duration(0,1,0,0); // This will get overwritten on update
                                                // But allows some tests to run
   suiteTime_ = the_time; // includes gain _IF_ it was specified
   initTime_ = the_time;  // includes gain
   dayChanged_ = false;
   initLocalTime_ = second_clock_time();  // for real time clock
   lastTime_ = initLocalTime_;            // for real time clock

   // Cache the most common requests
   update_cache();
}

void Calendar::init(const boost::posix_time::ptime& time, Clock_t clock, bool startStopWithServer)
{
   init(clock,startStopWithServer);
   begin(time);
}

void Calendar::update( const ecf::CalendarUpdateParams & calUpdateParams )
{
	assert(!suiteTime_.is_special()); // begin has not been called.

	// Get the day of week before we update calendar, then same after to determine if the day changed
	boost::gregorian::date currentdate = suiteTime_.date();
 	int theDayOfWeek = currentdate.day_of_week().as_number();

 	// However there are two ways of incremented/updating calendar.
 	if ( !startStopWithServer_ && !calUpdateParams.forTest() ) {

 	   if (calUpdateParams.serverPollPeriod().total_seconds() < 60) {
 	      // 0/ We are still testing. User wants to speed up calendar.
 	      //    i.e. if server poll period is 2 seconds, we increment calendar by 1 minute

 	      time_duration one_minute(0,1,0,0);
 	      duration_ += one_minute;
 	      suiteTime_ += one_minute;
 	      calendarIncrement_ = one_minute;

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
 	      calendarIncrement_ = time_now - lastTime_;
 	      suiteTime_ += calendarIncrement_;
 	      lastTime_ = time_now;
#ifdef DEBUG_CALENDAR
 	      std::cout << "Calendar::update:  if ( !startStopWithServer_ && !calUpdateParams.forTest() ) { \n";
#endif
 	   }
	}
	else {
		// 2. Update calendar based on server poll period/ Job submission interval
		// _OR_ For TESTING allow calendar to be speeded up.
	   // Note: for simulation serverPollPeriod could be 1 hour
		duration_ += calUpdateParams.serverPollPeriod();
		suiteTime_ += calUpdateParams.serverPollPeriod();
		calendarIncrement_ = calUpdateParams.serverPollPeriod();
#ifdef DEBUG_CALENDAR
      std::cout << "Calendar::update:  calUpdateParams.serverPollPeriod() = " << calUpdateParams.serverPollPeriod() << "\n";
#endif
	}

 	update_cache();

	// *This relies on update_cache() being called first, since it needs day_of_week_
	// Day change required for both REAL and HYBRID. See TimeDependencies.ddoc for reason
  	if (theDayOfWeek != day_of_week_)  dayChanged_ = true;
 	else                               dayChanged_ = false;

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

#ifdef DEBUG_CALENDAR
	cout << "   Calendar::update serverPollPeriod = " << to_simple_string(calUpdateParams.serverPollPeriod()) << " " << toString() << endl;
#endif
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
	if ( ctype_ == Calendar::HYBRID) return initTime_.date();
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
     ss << " calendarIncrement_(" << to_simple_string(calendarIncrement_) << ")";

   if (day_of_week_ == 0) ss << " SUNDAY";
   else if (day_of_week_ == 1) ss << " MONDAY";
   else if (day_of_week_ == 2) ss << " TUESDAY";
   else if (day_of_week_ == 3) ss << " WEDNESDAY";
   else if (day_of_week_ == 4) ss << " THURSDAY";
   else if (day_of_week_ == 5) ss << " FRIDAY";
   else if (day_of_week_ == 6) ss << " SATURDAY";

 	return ss.str();
}


std::string Calendar::write_state() const
{
   if ( initTime_.is_special() ) return string();

   bool calendarIncrement__changed = (!calendarIncrement_.is_special() && calendarIncrement_.total_seconds() != 0);

   // cType is obtained from the suite clock attribute, and not persisted
   std::stringstream ss;
   ss << " initTime:" << to_simple_string(initTime_);
   ss << " suiteTime:" << to_simple_string(suiteTime_);
   ss << " duration:" << to_simple_string(duration_);
   ss << " initLocalTime:" << to_simple_string(initLocalTime_);
   ss << " lastTime:" << to_simple_string(lastTime_);
   if (calendarIncrement__changed) ss << " calendarIncrement:" << to_simple_string(calendarIncrement_);

   if (dayChanged_) ss << " dayChanged:" << dayChanged_;
   return ss.str();
}

void Calendar::read_state(const std::string& line,const std::vector<std::string>& lineTokens)
{
   // initTime:2012-Jul-16 16:19:35 suiteTime:2012-Jul-16 16:19:35 duration:00:00:00 dayChanged:0 initLocalTime:2012-Jul-16 16:19:35 lastTime:2012-Jul-16 16:19:35 calendarIncrement:00:00:00
//   std::cout << "Calendar::read_state:"  << line << "\n";
   std::string time;
   for(size_t i = 0; i < lineTokens.size(); i++) {
      time.clear();
      if (lineTokens[i].find("initTime:") != std::string::npos ) {
         if (!Extract::split_get_second(lineTokens[i],time)) throw std::runtime_error( "Calendar::read_state failed: (initTime)");
         if (i + 1 < lineTokens.size()) { time += " "; time += lineTokens[i+1]; }
         else throw std::runtime_error( "Calendar::read_state failed: 1");
         initTime_ = time_from_string(time);
      }
      else if (lineTokens[i].find("suiteTime:") != std::string::npos ) {
         if (!Extract::split_get_second(lineTokens[i],time)) throw std::runtime_error( "Calendar::read_state failed: (suiteTime)");
         if (i + 1 < lineTokens.size()) { time += " "; time += lineTokens[i+1]; }
         else throw std::runtime_error( "Calendar::read_state failed: 1");
         suiteTime_ = time_from_string(time);
      }
      else if (lineTokens[i].find("initLocalTime:") != std::string::npos ) {
         if (!Extract::split_get_second(lineTokens[i],time)) throw std::runtime_error( "Calendar::read_state failed: (initLocalTime)");
         if (i + 1 < lineTokens.size()) { time += " "; time += lineTokens[i+1]; }
         else throw std::runtime_error( "Calendar::read_state failed: 1");
         initLocalTime_ = time_from_string(time);
      }
      else if (lineTokens[i].find("lastTime:") != std::string::npos ) {
         if (!Extract::split_get_second(lineTokens[i],time)) throw std::runtime_error( "Calendar::read_state failed: (lastTime)");
         if (i + 1 < lineTokens.size()) { time += " "; time += lineTokens[i+1]; }
         else throw std::runtime_error( "Calendar::read_state failed: 1");
         lastTime_ = time_from_string(time);
      }
      else if (lineTokens[i].find("duration:") != std::string::npos ) {
         if (!Extract::split_get_second(lineTokens[i],time)) throw std::runtime_error( "Calendar::read_state failed: (duration)");
         duration_ =  duration_from_string(time);
      }
      else if (lineTokens[i].find("calendarIncrement:") != std::string::npos ) {
         if (!Extract::split_get_second(lineTokens[i],time)) throw std::runtime_error( "Calendar::read_state failed: (calendarIncrement)");
         calendarIncrement_ = duration_from_string(time);
      }
      else if (lineTokens[i] == "dayChanged:1") dayChanged_ = true;
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

}
