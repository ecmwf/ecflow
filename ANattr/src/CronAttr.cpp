//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #59 $ 
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
#include <sstream>

#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>  // requires boost date and time lib
#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>
#include <boost/token_functions.hpp>
#include <boost/lexical_cast.hpp>
#include "boost/date_time/gregorian/gregorian.hpp"

#include "CronAttr.hpp"
#include "Indentor.hpp"
#include "Calendar.hpp"
#include "PrintStyle.hpp"
#include "Str.hpp"
#include "Ecf.hpp"
#include "Log.hpp"
#include "Serialization.hpp"

using namespace std;
using namespace ecf;
using namespace boost;
using namespace boost::gregorian;
using namespace boost::posix_time;

//#define DEBUG_CRON_ATTR 1
//#define DEBUG_CRON_PARSING 1
//#define DEBUG_CRON_SIM 1

namespace ecf {

CronAttr::CronAttr() = default;

CronAttr::CronAttr(const std::string& str)
:  last_day_of_month_(false),free_(false),w_(0),state_change_no_(0)
{
   if (str.empty()) throw std::runtime_error("CronAttr::CronAttr : empty string passed");
   std::vector<std::string> tokens;
   Str::split(str,tokens);
   if (tokens.empty())  throw std::runtime_error("CronAttr::CronAttr : incorrect time string ?");

   size_t index = 0;
   timeSeries_  = TimeSeries::create(index,tokens,false/*parse_state*/);
}

void CronAttr::addWeekDays( const std::vector<int>& w)
{
	weekDays_ = w;
	BOOST_FOREACH(int day,weekDays_) {
		if (day < 0 || day > 6) {
			std::stringstream ss; ss << "Invalid range for day(" << day << ") of the week expected range is 0==Sun to 6==Sat";
 			throw std::out_of_range(ss.str());
 		}
		auto result = std::find(std::begin(last_week_days_of_month_), std::end(last_week_days_of_month_), day);
		if (result != std::end(last_week_days_of_month_)) {
         std::stringstream ss; ss << "Duplicate day(" << day << ") of the week also found in last week day of the month";
         throw std::runtime_error(ss.str());
		}
 	}
}
void CronAttr::add_last_week_days_of_month( const std::vector<int>& w)
{
   last_week_days_of_month_ = w;
   BOOST_FOREACH(int day, last_week_days_of_month_) {
      if (day < 0 || day > 6) {
         std::stringstream ss; ss << "Invalid range for day(" << day << ") of the week expected range is 0==Sun to 6==Sat";
         throw std::out_of_range(ss.str());
      }
      auto result = std::find(std::begin(weekDays_), std::end(weekDays_), day);
      if (result != std::end(weekDays_)) {
         std::stringstream ss; ss << "Duplicate last week day (" << day << ") of the month also found in week day";
         throw std::runtime_error(ss.str());
      }
   }
}

void CronAttr::addDaysOfMonth( const std::vector<int>& d)
{
	daysOfMonth_ = d;
	BOOST_FOREACH(int day_of_month,daysOfMonth_) {
		if (day_of_month < 1 || day_of_month > 31) {
			std::stringstream ss; ss << "Invalid range for day of month(" << day_of_month << ") expected range is  1-31";
 			throw std::out_of_range(ss.str());
 		}
 	}
}

void CronAttr::addMonths( const std::vector<int>& m)
{
	months_ = m;
	BOOST_FOREACH(int month,months_) {
		if (month < 1 || month > 12) {
			std::stringstream ss; ss << "Invalid range for month(" << month << ")  expected range is 1==Jan to 12==Dec";
 			throw std::out_of_range(ss.str());
  		}
 	}
}

void CronAttr::print(std::string& os) const
{
	Indentor in;
	Indentor::indent(os) ; write(os);
	if (!PrintStyle::defsStyle()) {
     timeSeries_.write_state(os,free_);
	}
	os += "\n";
}

std::string CronAttr::toString() const
{
	std::string ret;
	write(ret);
 	return ret;
}

void CronAttr::write(std::string& ret) const
{
   ret += "cron ";
   if (!weekDays_.empty()) {
      ret += "-w ";
      for(size_t i=0; i<weekDays_.size();++i) {
         ret += boost::lexical_cast<std::string>(weekDays_[i]);
         if (i !=weekDays_.size()-1) ret += ",";
      }
      if (last_week_days_of_month_.empty()) ret += " ";
      else                                  ret += ",";
   }
   if (!last_week_days_of_month_.empty()) {
      if (weekDays_.empty()) ret += "-w ";
      for(size_t i=0; i< last_week_days_of_month_.size();++i) {
         ret += boost::lexical_cast<std::string>( last_week_days_of_month_[i]);
         ret += 'L';
         if (i !=last_week_days_of_month_.size()-1) ret += ",";
      }
      ret += " ";
   }

   if (!daysOfMonth_.empty()) {
      ret += "-d ";
      for(size_t i=0; i<daysOfMonth_.size();++i) {
         ret += boost::lexical_cast<std::string>(daysOfMonth_[i]);
         if (i !=daysOfMonth_.size()-1) ret += ",";
      }
      if (!last_day_of_month_) ret += " ";
   }
   if (last_day_of_month_) {
      if (daysOfMonth_.empty()) ret += "-d L ";
      else                      ret += ",L ";
   }

   if (!months_.empty()) {
      ret += "-m ";
      for(size_t i=0; i<months_.size();++i) {
         ret += boost::lexical_cast<std::string>(months_[i]);
         if (i !=months_.size()-1) ret += ",";
      }
      ret += " ";
   }

   timeSeries_.write(ret); // no new line added, up to caller
}


std::string CronAttr::dump() const
{
	std::stringstream ss; ss << toString();
 	if (free_) ss << " (free)";
	else       ss << " (holding)";
 	return ss.str();
}

bool CronAttr::operator==(const CronAttr& rhs) const
{
	if (free_ != rhs.free_) {
		return false;
	}
   if (last_day_of_month_ != rhs.last_day_of_month_) return false;
   if (weekDays_ != rhs.weekDays_) return false;
   if (last_week_days_of_month_  != rhs.last_week_days_of_month_) return false;
	if (daysOfMonth_ != rhs.daysOfMonth_) return false;
	if (months_ != rhs.months_) return false;
	return timeSeries_.operator==(rhs.timeSeries_);
}
bool CronAttr::structureEquals(const CronAttr& rhs) const
{
   if (last_day_of_month_ != rhs.last_day_of_month_) return false;
   if (weekDays_ != rhs.weekDays_) return false;
	if (daysOfMonth_ != rhs.daysOfMonth_) return false;
   if (last_week_days_of_month_  != rhs.last_week_days_of_month_) return false;
	if (months_ != rhs.months_) return false;
   return timeSeries_.structureEquals(rhs.timeSeries_);
}

void CronAttr::calendarChanged( const ecf::Calendar& c )
{
   if ( free_ ) {
      return;
   }

   // This assumes that calendarChanged will set TimeSeries::isValid = true, at day change
   if (timeSeries_.calendarChanged(c)) {
      state_change_no_ = Ecf::incr_state_change_no();
   }

   // Once a cron is free, it stays free until re-queue
   if (isFree(c)) {
      setFree();
   }
   // A cron is always re-queable, hence we use isFree to control when it can actually run.
}

void CronAttr::resetRelativeDuration()
{
   if (timeSeries_.resetRelativeDuration()) {
      state_change_no_ = Ecf::incr_state_change_no();
   }
}

void CronAttr::setFree() {
	free_ = true;
	state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "CronAttr::setFree()\n";
#endif
}

void CronAttr::clearFree() {
	free_ = false;
	state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "CronAttr::clearFree()\n";
#endif
}

void CronAttr::miss_next_time_slot()
{
   // A cron attribute with a single time slot is repeated indefinitely hence always re-queues
   timeSeries_.miss_next_time_slot();
   state_change_no_ = Ecf::incr_state_change_no();
}

// **************************************************************************************
// FOR DEBUG THIS IS THE MAIN FUNCTION, AS THIS DECIDES WHETHER WE CONTINE OR STOP
// **************************************************************************************
bool CronAttr::checkForRequeue( const ecf::Calendar& calendar) const
{
	// checkForRequeue is called when a task/family has reach the complete state
	// This simple checks if node should be put in re-queued state
   // A cron is always re-queable

   // Hence: In order to use this it should be used in conjunction with a
   // with a parent node that has complete  expression, (& maybe a dummy task)
   // This will allow its use with a parent repeat somewhere in the hierarchy
   return true;
}

bool CronAttr::validForHybrid(const ecf::Calendar& calendar) const
{
	if (timeSeries_.hasIncrement()) {
	   if (last_day_of_month_)        return false; // relies on day change
		if (!months_.empty() )         return false; // relies on day change
		if (!daysOfMonth_.empty() )    return false; // relies on day change
		if (!weekDays_.empty()  )  {
			if ( weekDays_.size() != 1)  return false; // relies on day change
   	 	return ( weekDays_[0] == calendar.day_of_week() );
		}


		// cron 10:00 20:00 01:00  // valid for hybrid ?
		return true;
	}

	// A time series that does not have an increment runs indefinitely and hence relies on day change
	// cron 23:00
	return false;
}

bool CronAttr::why(const ecf::Calendar& c, std::string& theReasonWhy) const
{
   // This will logically AND all the times
	if (isFree(c)) return false;

	// We are here because:
	//  1/ Not on a valid time slot in the time series
	//  *OR*
	//  2/ Logical *AND* of day of week, day of month, or month returned false
	theReasonWhy += "is cron dependent";

	// Lets say that the time series was NOT free.
	// First check if week day, day of month, month, matches
 	if ( is_day_of_week_day_of_month_and_month_free(c)) {

 	   if (timeSeries_.is_valid()) {

 	      // This can apply to single and series
 	      boost::posix_time::time_duration calendar_time = timeSeries_.duration(c);
 	      if (calendar_time < timeSeries_.start().duration()) {
 	         timeSeries_.why(c, theReasonWhy);
 	         return true;
 	      }

 	      // calendar_time >= timeSeries_.start().duration()
 	      if (timeSeries_.hasIncrement()) {
 	         if (calendar_time < timeSeries_.finish().duration()) {
 	            timeSeries_.why(c, theReasonWhy);
 	            return true;
 	         }
 	      }
 	   }
 	   // calendar_time >= timeSeries_.start().duration() && calendar_time >= timeSeries_.finish().duration()
  		// past the end of time slot, find next valid date
  	}

   // take into account, user can use run/force complete to miss time slots
   bool do_a_requeue = timeSeries_.requeueable(c);
   if (do_a_requeue && weekDays_.empty() && daysOfMonth_.empty() && months_.empty()) {
      TimeSlot the_next_time_slot = timeSeries_.compute_next_time_slot(c);
      if (the_next_time_slot.isNULL() ) {
         theReasonWhy += " ( *re-queue* to run at this time ";
      }
      else {
         theReasonWhy += " ( *re-queue* to run at ";
         theReasonWhy += the_next_time_slot.toString() ;
      }

      theReasonWhy += ", otherwise next run is at ";
   }
   else {
      theReasonWhy += " ( next run is at ";
   }

  	// Find the *NEXT* date that matches, and use the first time slot
 	boost::gregorian::date the_next_date = next_date(c);
 	theReasonWhy += timeSeries_.start().toString();
 	theReasonWhy += " ";
 	theReasonWhy += to_simple_string( the_next_date );

 	std::stringstream ss;
   TimeSlot currentTime = TimeSlot(timeSeries_.duration(c));
   ss << ", current time ";
   if (timeSeries_.relative()) ss << "+";
   ss << currentTime.toString() << " " << to_simple_string(c.date()) << " )";
   theReasonWhy += ss.str();
	return true;
}

void CronAttr::reset_only()
{
   clearFree();
   timeSeries_.reset_only();
}

void CronAttr::reset(const ecf::Calendar& c)
{
	clearFree();
 	timeSeries_.reset(c);
}

void CronAttr::requeue(const ecf::Calendar& c, bool reset_next_time_slot)
{
   clearFree();
   timeSeries_.requeue(c,reset_next_time_slot);
}

bool CronAttr::isFree(const ecf::Calendar& c) const
{
	// The FreeDepCmd can be used to free the crons,
	if (free_) {
		return true;
	}

	if (!timeSeries_.isFree(c))  return false;

	// Ok time series is Free

	// ********************************************************************
	// IMPORTANT: when we have multiple week days, days of month and months
	// Then we are *ONLY* free, if *ALL* are free, i.e we need AND behaviour
	// ********************************************************************
	return is_day_of_week_day_of_month_and_month_free(c);
}


bool CronAttr::is_day_of_week_day_of_month_and_month_free( const ecf::Calendar& c) const
{
#ifdef DEBUG_CRON_SIM
   cout << toString() << "  cal : " << to_simple_string(c.date())
            << " c.day_of_week:" << c.day_of_week() << " c.day_of_month:" << c.day_of_month();
   cout.flush();
#endif

   bool the_week_day_matches = weekDays_.empty() && last_week_days_of_month_.empty(); // week day matches if no week days
   bool the_day_of_month_matches = daysOfMonth_.empty();  // day of month if no days of month
   bool the_month_matches = months_.empty();              // month matches if no months

   if ( !weekDays_.empty())
      the_week_day_matches = week_day_matches(c.day_of_week());
   if ( !the_week_day_matches && !last_week_days_of_month_.empty())
      the_week_day_matches = last_week_day_of_month_matches(c);

   if ( !daysOfMonth_.empty() || last_day_of_month_)
      the_day_of_month_matches = day_of_month_matches(c.day_of_month(),c);
   if ( !months_.empty()  )
      the_month_matches   = month_matches(c.month());


   // Remember we *AND* across -w, -d, -m or *OR* for each element in -w, -d,-m
   bool matches = false;
   if (daysOfMonth_.empty() && !last_day_of_month_ && months_.empty()) {
      // cron -w 0L 10:00    # run on the last sunday of each month
      matches = the_week_day_matches ; // only week day,
   }
   else {
      matches = the_week_day_matches && the_day_of_month_matches && the_month_matches ;
   }

#ifdef DEBUG_CRON_SIM
   if (matches) {
      cout << " *MATCHES*";
   }
   cout << "\n";
#endif

   return matches;
}

bool CronAttr::week_day_matches( int theDayOfWeek ) const
{
 	BOOST_FOREACH(int theWeekDay, weekDays_) {
 		if ( theDayOfWeek == theWeekDay)  return true;
 	}
 	return false;
}

bool CronAttr::last_week_day_of_month_matches( const ecf::Calendar& c  ) const
{
   int cal_day_of_week = c.day_of_week();
   boost::gregorian::date last_day_of_month = c.date().end_of_month();
   boost::gregorian::date_duration diff_current_date_and_last_day_of_month = last_day_of_month - c.date();

   BOOST_FOREACH(int cron_last_week_day_of_month,last_week_days_of_month_ ) {
      if ( cal_day_of_week == cron_last_week_day_of_month ) {

         if ( diff_current_date_and_last_day_of_month.days() < 7) {
            return true;
         }
      }
   }
   return false;
}

bool CronAttr::day_of_month_matches(int theDayOfMonth, const ecf::Calendar& c) const
{
	BOOST_FOREACH(int dayOfMonth, daysOfMonth_) {
	 	if ( theDayOfMonth == dayOfMonth)  return true;
	}
	if (last_day_of_month_) {
	   return c.date() == c.date().end_of_month();
	}
	return false;
}

bool CronAttr::month_matches(int theMonth) const
{
 	 BOOST_FOREACH(int month, months_) {
	 	if ( theMonth == month) return true;
	 }
 	 return false;
}

//------------------------------------------------------------------

bool CronAttr::checkInvariants(std::string& errormsg) const
{
	return  timeSeries_.checkInvariants(errormsg);
}

//--------------------------------------------------------------

boost::gregorian::date CronAttr::next_date(const ecf::Calendar& calendar) const
{
	// Find the next date that matches, day of week, day of year, and month
	// that is greater than todays date. This *ASSUMES* day of week, day of month,
	// and month is *ANDED* together
 	boost::gregorian::date_duration one_day(1);
	boost::gregorian::date future_date = calendar.date();  // todays date

#ifdef DEBUG_CRON_SIM
   cout  << "cron : " << toString() << "\n";
   cout << "future_date start : " << to_simple_string(future_date) << "\n";
#endif
	future_date += one_day;                     // add one day, so its in the future

 	while ( true ) {

 		bool week_day_matches = weekDays_.empty();         // week day matches if no week days
 	   bool the_last_week_day_of_month_matches = last_week_days_of_month_.empty(); // matches if EMPTY
 		bool day_of_month_matches = daysOfMonth_.empty();  // day of month if no days of month
 		bool month_matches = months_.empty();              // month matches if no months
 		if ( daysOfMonth_.empty() && last_day_of_month_) day_of_month_matches = false;

		// deal with case where we have: cron -w 0,1
 		for (int weekDay : weekDays_) {
			if ( future_date.day_of_week().as_number() == weekDay ) {
				week_day_matches = true;
				break;
			}
		}
 		// *IMPORTANT* the days in weekDays_ and last_week_days_of_month_ can *NOT* overlap.
      for (int weekDay : last_week_days_of_month_ ) {
         if ( future_date.day_of_week().as_number() == weekDay ) {
            boost::gregorian::date_duration diff = future_date - future_date.end_of_month();
            if (diff.days() < 7 ) {
               the_last_week_day_of_month_matches = true;
            }
            break;
         }
      }

 		// deal with case where we have: cron  -d 14,15,16,L  # L means last day of month
      if ( !daysOfMonth_.empty() || last_day_of_month_) {
         for (int d : daysOfMonth_) {
            if ( future_date.day() == d ) {
               day_of_month_matches = true;
               break;
            }
         }
         if (last_day_of_month_ && future_date == future_date.end_of_month()) {
            day_of_month_matches = true;
         }
      }

 		// deal with case where we have: cron -w 0,1 -d 14,15,16 -m 8, 9
 		for(int month : months_) {
			if ( future_date.month() == month ) {
				month_matches = true;
				break;
  			}
		}

 		// if it all matches, then return the future day
 	   // Remember we *AND* across -w, -d, -m or *OR* for each element in -w, -d,-m
 		if ( ( week_day_matches || the_last_week_day_of_month_matches) && day_of_month_matches && month_matches ) {
 			break; // return future_date, replaced with break to  keep HPUX compiler happy
 			       // otherwise it complains that return at the end of the function is
 			       // unreachable
 		}

		future_date += one_day;
#ifdef DEBUG_CRON_SIM
		cout << "future_date " << to_simple_string(future_date) << "\n";
#endif
	}
 	return future_date; // should never happen, i.e we can find future date that matches
}


//=========================================================================================================
// code for parsing a cron:

static bool isComment(const std::string& token)
{
	if (token.find("#") == std::string::npos) return false;
	return true;
}

static bool isTimeSpec(const std::string& token)
{
	if (token.find(Str::COLON()) == std::string::npos) return false;
	return true;
}

static bool isOption(const std::string& token)
{
	if (token.find("-w") != std::string::npos) return true;
	if (token.find("-d") != std::string::npos) return true;
	if (token.find("-m") != std::string::npos) return true;
	return false;
}

static std::string nextToken( size_t& index, const std::vector<std::string >& lineTokens)
{
	assert(index < lineTokens.size());
	index++;
	if (index < lineTokens.size()) {
#ifdef DEBUG_CRON_PARSING
	 	cerr << "nextToken lineTokens[" << index << "] = " << lineTokens[index] << "\n";
#endif
		return lineTokens[index];
	}
#ifdef DEBUG_CRON_PARSING
 	cerr << "nextToken empty \n";
#endif
	return string();
}

std::string extract_list( size_t& index, const std::vector<std::string >& lineTokens)
{
	// cron -w 0,1L,2L,3 -d 1,12,14,L   -m 5,6,7,8   10:00 20:00 01:00
	assert(index < lineTokens.size());

	// Collate the list of integers, these may have been separated by spaces
	// since we stop on option or time spec, the top level code should decrement index
	std::string theIntList;
	while ( index < lineTokens.size() && ( !isOption(lineTokens[index]) || !isTimeSpec(lineTokens[index]) )) {
		string theNextToken = nextToken(index,lineTokens);
		if (theNextToken.empty()) break;
		if (isOption( theNextToken )) break;
		if (isTimeSpec( theNextToken )) break;
		theIntList += theNextToken;
  	}
#ifdef DEBUG_CRON_PARSING
	cerr << "theIntList = " << theIntList << "\n";
#endif
	return theIntList;
}

std::vector<int> extract_month(size_t& index,
                               const std::vector<std::string >& lineTokens,
                               const std::string& option)
{
   // cron -w 0,1L,2L,3 -d 1,12,14,L   -m 5,6,7,8   10:00 20:00 01:00
   assert(index < lineTokens.size());

   // Collate the list of integers, these may have been separated by spaces
   // since we stop on option or time spec, the top level code should decrement index
   std::string theIntList = extract_list(index,lineTokens);

   // should have 0,1,2,3
   std::vector< int > theIntVec;
   char_separator< char > sep( ",", nullptr, boost::drop_empty_tokens );
   typedef boost::tokenizer< boost::char_separator< char > > tokenizer;
   tokenizer theTokenizer( theIntList, sep );

   for (tokenizer::iterator beg = theTokenizer.begin(); beg != theTokenizer.end(); ++beg) {
      string theIntToken = *beg;
      boost::algorithm::trim( theIntToken );
      if ( theIntToken.empty() ) continue;

      try {
         auto theInt = boost::lexical_cast< int >( theIntToken );
         theIntVec.push_back( theInt );
      }
      catch ( boost::bad_lexical_cast& ) {
         std::stringstream ss; ss << "Invalid cron option: " << option  ;
         throw std::runtime_error( ss.str() );
      }
   }
   return theIntVec;
}

void extract_days_of_week(size_t& index, const std::vector<std::string >& lineTokens, const std::string& option,
                          std::vector<int>& days_of_week, std::vector<int>& last_week_days_of_month)
{
   // cron -w 0,1L,2L,3 10:00 20:00 01:00
   assert(index < lineTokens.size());

   // Collate the list of integers, these may have been separated by spaces
   // since we stop on option or time spec, the top level code should decrement index
   std::string theIntList = extract_list(index,lineTokens);

   // should have 0,1,2,3,4L
   char_separator< char > sep( ",", nullptr, boost::drop_empty_tokens );
   typedef boost::tokenizer< boost::char_separator< char > > tokenizer;
   tokenizer theTokenizer( theIntList, sep );

   for (tokenizer::iterator beg = theTokenizer.begin(); beg != theTokenizer.end(); ++beg) {
      string theIntToken = *beg;
      boost::algorithm::trim( theIntToken );
      if ( theIntToken.empty() ) continue;

      try {
         if (theIntToken.size() == 2) {
            if (theIntToken[1] != 'L' ) {
               std::stringstream ss; ss << "Invalid cron option: " << option << " " << theIntToken;
               throw std::runtime_error( ss.str() );
            }
            auto theInt = boost::lexical_cast< int >( theIntToken[0] );
            last_week_days_of_month.push_back(theInt);
         }
         else {
            auto theInt = boost::lexical_cast< int >( theIntToken );
            days_of_week.push_back( theInt );
         }
      }
      catch ( boost::bad_lexical_cast& ) {
         std::stringstream ss; ss << "Invalid cron option: " << option  ;
         throw std::runtime_error( ss.str() );
      }
   }
}

void extract_days_of_month(size_t& index, const std::vector<std::string >& lineTokens, const std::string& option,
                          std::vector<int>& days_of_month, bool& last_day_of_month)
{
   // cron -d 1,12,14,L 10:00 20:00 01:00

   assert(index < lineTokens.size());

   // Collate the list of integers, these may have been separated by spaces
   // since we stop on option or time spec, the top level code should decrement index
   std::string theIntList = extract_list(index,lineTokens);

   // should have 0,1,2,3,4L
   char_separator< char > sep( ",", nullptr, boost::drop_empty_tokens );
   typedef boost::tokenizer< boost::char_separator< char > > tokenizer;
   tokenizer theTokenizer( theIntList, sep );

   for (tokenizer::iterator beg = theTokenizer.begin(); beg != theTokenizer.end(); ++beg) {
      string theIntToken = *beg;
      boost::algorithm::trim( theIntToken );
      if ( theIntToken.empty() ) continue;

      try {
         if (theIntToken == "L")  last_day_of_month = true;
         else {
            auto theInt = boost::lexical_cast< int >( theIntToken );
            days_of_month.push_back( theInt );
         }
      }
      catch ( boost::bad_lexical_cast& ) {
         std::stringstream ss; ss << "Invalid cron option: " << option  ;
         throw std::runtime_error( ss.str() );
      }
   }
}

void extractOption(CronAttr& cronAttr, size_t& index, const std::vector<std::string >& lineTokens)
{
	assert(index < lineTokens.size());
	if (lineTokens[index] == "-w") {
      std::vector<int> days_of_week;
      std::vector<int> last_week_days_of_month;
      extract_days_of_week(index, lineTokens, "week days",days_of_week,last_week_days_of_month );

      cronAttr.addWeekDays(days_of_week );
      cronAttr.add_last_week_days_of_month( last_week_days_of_month );
 	}
	else if (lineTokens[index] == "-d") {
      std::vector<int> days_of_month;
      bool last_day_of_month = false;
      extract_days_of_month(index, lineTokens, "Days of the month",days_of_month, last_day_of_month);

 		cronAttr.addDaysOfMonth( days_of_month );
 		if (last_day_of_month) cronAttr.add_last_day_of_month();
  	}
	else if (lineTokens[index] == "-m") {
 		cronAttr.addMonths( extract_month(index, lineTokens,"Months" ) );
  	}
	else throw std::runtime_error( "extractOption: Invalid cron option :" + lineTokens[index] );
}

void CronAttr::parse( CronAttr& cronAttr, const std::vector<std::string>& lineTokens, size_t index, bool parse_state )
{
	// cron 23:00                 # run every day at 23:00
	// cron 10:00 20:00 01:00     # run every hour between 10am and 8pm
	// cron -w 0,1 10:00          # run every sunday and monday at 10am
	// cron -d 10,11,12 12:00     # run 10th, 11th and 12th of each month at noon
	// cron -m 1,2,3 12:00        # run on Jan,Feb and March every day at noon.
   // cron -w 0 -m 5,6,7,8 10:00 20:00 01:00 # run every sunday, between May-Aug, every hour between 10am and 8pm
   // cron -w 0,1,2L -d 5,6,L  23:00 # run every sunday,monday, and last tuesday of the month, 5,6 on month, and *last* day of month @11 pm

   // make *sure* a time spec is specified
   bool time_spec_specified = false;
   size_t line_tokens_size = lineTokens.size();
 	while (index < line_tokens_size) {

		const std::string& token = lineTokens[index];
#ifdef DEBUG_CRON_PARSING
		cerr << "CronAttr::doParse " << token << "\n";
#endif

		if (isOption(token)) {
#ifdef DEBUG_CRON_PARSING
			cerr << "CronAttr::doParse isOption \n";
#endif
 			extractOption(cronAttr,index,lineTokens);
			index--; // since we did a look ahead
		}
		else if (!time_spec_specified && isTimeSpec(token)) {
#ifdef DEBUG_CRON_PARSING
			cerr << "CronAttr::doParse isTimeSpec \n";
#endif
			// index is passed by *reference*, and used skip over time series
			cronAttr.addTimeSeries(  TimeSeries::create(index,lineTokens, parse_state) );
			time_spec_specified = true;
			if (parse_state) {
			   //  if index is on the comment, back track, so that we can add cron state( free)
			   if (index < line_tokens_size && lineTokens[index] == "#") {
			      index--;
			   }
			}
			else break; // need to read state after comment
 		}
		else if (isComment(token)) {
		   // cron -m 1,2,3 12:00        # free
		   if (parse_state && index+1 < line_tokens_size) {
		      if (lineTokens[index+1] == "free") {
		         cronAttr.setFree();
		      }
		   }
			break;
		}
		index++;
	}

 	if (!time_spec_specified) {
 	   throw std::runtime_error( "Invalid cron, no time specified");
 	}

#ifdef DEBUG_CRON_PARSING
	cronAttr.print(cerr); cerr <<"\n";
#endif
}

CronAttr CronAttr::create(const std::string& cronString)
{
	std::vector<std::string> lineTokens;
	Str::split(cronString,lineTokens);

	CronAttr theCronAttr;
   if ( lineTokens.empty() ) {
      return theCronAttr;
   }

   // adjust the index
	size_t index = 0;
	if ( lineTokens[0] == "cron") {
		index = 1;
	}

	parse(theCronAttr,lineTokens,index);
	return theCronAttr;
}


template<class Archive>
void CronAttr::serialize(Archive & ar, std::uint32_t const version )
{
   ar( CEREAL_NVP(timeSeries_));
   CEREAL_OPTIONAL_NVP(ar, weekDays_ ,              [this](){return !weekDays_.empty(); });   // conditionally save
   CEREAL_OPTIONAL_NVP(ar, last_week_days_of_month_,[this](){return !last_week_days_of_month_.empty(); });// conditionally save
   CEREAL_OPTIONAL_NVP(ar, daysOfMonth_,            [this](){return !daysOfMonth_.empty(); });// conditionally save
   CEREAL_OPTIONAL_NVP(ar, months_,                 [this](){return !months_.empty(); });     // conditionally save
   CEREAL_OPTIONAL_NVP(ar, free_,                   [this](){return free_; });                // conditionally save
   CEREAL_OPTIONAL_NVP(ar, last_day_of_month_,      [this](){return last_day_of_month_; });   // conditionally save
   CEREAL_OPTIONAL_NVP(ar, w_,                      [this](){return w_ != 0; });              // conditionally save
}
CEREAL_TEMPLATE_SPECIALIZE_V(CronAttr);

}

