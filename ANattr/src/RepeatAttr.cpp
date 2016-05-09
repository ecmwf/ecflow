//============================================================================
// Name        : NodeTree.cpp
// Author      : Avi
// Revision    : $Revision: #57 $ 
//
// Copyright 2009-2016 ECMWF. 
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

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>  // requires boost date and time lib
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "RepeatAttr.hpp"
#include "Indentor.hpp"
#include "Ecf.hpp"
#include "Log.hpp"
#include "Str.hpp"
#include "PrintStyle.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

const Repeat& Repeat::EMPTY() { static const Repeat REPEAT = Repeat(); return REPEAT; }

//=========================================================================

Repeat::Repeat() : repeatType_(NULL) {}
Repeat::Repeat( const RepeatDate& r) : repeatType_(new RepeatDate(r)) {}
Repeat::Repeat( const RepeatInteger& r) : repeatType_(new RepeatInteger(r)) {}
Repeat::Repeat( const RepeatEnumerated& r) : repeatType_(new RepeatEnumerated(r)) {}
Repeat::Repeat( const RepeatString& r) : repeatType_(new RepeatString(r)) {}
Repeat::Repeat( const RepeatDay& r) : repeatType_(new RepeatDay(r)) {}

Repeat::~Repeat() { delete repeatType_;}

Repeat::Repeat( const Repeat& rhs) : repeatType_(NULL)
{
	// Do stuff that could throw exception first
	RepeatBase* clone = NULL;
	if ( rhs.repeatType_) {
		clone = rhs.repeatType_->clone();
	}

	// Change state
 	repeatType_ = clone;
}

Repeat& Repeat::operator=(const Repeat& rhs)
{
	// Do stuff that could throw exception first
	RepeatBase* clone = NULL;
	if ( rhs.repeatType_) {
		clone = rhs.repeatType_->clone();
	}

	// Change state
	delete repeatType_; repeatType_ = NULL;
	repeatType_ = clone;

	return *this;
}

bool Repeat::operator==(const Repeat& rhs) const
{
	if (!repeatType_ && rhs.repeatType_) return false;
	if (repeatType_ && !rhs.repeatType_) return false;
	if (!repeatType_ && !rhs.repeatType_) return true	;
	return repeatType_->compare(rhs.repeatType_);
}

const std::string& Repeat::name() const {
   return (repeatType_) ? repeatType_->name() : Str::EMPTY();
}

const Variable& Repeat::gen_variable() const
{
   return (repeatType_) ? repeatType_->gen_variable() : Variable::EMPTY();
}

void Repeat::update_repeat_genvar() const
{
   if (repeatType_) {
      // **** reset name since generated variables are not persisted
      repeatType_->set_gen_variable().set_name( repeatType_->name() );

      // valueAsString() use the last_valid_value() which should always be in range.
      // Note repeat::value() can be on e past the last valid value, at expiration of Repeat loop
      //      However Repeat::last_valid_value() will just return the last valid value.
      repeatType_->set_gen_variable().set_value( repeatType_->valueAsString() );
   }
}


std::ostream& Repeat::print( std::ostream& os ) const {
	if (repeatType_) {
		Indentor in;
		Indentor::indent(os) << toString() << "\n";
	}
	return os;
}

// =========================================================================
RepeatBase::~RepeatBase() {}

void RepeatBase::incr_state_change_no()
{
	state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "RepeatBase::incr_state_change_no()\n";
#endif
}

// =========================================================================

long sms_repeat_julian_to_date(long jdate)
{
   long x,y,d,m,e;
   long day,month,year;

   x = 4 * jdate - 6884477;
   y = (x / 146097) * 100;
   e = x % 146097;
   d = e / 4;

   x = 4 * d + 3;
   y = (x / 1461) + y;
   e = x % 1461;
   d = e / 4 + 1;

   x = 5 * d - 3;
   m = x / 153 + 1;
   e = x % 153;
   d = e / 5 + 1;

   if( m < 11 )
      month = m + 2;
   else
      month = m - 10;


   day = d;
   year = y + m / 11;

   return year * 10000 + month * 100 + day;
}

long sms_repeat_date_to_julian(long ddate)
{
   long  m1,y1,a,b,c,d,j1;
   long month,day,year;

   year = ddate / 10000;
   ddate %= 10000;
   month  = ddate / 100;
   ddate %= 100;
   day = ddate;

   if (month > 2)
   {
      m1 = month - 3;
      y1 = year;
   }
   else
   {
      m1 = month + 9;
      y1 = year - 1;
   }
   a = 146097*(y1/100)/4;
   d = y1 % 100;
   b = 1461*d/4;
   c = (153*m1+2)/5+day+1721119;
   j1 = a+b+c;

   return(j1);
}

RepeatDate::RepeatDate( const std::string& variable,
                        int start,
                        int end,
                        int delta /* always in days*/
) : RepeatBase(variable), start_(start), end_(end), delta_(delta), value_(start)
{
   if ( !Str::valid_name( variable ) ) {
      throw std::runtime_error("RepeatDate::RepeatDate: Invalid name: " + variable);
   }

   if (delta == 0) {
      std::stringstream ss; ss << "repeat " << variable << " " << start << " " << end << " " << delta;
      throw std::runtime_error("Invalid Repeat date: the delta can not be zero" + ss.str());
   }

   std::string theStart = boost::lexical_cast< std::string >(start);
   if (theStart.size() != 8) {
      std::stringstream ss; ss << "repeat " << variable << " " << start << " " << end << " " << delta;
      throw std::runtime_error("Invalid Repeat date: The start is not a valid date. Please use yyyymmdd format." + ss.str());
   }
   std::string theEnd = boost::lexical_cast< std::string >(end);
   if (theEnd.size() != 8) {
      std::stringstream ss; ss << "repeat " << variable << " " << start << " " << end << " " << delta;
      throw std::runtime_error("Invalid Repeat date: The end is not a valid date. Please use yyyymmdd format." + ss.str());
   }


   if (delta_ > 0) {
      // assert end => start
      if (!(end >= start)) {
         std::stringstream ss; ss << "repeat " << variable << " " << start << " " << end << " " << delta;
         throw std::runtime_error("Invalid Repeat date: The end must be greater than the start date, when delta is positive " + ss.str());
      }
   }
   else {
      // assert start >= end
      if (!(start >= end)) {
         std::stringstream ss; ss << "repeat " << variable << " " << start << " " << end << " " << delta;
         throw std::runtime_error("Invalid Repeat date: The start must be greater than the end date, when delta is negative " + ss.str());
      }
   }

	// Use date lib to check YMD
	try {
		boost::gregorian::date(from_undelimited_string(theStart));
		boost::gregorian::date(from_undelimited_string(theEnd));
	}
	catch (std::exception& e) {
      std::stringstream ss; ss << "repeat " << variable << " " << start << " " << end << " " << delta;
		throw std::runtime_error("Invalid Repeat date: The start/end is not a valid date." + ss.str());
	}
}

bool RepeatDate::compare(RepeatBase* rb) const
{
	RepeatDate* rhs = dynamic_cast<RepeatDate*>(rb);
	if(!rhs) return false;
	return operator==(*rhs);
}

void RepeatDate::setToLastValue()
{
   value_ = end_;
	incr_state_change_no();
}

long RepeatDate::last_valid_value() const
{
   if (delta_ > 0) {
      if (value_ < start_) return start_;
      if (value_ > end_)   return end_;
      return value_;
   }
   if (value_ > start_) return start_;
   if (value_ < end_)   return end_;
   return value_;
}

long RepeatDate::last_valid_value_minus(int val) const
{
   long last_value = last_valid_value();
   long julian = sms_repeat_date_to_julian(last_value);
   julian -= val;
   return sms_repeat_julian_to_date(julian);
}

long RepeatDate::last_valid_value_plus(int val) const
{
   long last_value = last_valid_value();
   long julian = sms_repeat_date_to_julian(last_value);
   julian += val;
   return sms_repeat_julian_to_date(julian);
}

void RepeatDate::reset() {
	value_ = start_;
	incr_state_change_no();
}

std::string RepeatDate::toString() const
{
	std::stringstream ss;
	ss << "repeat date " << name_ << " " << start_ << " " << end_ << " " << delta_;
	if (!PrintStyle::defsStyle() && (value_ != start_)) {
	   ss << " # " << value_;
	}
	return ss.str();
}
std::string RepeatDate::dump() const
{
	std::stringstream ss;
	ss << toString() << " value(" << value_ << ")";
	return ss.str();
}

bool RepeatDate::operator==(const RepeatDate& rhs) const
{
	if (name_ != rhs.name_) {
		return false;
	}
	if (start_ != rhs.start_) {
		return false;
	}
	if (end_ != rhs.end_) {
		return false;
	}
	if (delta_ != rhs.delta_) {
		return false;
	}
   if (value_ != rhs.value_) {
      return false;
   }
	return true;
}

std::string RepeatDate::valueAsString() const
{
	/// will throw a boost::bad_lexical_cast& if value is not convertible to a string
 	try {
		return boost::lexical_cast< std::string >( last_valid_value() );
	}
	catch ( boost::bad_lexical_cast& ) {
		LOG_ASSERT(false,"RepeatDate::valueAsString(): could not convert value " << value_ << " to a string");
	}
	return string();
}

std::string RepeatDate::value_as_string(int index) const
{
   /// will throw a boost::bad_lexical_cast& if value is not convertible to a string
   try {
      return boost::lexical_cast< std::string >( index );
   }
   catch ( boost::bad_lexical_cast& ) {}
   return string();
}

void RepeatDate::increment()
{
   long julian = sms_repeat_date_to_julian(value_);
   julian += delta_;
   value_ = sms_repeat_julian_to_date(julian);

   incr_state_change_no();
}

void RepeatDate::change( const std::string& newdate)
{
 	if (newdate.size() != 8) {
      std::stringstream ss;
      ss << "RepeatDate::change: " << toString() << " The new date is not valid, expected 8 characters in yyyymmdd format but found " << newdate;
		throw std::runtime_error(ss.str());
 	}

	long the_new_date = 0;
 	try {
 		the_new_date = boost::lexical_cast< long >( newdate );
	}
	catch ( boost::bad_lexical_cast& ) {
      std::stringstream ss;
      ss << "RepeatDate::change: " << toString() << " The new date(" << newdate << ") is not convertible to an long";
 		throw std::runtime_error(ss.str());
	}

	// Use date lib to check YMD
	try { boost::gregorian::date(from_undelimited_string(newdate));}
	catch (std::exception& e) {
      std::stringstream ss;
      ss << "RepeatDate::change: " << toString() << " The new date(" << newdate << ") is not valid";
 		throw std::runtime_error(ss.str());
	}

	changeValue(the_new_date);
}

void RepeatDate::changeValue(long the_new_date)
{
   if (delta_ > 0) {
      if (the_new_date < start_ || the_new_date > end_) {
         std::stringstream ss;
         ss << "RepeatDate::changeValue: " << toString() << "\nThe new date should be in the range[" << start_ << " : " << end_ << "] but found " << the_new_date;
         throw std::runtime_error(ss.str());
      }
   }
   else {
      if (the_new_date > start_ || the_new_date < end_) {
         std::stringstream ss;
         ss << "RepeatDate::changeValue: " << toString() << "\nThe new date should be in the range[" << start_ << " : " << end_ << "] but found " << the_new_date;
         throw std::runtime_error(ss.str());
      }
   }

   // Check new value is in step. ECFLOW-325 repeat date 7
   long julian_new_date = sms_repeat_date_to_julian(the_new_date);
   long julian_start = sms_repeat_date_to_julian(start_);
   long diff = julian_new_date - julian_start;
   if ( diff % delta_ != 0 ) {
       std::stringstream ss;
       ss << "RepeatDate::changeValue: " << toString() << "\nThe new date " << the_new_date << " is not in line with the delta/step";
       throw std::runtime_error(ss.str());
   }

	set_value(the_new_date);
}

void RepeatDate::set_value(long the_new_date)
{
   // Note: the node is incremented one past, the last value
   // In Node we increment() then check for validity
   // hence the_new_value may be outside of the valid range.
   // This can be seen when do a incremental sync,
   // *hence* allow memento to copy the value as is.
   value_ = the_new_date;
   incr_state_change_no();
}

//======================================================================================

RepeatInteger::RepeatInteger( const std::string& variable, int start, int end, int delta ) :
	RepeatBase( variable ), start_( start ), end_( end ), delta_( delta ), value_( start )
{
//	cout << toString() << "\n";
   if ( !Str::valid_name( variable ) ) {
      throw std::runtime_error("RepeatInteger: Invalid name: " + variable);
   }
}
RepeatInteger::RepeatInteger() : start_( 0 ), end_( 0 ), delta_( 0 ), value_( 0 ) {}

bool RepeatInteger::compare(RepeatBase* rb) const
{
	RepeatInteger* rhs = dynamic_cast<RepeatInteger*>(rb);
	if(!rhs) return false;
	return operator==(*rhs);
}

void RepeatInteger::reset() {
	value_ = start_;
	incr_state_change_no();
}

long RepeatInteger::last_valid_value() const
{
   if (delta_ > 0) {
      if (value_ < start_) return start_;
      if (value_ > end_)   return end_;
      return value_;
   }
   if (value_ > start_) return start_;
   if (value_ < end_)   return end_;
   return value_;
}

void RepeatInteger::increment() {
	value_ += delta_;
	incr_state_change_no();
}

void RepeatInteger::change( const std::string& newValue)
{
 	long the_new_value = 0;
 	try {
 		the_new_value = boost::lexical_cast< long >( newValue );
	}
	catch ( boost::bad_lexical_cast& ) {
      std::stringstream ss;
      ss << "RepeatInteger::change:" << toString() << " The new value(" << newValue << ") is not convertible to an long";
 		throw std::runtime_error( ss.str() );
	}
	changeValue(the_new_value);
}

void RepeatInteger::changeValue(long the_new_value)
{
	if (delta_ > 0) {
		if (the_new_value < start_ || the_new_value > end_ ) {
			std::stringstream ss;
			ss << "RepeatInteger::changeValue:" << toString() << ". The new value should be in the range[" << start_ << "-" << end_ << "] but found " << the_new_value;
			throw std::runtime_error(ss.str());
		}
	}
	else {
      if (the_new_value > start_ || the_new_value < end_ ) {
         std::stringstream ss;
         ss << "RepeatInteger::changeValue:" << toString() << ". The new value should be in the range[" << start_ << "-" << end_ << "] but found " << the_new_value;
         throw std::runtime_error(ss.str());
      }
	}
	set_value(the_new_value);
}

void RepeatInteger::set_value(long the_new_value)
{
   // To be used by Memento only. as it does no checking
   // Note: the node is incremented one past, the last value
   // In Node we increment() then check for validity
   // hence the_new_value may be outside of the valid range.
   // This can be seen when do a incremental sync,
   // *hence* allow memento to copy the value as is.
   value_ = the_new_value;
   incr_state_change_no();
}

void RepeatInteger::setToLastValue()
{
   value_ = end_;
	incr_state_change_no();
}

std::string RepeatInteger::toString() const
{
	std::stringstream ss;
	ss << "repeat integer " << name_ << " " << start_ << " " << end_;
 	if (delta_ != 1) ss << " " << delta_;
   if (!PrintStyle::defsStyle() && (value_ != start_)) {
      ss << " # " << value_;
   }
	return ss.str();
}
std::string RepeatInteger::dump() const
{
	std::stringstream ss;
	ss << toString() << " value(" << value_ << ")";
	return ss.str();
}

bool RepeatInteger::operator==(const RepeatInteger& rhs) const
{
	if (name_ != rhs.name_) {
		return false;
	}
	if (start_ != rhs.start_) {
		return false;
	}
	if (end_ != rhs.end_) {
		return false;
	}
	if (delta_ != rhs.delta_) {
		return false;
	}
   if (value_ != rhs.value_) {
      return false;
   }
	return true;
}

std::string RepeatInteger::valueAsString() const
{
	/// will throw a boost::bad_lexical_cast& if value is not convertible to a string
 	try {
		return boost::lexical_cast< std::string >( last_valid_value() );
	}
	catch ( boost::bad_lexical_cast& ) { LOG_ASSERT(false,"");}
	return string();
}

std::string RepeatInteger::value_as_string(int index) const
{
   /// will throw a boost::bad_lexical_cast& if value is not convertible to a string
   try {
      return boost::lexical_cast< std::string >( index );
   }
   catch ( boost::bad_lexical_cast& ) {}
   return string();
}

//void RepeatInteger::truncate(int theLength)
//{
////	 cout << "   RepeatInteger::truncate by " <<  theLength << " BEFORE " << toString();
//
//	LOG_ASSERT(theLength < length(),"");
//	 end_ = start_ + (theLength * delta_);
//
////	 cout << " AFTER " << toString() << "\n";
//}

//======================================================================================

RepeatEnumerated::RepeatEnumerated( const std::string& variable, const std::vector<std::string>& theEnums)
: RepeatBase(variable), theEnums_(theEnums), currentIndex_(0)
{
   if ( !Str::valid_name( variable ) ) {
      throw std::runtime_error("RepeatEnumerated: Invalid name: " + variable);
   }
}

int RepeatEnumerated::end() const   {
	if ( theEnums_.empty() ) return 0;
	return static_cast<int>(theEnums_.size()-1);
}

bool RepeatEnumerated::compare(RepeatBase* rb) const
{
	RepeatEnumerated* rhs = dynamic_cast<RepeatEnumerated*>(rb);
	if(!rhs) return false;
	return operator==(*rhs);
}

std::string RepeatEnumerated::toString() const
{
	std::stringstream ss;
 	ss << "repeat enumerated " << name_;
 	BOOST_FOREACH(const string& s, theEnums_) { ss << " \"" << s << "\""; }
 	if (!PrintStyle::defsStyle() && (currentIndex_ != 0)) {
 	   ss << " # " << currentIndex_;
 	}
  	return ss.str();
}
std::string RepeatEnumerated::dump() const
{
	std::stringstream ss;
	ss << toString() << " ordinal-value(" << value() << ")   value-as-string(" << valueAsString() << ")";
 	return ss.str();
}

void RepeatEnumerated::reset() {
	currentIndex_ = 0;
	incr_state_change_no();
}

void RepeatEnumerated::increment() {
	currentIndex_++;
	incr_state_change_no();
}

long RepeatEnumerated::value() const
{
   if (currentIndex_ >= 0 && currentIndex_ < static_cast<int>(theEnums_.size()) ) {
      try {
         return boost::lexical_cast<int>( theEnums_[currentIndex_] );
      }
      catch ( boost::bad_lexical_cast& ) {
         // Ignore and return currentIndex_
      }
   }
   return currentIndex_;
}

long RepeatEnumerated::last_valid_value() const
{
   if (!theEnums_.empty()) {
      if (currentIndex_ < 0) {
         try { return boost::lexical_cast<int>( theEnums_[0] ); }
         catch ( boost::bad_lexical_cast& ) { /* Ignore and return first index */ }
         return 0;
      }
      if (currentIndex_ >= static_cast<int>(theEnums_.size())) {

         try { return boost::lexical_cast<int>( theEnums_[theEnums_.size()-1] ); }
         catch ( boost::bad_lexical_cast& ) { /* Ignore and return last index */ }
         return static_cast<long>(theEnums_.size()-1);
      }

      // return current value as integer or as index
      return value();
   }
   return 0;
}

void RepeatEnumerated::setToLastValue()
{
	currentIndex_ = static_cast< int > ( theEnums_.size()  - 1);
	if (currentIndex_ < 0) currentIndex_ = 0;
	incr_state_change_no();
}

std::string RepeatEnumerated::valueAsString() const
{
   // This must always return a valid value
   if (!theEnums_.empty()) {

      // Returns the last valid value
      if (currentIndex_ < 0)
         return theEnums_[0]; // return first

      if (currentIndex_ >= static_cast<int>(theEnums_.size())) {
         return theEnums_[theEnums_.size()-1]; // return last
      }

      return theEnums_[currentIndex_];
   }
   return std::string();
}

std::string RepeatEnumerated::value_as_string(int index) const
{
   if (index >= 0 && index < static_cast<int>(theEnums_.size())) {
      return theEnums_[index];
   }
   return std::string();
}

void RepeatEnumerated::change( const std::string& newValue)
{
	// See if if matches one of the enums
	for(size_t i = 0; i < theEnums_.size(); i++) {
		if ( theEnums_[i] == newValue) {
			currentIndex_ = i;
			incr_state_change_no();
			return;
		}
	}

	// If the value is convertible to an integer, treat as an index
  	try {
 		long the_new_value = boost::lexical_cast< long >( newValue );
 		changeValue(the_new_value); // can throw if out of range
 		return;
	}
	catch ( boost::bad_lexical_cast& ) {}


	std::stringstream ss;
	ss << "RepeatEnumerated::change:" << toString() << "\nThe new value " << newValue << " is not a valid index or a member of the enumerated list\n";
	throw std::runtime_error(ss.str());
}

void RepeatEnumerated::changeValue( long the_new_value)
{
 	if ( the_new_value < 0 || the_new_value >= static_cast<int>(theEnums_.size())) {
		std::stringstream ss;
		ss << "RepeatEnumerated::changeValue:" << toString() << "\nThe new value '" << the_new_value << "' is not a valid index ";
		ss << "expected range[0-" << theEnums_.size()-1 << "] but found '" << the_new_value << "'";
		throw std::runtime_error( ss.str() );
	}
 	set_value(the_new_value);
}

void RepeatEnumerated::set_value(long the_new_value)
{
   // Note: the node is incremented one past, the last value
   // In Node we increment() then check for validity
   // hence the_new_value may be outside of the valid range.
   // This can be seen when do a incremental sync,
   // *hence* allow memento to copy the value as is.
   currentIndex_ = the_new_value;
   incr_state_change_no();
}

bool RepeatEnumerated::operator==(const RepeatEnumerated& rhs) const
{
	if (name_ != rhs.name_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "RepeatEnumerated::operator==( name_(" << name_ << ") != rhs.name_(" << rhs.name_ << "))\n";
      }
#endif
		return false;
	}
	if (theEnums_ != rhs.theEnums_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "RepeatEnumerated::operator==( theEnums_ != rhs.theEnums_ )\n";
      }
#endif
		return false;
	}
   if (currentIndex_ != rhs.currentIndex_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "RepeatEnumerated::operator==( currentIndex_(" << currentIndex_ << ") != rhs.currentIndex_(" << rhs.currentIndex_ << "))\n";
      }
#endif
      return false;
   }
	return true;
}

//void RepeatEnumerated::truncate(int theLength)
//{
////	 cout << "   RepeatEnumerated::truncate by " <<  theLength << " BEFORE " << toString();
//
//	 LOG_ASSERT(theLength < length(),"");
//	 while ( static_cast<int>(theEnums_.size()) > theLength) {
//		 theEnums_.pop_back();
//	 }
//
////	 cout << " AFTER " << toString() << "\n";
//}

//======================================================================================

RepeatString::RepeatString( const std::string& variable, const std::vector<std::string>& theEnums)
: RepeatBase(variable), theStrings_(theEnums), currentIndex_(0)
{
   if ( !Str::valid_name( variable ) ) {
      throw std::runtime_error("RepeatString:: Invalid name: " + variable);
   }
}

int RepeatString::end() const   {
	if ( theStrings_.empty() ) return 0;
	return static_cast<int>(theStrings_.size()-1);
}

bool RepeatString::compare(RepeatBase* rb) const
{
	RepeatString* rhs = dynamic_cast<RepeatString*>(rb);
	if(!rhs) return false;
	return operator==(*rhs);
}

std::string RepeatString::toString() const
{
	std::stringstream ss;
	ss << "repeat string " << name_;
 	BOOST_FOREACH(const string& s, theStrings_) { ss << " \"" << s << "\""; }
   if (!PrintStyle::defsStyle() && (currentIndex_ != 0)) {
      ss << " # " << value();
   }
 	return ss.str();
}
std::string RepeatString::dump() const
{
	std::stringstream ss;
	ss << toString() << " ordinal-value(" << value() << ")   value-as-string(" << valueAsString() << ")";
 	return ss.str();
}

void RepeatString::reset() {
	currentIndex_ = 0;
	incr_state_change_no();
}

long RepeatString::last_valid_value() const
{
   if (!theStrings_.empty()) {
      if (currentIndex_ < 0)  return 0;
      if (currentIndex_ >= static_cast<int>(theStrings_.size())) return static_cast<long>(theStrings_.size()-1);
      return currentIndex_;
   }
   return 0;
}

void RepeatString::increment() {
	currentIndex_++;
	incr_state_change_no();
}

void RepeatString::setToLastValue() {
	currentIndex_ = static_cast<int>(theStrings_.size()-1);
   if (currentIndex_ < 0) currentIndex_ = 0;
	incr_state_change_no();
}

std::string RepeatString::valueAsString() const
{
   if (!theStrings_.empty()) return theStrings_[last_valid_value()];
   return std::string();
}

std::string RepeatString::value_as_string(int index) const
{
   if (index >= 0 && index < static_cast<int>(theStrings_.size())) {
      return theStrings_[index];
   }
   return std::string();
}

void RepeatString::change( const std::string& newValue)
{
	// See if if matches one of the strings
	for(size_t i = 0; i < theStrings_.size(); i++) {
		if ( theStrings_[i] == newValue) {
			currentIndex_ = i;
			incr_state_change_no();
			return;
		}
	}

	// If the value is convertible to an integer, treat as an index
  	try {
 		long the_new_value = boost::lexical_cast< int >( newValue );
 		changeValue(the_new_value);
 		return;
	}
	catch ( boost::bad_lexical_cast& ) {}

	std::stringstream ss;
	ss << "RepeatString::change: " << toString() << "\nThe new value " << newValue << " is not a valid index or member of the string list";
	throw std::runtime_error(ss.str());
}

void RepeatString::changeValue( long the_new_value)
{
	if ( the_new_value < 0 || the_new_value >= static_cast<int>(theStrings_.size())) {
		std::stringstream ss;
		ss << "RepeatString::change: " << toString() << " The new the integer " << the_new_value << " is not a valid index ";
		ss << "expected range[0-" << theStrings_.size()-1 << "]'";
		throw std::runtime_error( ss.str() );
	}
	set_value(the_new_value);
}

void RepeatString::set_value(long the_new_value)
{
   // Note: the node is incremented one past, the last value
   // In Node we increment() then check for validity
   // hence the_new_value may be outside of the valid range.
   // This can be seen when do a incremental sync,
   // *hence* allow memento to copy the value as is.
   currentIndex_ = the_new_value;
   incr_state_change_no();
}

bool RepeatString::operator==(const RepeatString& rhs) const
{
	if (name_ != rhs.name_) {
		return false;
	}
	if (theStrings_ != rhs.theStrings_) {
		return false;
	}
   if (currentIndex_ != rhs.currentIndex_) {
      return false;
   }
	return true;
}

//void RepeatString::truncate(int theLength)
//{
////	 cout << "   RepeatString::truncate by " <<  theLength << " BEFORE " << toString();
//
//	 LOG_ASSERT(theLength < length(),"");
//	 while ( static_cast<int>(theStrings_.size()) > theLength) {
//		 theStrings_.pop_back();
//	 }
//
////	 cout << " AFTER " << toString() << "\n";
//}

//=======================================================================================

bool RepeatDay::compare(RepeatBase* rb) const
{
	RepeatDay* rhs = dynamic_cast<RepeatDay*>(rb);
	if(!rhs) return false;
	return operator==(*rhs);
}

std::string RepeatDay::toString() const
{
	std::stringstream ss;
	ss << "repeat day " << step_;
 	return ss.str();
}

std::string RepeatDay::dump() const
{
	return toString();
}

bool RepeatDay::operator==(const RepeatDay& rhs) const
{
	if (step_ != rhs.step_) {
		return false;
	}
	return true;
}

BOOST_CLASS_EXPORT_IMPLEMENT(RepeatDate);
BOOST_CLASS_EXPORT_IMPLEMENT(RepeatInteger);
BOOST_CLASS_EXPORT_IMPLEMENT(RepeatEnumerated);
BOOST_CLASS_EXPORT_IMPLEMENT(RepeatString);
BOOST_CLASS_EXPORT_IMPLEMENT(RepeatDay);
