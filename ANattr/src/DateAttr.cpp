//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #31 $ 
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

#include <assert.h>
#include <sstream>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "DateAttr.hpp"
#include "Extract.hpp"
#include "Indentor.hpp"
#include "Calendar.hpp"
#include "PrintStyle.hpp"
#include "Ecf.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

//==========================================================================================

DateAttr::DateAttr( int day, int month, int year )
: day_( day ), month_( month ), year_( year ), makeFree_( false ), state_change_no_(0)
{
	checkDate(day_,month_,year_,true /* allow wild cards */);
}

bool DateAttr::operator<(const DateAttr& rhs) const {
   if (year_ < rhs.year_) return true;
   if (year_ == rhs.year_) {
      if (month_ < rhs.month_ ) return true;
      if (month_ == rhs.month_) {
         return day_ < rhs.day_;
      }
   }
   return false;
}

void DateAttr::checkDate(int day, int month, int year, bool allow_wild_cards)
{
	if (allow_wild_cards) {
		if (day != 0 &&  (day < 1 || day > 31 )) {
			throw std::out_of_range( "Invalid Date(day,month,year) : the day >= 0 and day < 31, where 0 means wild card ");
		}
		if (month !=0 && ( month < 1 || month > 12 )) {
			throw std::out_of_range( "Invalid Date(day,month,year): the month >=0 and month <= 12, where 0 means wild card");
		}
		if (year < 0) {
			throw std::out_of_range( "Invalid Date(day,month,year): the year >=0, where 0 means wild card");
		}
	}
	else {
		if (day < 1 || day > 31 ) {
			throw std::out_of_range( "Invalid date attribute : the day >= 1 and day < 31");
		}
		if (month < 1 || month > 12 ) {
			throw std::out_of_range( "Invalid date attribute: the month >=1 and month <= 12");
		}
		if (year <= 0) {
			throw std::out_of_range( "Invalid date attribute: the year >0");
		}
	}

	if ( day != 0 && month != 0 && year != 0 ) {

		// let boost validate the date
		boost::gregorian::date theDate( year, month, day );
	}
}

void DateAttr::calendarChanged( const ecf::Calendar& c )
{
   // See ECFLOW-337
   if (c.dayChanged()) {
      clearFree();
   }

   if (makeFree_) {
      return;
   }
   else if (isFree(c)) {
      setFree();
   }
}

void DateAttr::setFree() {
	makeFree_ = true;
	state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "DateAttr::setFree()\n";
#endif
}

void DateAttr::clearFree() {
	makeFree_ = false;
	state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "DateAttr::clearFree()\n";
#endif
}

bool DateAttr::isFree(const ecf::Calendar& calendar) const
{
	// The FreeDepCmd can be used to free the dates,
 	if (makeFree_) {
		return true;
	}
 	return is_free(calendar);
}

bool DateAttr::is_free(const ecf::Calendar& calendar) const
{
   bool dayMatches = true;
   bool monthMatches = true;
   bool yearMatches = true;
   if (day_ != 0)   dayMatches   = calendar.day_of_month() == day_;
   if (month_ != 0) monthMatches = calendar.month()        == month_;
   if (year_ != 0)  yearMatches  = calendar.year()         == year_;

   return ( dayMatches && monthMatches && yearMatches);
}

bool DateAttr::checkForRequeue( const ecf::Calendar& calendar) const
{
	// if calendar is hybrid, we can't re-queue
	if (calendar.hybrid()) {
 		return false;
	}

	// checkForRequeue is called when we are deciding whether to re-queue the node.
	// If this date is in the future, they we should re-queue
	if ( day_ != 0 && month_ != 0 && year_ != 0) {
		date theDate(year_,month_,day_);
		if (theDate >  calendar.date()) {
//#ifdef DEBUG
//			cout << toString() << "   > " << " calendar date " << to_simple_string( calendar.date()) << "\n";
//#endif
			return true;
		}
		return false;
	}

 	bool futureDayMatches = true;
 	bool futureMonthMatches = true;
 	bool futureYearMatches = true;
 	if (day_ != 0)   futureDayMatches   = day_ >   calendar.day_of_month() ;
 	if (month_ != 0) futureMonthMatches = month_ > calendar.month() ;
 	if (year_ != 0)  futureYearMatches  = year_ >  calendar.year() ;

//#ifdef DEBUG
//  	if ( futureDayMatches ) {
// 		cout << "futureDayMatches " << toString() << " > " << calendar.day_of_month() << "\n";
// 	}
//	if ( futureMonthMatches ) {
// 		cout << "futureMonthMatches " << toString() << " > " << calendar.month() << "\n";
// 	}
//	if ( futureYearMatches ) {
// 		cout << "futureYearMatches " << toString() << " > " << calendar.year() << "\n";
// 	}
//#endif

 	return ( futureDayMatches || futureMonthMatches || futureYearMatches);
}

bool DateAttr::validForHybrid(const ecf::Calendar& calendar) const
{
 	if (day_ == 0)    return false; // relies on day change i.e. date *.10.2009
 	if (month_ == 0)  return false; // relies on day change i.e. date 12.*.2009
 	if (year_ == 0)   return false; // relies on day change i.e. date 12.10.*

 	// if the date matches exactly for today
   	return (day_ == calendar.day_of_month() && month_ == calendar.month() && year_ == calendar.year() );
}

bool DateAttr::why(const ecf::Calendar& c, std::string& theReasonWhy) const
{
	if (isFree(c)) return false;

	std::stringstream ss;
	ss << " is date dependent ( next run on " << toString() << " the current date is ";
	ss << c.day_of_month() << "/" << c.month() << "/" << c.year() << " )";
	theReasonWhy += ss.str();
	return true;
}

std::ostream& DateAttr::print(std::ostream& os) const
{
	Indentor in;
	Indentor::indent(os) << toString();
   if (!PrintStyle::defsStyle()) {
      if (makeFree_) os << " # free";
   }
	os << "\n";
	return os;
}

std::string DateAttr::toString() const
{
	std::stringstream ss;
	ss << "date ";
	if (day_ == 0) ss << "*.";
	else           ss << day_ << ".";

	if (month_ == 0) ss << "*.";
	else             ss << month_ << ".";

	if (year_ == 0) ss << "*";
	else            ss << year_;

	return ss.str();
}

std::string DateAttr::dump() const
{
	std::stringstream ss; ss << toString();
 	if (makeFree_) ss << " (free)";
	else           ss << " (holding)";
	return ss.str();
}


bool DateAttr::operator==(const DateAttr& rhs) const
{
	if (makeFree_ != rhs.makeFree_) {
		return false;
	}
	return structureEquals(rhs);
}
bool DateAttr::structureEquals(const DateAttr& rhs) const
{
	if (day_ != rhs.day_) return false;
	if (month_ != rhs.month_) return false;
	if (year_ != rhs.year_) return false;
	return true;
}

DateAttr DateAttr::create(const std::string& dateString)
{
	int day = -1,month = -1,year = -1;
	getDate(dateString,day,month,year);
	return DateAttr(day,month,year);
}

void DateAttr::getDate(const std::string& date,int& day,int& month,int& year)
{
	size_t firstDotPos = date.find_first_of('.');
	size_t lastDotPos = date.find_first_of('.',firstDotPos+1);
	if (firstDotPos == std::string::npos)  throw std::runtime_error( "DateAttr::getDate Invalid date missing first dot :" + date );
	if (lastDotPos == std::string::npos)  throw std::runtime_error( "DateAttr::getDate: Invalid date missing second dot :" + date );
	if (firstDotPos ==  lastDotPos) throw std::runtime_error( "DateAttr::getDate: Invalid date :" + date );


	std::string theDay = date.substr(0,firstDotPos);
	std::string theMonth = date.substr(firstDotPos+1, (lastDotPos-firstDotPos)-1);
	std::string theYear = date.substr(lastDotPos+1);

	if (theDay == "*") day = 0;
	else  {
		day = Extract::theInt(theDay,"DateAttr::getDate: Invalid day :" + date);
		if (day < 1 || day > 31)  throw std::runtime_error("DateAttr::getDate: Invalid clock date: " + date );
	}


	if (theMonth == "*") month = 0;
	else  {
		month = Extract::theInt(theMonth,"DateAttr::getDate: Invalid month :" + date);
		if (month < 1 || month > 12)  throw std::runtime_error("DateAttr::getDate Invalid clock date: " + date );
	}

	if (theYear == "*") year = 0;
	else                year = Extract::theInt(theYear,"DateAttr::getDate: Invalid year :" + date);


	if (day == -1 || month == -1 || year == -1)  throw std::runtime_error("DateAttr::getDate: Invalid clock date:" + date );

	// let boost validate the date
	if ( day != 0 && month != 0 && year != 0) {

		boost::gregorian::date theDate(year,month,day);
	}

//	cerr << " DateParser::getDate date=" << date << " day=" << day << " month=" << month << " year=" << year << "\n";
}

