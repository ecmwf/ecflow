//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #59 $ 
//
// Copyright 2009-2012 ECMWF. 
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

#include "CronAttr.hpp"
#include "Indentor.hpp"
#include "Calendar.hpp"
#include "PrintStyle.hpp"
#include "Str.hpp"
#include "Ecf.hpp"
#include "Log.hpp"

using namespace std;
using namespace ecf;
using namespace boost;
using namespace boost::gregorian;
using namespace boost::posix_time;

//#define DEBUG_CRON_ATTR 1
//#define DEBUG_CRON_PARSING 1

namespace ecf {

CronAttr::CronAttr() :  makeFree_(false),state_change_no_(0) {}

void CronAttr::addWeekDays( const std::vector<int>& w)
{
	weekDays_ = w;
	BOOST_FOREACH(int day,weekDays_) {
		if (day < 0 || day > 6) {
			std::stringstream ss; ss << "Invalid range for day(" << day << ") of the week expected range is 0==Sun to 6==Sat";
 			throw std::out_of_range(ss.str());
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

std::ostream& CronAttr::print(std::ostream& os) const
{
	Indentor in;
	Indentor::indent(os) << toString();
	if (!PrintStyle::defsStyle()) {
      os << timeSeries_.state_to_string(makeFree_);
	}
	os << "\n";
	return os;
}

std::string CronAttr::toString() const
{
	std::stringstream ss;
	ss << "cron ";
	if (!weekDays_.empty()) {
		ss << "-w ";
		for(size_t i=0; i<weekDays_.size();++i) {
			ss << weekDays_[i];
			if (i !=weekDays_.size()-1) ss << ",";
		}
		ss << " ";
	}
	if (!daysOfMonth_.empty()) {
		ss << "-d ";
		for(size_t i=0; i<daysOfMonth_.size();++i) {
			ss << daysOfMonth_[i];
			if (i !=daysOfMonth_.size()-1) ss << ",";
		}
		ss << " ";
	}
	if (!months_.empty()) {
		ss << "-m ";
		for(size_t i=0; i<months_.size();++i) {
			ss << months_[i];
			if (i !=months_.size()-1) ss << ",";
		}
		ss << " ";
	}

	ss << timeSeries_.toString(); // no new line added, up to caller
 	return ss.str();
}

std::string CronAttr::dump() const
{
	std::stringstream ss; ss << toString();
 	if (makeFree_) ss << " (free)";
	else           ss << " (holding)";
 	return ss.str();
}

bool CronAttr::operator==(const CronAttr& rhs) const
{
	if (makeFree_ != rhs.makeFree_) {
		return false;
	}

	if (weekDays_ != rhs.weekDays_) return false;
	if (daysOfMonth_ != rhs.daysOfMonth_) return false;
	if (months_ != rhs.months_) return false;
	return timeSeries_.operator==(rhs.timeSeries_);
}
bool CronAttr::structureEquals(const CronAttr& rhs) const
{
	if (weekDays_ != rhs.weekDays_) return false;
	if (daysOfMonth_ != rhs.daysOfMonth_) return false;
	if (months_ != rhs.months_) return false;
   return timeSeries_.structureEquals(rhs.timeSeries_);
}

void CronAttr::calendarChanged( const ecf::Calendar& c )
{
   if ( makeFree_ ) {
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
	makeFree_ = true;
	state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "CronAttr::setFree()\n";
#endif
}

void CronAttr::clearFree() {
	makeFree_ = false;
	state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "CronAttr::clearFree()\n";
#endif
}

void CronAttr::miss_next_time_slot()
{
   // A cron attribute with a single time slot is repeated indefinitely hence always re-queue
   // hence only miss a time slot when we have a time series
   if (timeSeries_.hasIncrement()) {
      timeSeries_.miss_next_time_slot();
      state_change_no_ = Ecf::incr_state_change_no();
   }
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
	theReasonWhy += " is cron dependent ";

	// Lets say that the time series was NOT free.
	// First check if week day, day of month, month, matches
 	if ( is_day_of_week_day_of_month_and_month_free(c)) {

 	   // This can apply to single and series
 	   boost::posix_time::time_duration calendar_time = timeSeries_.duration(c);
 	   if (calendar_time < timeSeries_.start().duration()) {
 	       timeSeries_.why(c, theReasonWhy);
 	       return true;
 	   }

 	   if (timeSeries_.hasIncrement()) {
 	      if (calendar_time > timeSeries_.start().duration() && calendar_time < timeSeries_.finish().duration()) {
 	          timeSeries_.why(c, theReasonWhy);
 	          return true;
 	      }
 	   }
  		// past the end of time slot, find next valid date
  	}

  	// Find the *NEXT* date that matches, and use the first time slot
 	boost::gregorian::date the_next_date = next_date(c);
 	theReasonWhy += " ( next run is at ";
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

void CronAttr::reset(const ecf::Calendar& c)
{
	clearFree();
 	timeSeries_.reset(c);
}

void CronAttr::requeue(const ecf::Calendar& c)
{
   clearFree();
   timeSeries_.requeue(c);
}

bool CronAttr::isFree(const ecf::Calendar& c) const
{
	// The FreeDepCmd can be used to free the crons,
	if (makeFree_) {
		return true;
	}

	if (!timeSeries_.isFree(c))  return false;

	// Ok time series if Free

	// ********************************************************************
	// IMPORTANT: when we have multiple week days, days of month and months
	// Then we are *ONLY* free, if *ALL* are free, i.e we need AND behaviour
	// ********************************************************************
	return is_day_of_week_day_of_month_and_month_free(c);
}


bool CronAttr::is_day_of_week_day_of_month_and_month_free( const ecf::Calendar& c) const
{
   bool the_week_day_matches = weekDays_.empty();         // week day matches if no week days
   bool the_day_of_month_matches = daysOfMonth_.empty();  // day of month if no days of month
   bool the_month_matches = months_.empty();              // month matches if no months

   if ( !weekDays_.empty())    the_week_day_matches     = week_day_matches(c.day_of_week());
   if ( !daysOfMonth_.empty()) the_day_of_month_matches = day_of_month_matches(c.day_of_month());
   if ( !months_.empty()  )    the_month_matches        = month_matches(c.month());

   return ( the_week_day_matches && the_day_of_month_matches && the_month_matches) ;
}

bool CronAttr::week_day_matches( int theDayOfWeek ) const
{
 	BOOST_FOREACH(int theWeekDay, weekDays_) {
 		if ( theDayOfWeek == theWeekDay)  return true;
 	}
 	return false;
}

bool CronAttr::day_of_month_matches(int theDayOfMonth) const
{
	BOOST_FOREACH(int dayOfMonth, daysOfMonth_) {
	 	if ( theDayOfMonth == dayOfMonth)  return true;
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
int CronAttr::max_month() const {
	int max = -1;
	BOOST_FOREACH(int month, months_) {
		max = std::max(month,max);
	}
	return max;
}

int CronAttr::max_day_of_month() const
{
	int max = -1;
	BOOST_FOREACH(int dayOfMonth, daysOfMonth_) {
		max = std::max(dayOfMonth,max);
	}
	return max;
}

int CronAttr::max_day_of_week() const
{
	int max = -1;
 	BOOST_FOREACH(int theWeekDay, weekDays_) {
		max = std::max(theWeekDay,max);
  	}
	return max;
}

//-------------------------------------------------------------------------------
int CronAttr::min_month() const{
	int min = std::numeric_limits<int>::max();
	BOOST_FOREACH(int month, months_) {
		min = std::min(month,min);
	}
	return min;
}

int CronAttr::min_day_of_month() const
{
	int min = std::numeric_limits<int>::max();
 	BOOST_FOREACH(int dayOfMonth, daysOfMonth_) {
 		min = std::min(dayOfMonth,min);
	}
	return min;
}

int CronAttr::min_day_of_week() const
{
	int min = std::numeric_limits<int>::max();
  	BOOST_FOREACH(int theWeekDay, weekDays_) {
 		min = std::min(theWeekDay,min);
  	}
	return min;
}
//------------------------------------------------------------


bool CronAttr::checkInvariants(std::string& errormsg) const
{
	return  timeSeries_.checkInvariants(errormsg);
}


//--------------------------------------------------------------

boost::gregorian::date CronAttr::last_day_of_month(const ecf::Calendar& calendar) const
{
	boost::gregorian::date todays_date = calendar.date();
	boost::gregorian::date lastdayOfMonth = todays_date.end_of_month();
	boost::gregorian::date_duration one_day(1);

//	cout << "CronAttr::last_day_of_month  " << calendar.toString() << " \n";

	boost::gregorian::date max_date(neg_infin);
 	while ( todays_date <= lastdayOfMonth ) {

		// deal with case where we have:
		//  	cron -w 0,1 -m 5,6,7,8
		// Find the last Sunday/Monday for *THIS* month
		for (size_t i = 0; i < weekDays_.size(); ++i) {
			if ( todays_date.day_of_week().as_number() == weekDays_[i] ) {
//				cout << "CronAttr::last_day_of_month  ( todays_date.day_of_week().as_number() == weekDays_[i] ) " << weekDays_[i] << "\n";
				if ( todays_date > max_date ) {
					max_date = todays_date;
//					cout << "CronAttr::last_day_of_month  max_date =  " << to_simple_string(max_date) << "\n";
				}

				// The day of week MAY NOT MATCH the day of month
				// deal with case where we have:
				//  	cron -w 0,1 -d 14,15,16
				// Find the last date for *THIS* month
				for (size_t d = 0; d < daysOfMonth_.size(); ++d) {
					if ( todays_date.day() == daysOfMonth_[d] ) {
//						cout << "CronAttr::last_day_of_month **( todays_date.day() == daysOfMonth_[d] ) " << daysOfMonth_[d] << "\n";
						if ( todays_date > max_date ) {
							max_date = todays_date;
//							cout << "CronAttr::last_day_of_month  max_date =  " << to_simple_string(max_date) << "\n";
						}
					}
				}
			}
		}

		// deal with case where we have:
		//  	cron -d 14,15,16 -m 5,6,7,8
		// Find the last date for *THIS* month
		for (size_t d = 0; d < daysOfMonth_.size(); ++d) {
			if ( todays_date.day() == daysOfMonth_[d] ) {
//				cout << "CronAttr::last_day_of_month ( todays_date.day() == daysOfMonth_[d] ) " << daysOfMonth_[d] << "\n";
				if ( todays_date > max_date ) {
					max_date = todays_date;
//					cout << "CronAttr::last_day_of_month  max_date =  " << to_simple_string(max_date) << "\n";
				}
			}
		}

		todays_date += one_day;
	}
	return max_date;
}

boost::gregorian::date CronAttr::next_date(const ecf::Calendar& calendar) const
{
	// Find the next date that matches, day of week, day of year, and month
	// that is greater than todays date. This *ASSUMES* day of week, day of month,
	// and month is *ANDED* together

 	boost::gregorian::date_duration one_day(1);
	boost::gregorian::date future_date = calendar.date();  // todays date
	future_date += one_day;                                // add one day, so its in the future

 	while ( 1 ) {

 		bool week_day_matches = weekDays_.empty();         // week day matches if no week days
 		bool day_of_month_matches = daysOfMonth_.empty();  // day of month if no days of month
 		bool month_matches = months_.empty();              // month matches if no months

		// deal with case where we have: cron -w 0,1
 		for (size_t i = 0; i < weekDays_.size(); ++i) {
			if ( future_date.day_of_week().as_number() == weekDays_[i] ) {
				week_day_matches = true;
				break;
			}
		}

 		// deal with case where we have: cron -w 0,1 -d 14,15,16
 		for (size_t d = 0; d < daysOfMonth_.size(); ++d) {
			if ( future_date.day() == daysOfMonth_[d] ) {
				day_of_month_matches = true;
				break;
 			}
		}

 		// deal with case where we have: cron -w 0,1 -d 14,15,16 -m 8, 9
 		for (size_t m = 0; m < months_.size(); ++m) {
			if ( future_date.month() == months_[m] ) {
				month_matches = true;
				break;
  			}
		}

 		// if it all matches, then return the future day
 		if ( week_day_matches && day_of_month_matches && month_matches) {
 			break; // return future_date, replaced with break to  keep HPUX compiler happy
 			       // otherwise it complains that return at the end of the function is
 			       // unreachable
 		}

		future_date += one_day;
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

std::vector<int> extractOption(
                               size_t& index,
                               const std::vector<std::string >& lineTokens,
                               const std::string& option)
{
	// cron -w 0 -m 5,6,7,8 10:00 20:00 01:00
	assert(index < lineTokens.size());

	// Collate the list of integers, these may have been separated by spaces
	// since we stop on option or time spec, the top level code should decrement index
	std::string theIntList;
	while ( index < lineTokens.size() && ( !isOption(lineTokens[index]) || !isTimeSpec(lineTokens[index]) )) {
		string theNextToken = nextToken(index,lineTokens);
		if (theNextToken.empty()) break;
		if (isOption( theNextToken )) break;
		if (isTimeSpec( theNextToken )) break;
		theIntList +=  theNextToken;
  	}
#ifdef DEBUG_CRON_PARSING
	cerr << "theIntList = " << theIntList << "\n";
#endif

	// should have 0,1,2,3
	std::vector< int > theIntVec;
	char_separator< char > sep( ",", 0, boost::drop_empty_tokens );
	typedef boost::tokenizer< boost::char_separator< char > > tokenizer;
	tokenizer theTokenizer( theIntList, sep );

	for (tokenizer::iterator beg = theTokenizer.begin(); beg != theTokenizer.end(); ++beg) {
		string theIntToken = *beg;
		boost::algorithm::trim( theIntToken );
		if ( theIntToken.empty() ) continue;

		try {
			int theInt = boost::lexical_cast< int >( theIntToken );
 			theIntVec.push_back( theInt );
		}
		catch ( boost::bad_lexical_cast& ) {
			std::stringstream ss; ss << "Invalid cron option: " << option  ;
			throw std::runtime_error( ss.str() );
		}
	}
	return theIntVec;
}

void extractOption(CronAttr& cronAttr, size_t& index, const std::vector<std::string >& lineTokens)
{
	assert(index < lineTokens.size());
	if (lineTokens[index] == "-w") {
 		cronAttr.addWeekDays( extractOption(index, lineTokens, "week days" ) );
 	}
	else if (lineTokens[index] == "-d") {
 		cronAttr.addDaysOfMonth( extractOption(index, lineTokens, "Days of the month" ) );
  	}
	else if (lineTokens[index] == "-m") {
 		cronAttr.addMonths( extractOption(index, lineTokens,"Months" ) );
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

   // make *sure* a time spec is specified
   bool time_spec_specified = false;
 	while (index < lineTokens.size() ) {

		std::string token = lineTokens[index];
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
			   if (index < lineTokens.size() && lineTokens[index] == "#") {
			      index--;
			   }
			}
			else break; // need to read state after comment
 		}
		else if (isComment(token)) {
		   // cron -m 1,2,3 12:00        # free
		   if (parse_state && index+1 < lineTokens.size()) {
		      if ( lineTokens[index+1] == "free") {
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


}

