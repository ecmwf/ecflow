//============================================================================
// Name        : NodeTree.cpp
// Author      : Avi
// Revision    : $Revision: #57 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include <cassert>
#include <sstream>
#include <ostream>

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
#include "Cal.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

const Repeat& Repeat::EMPTY() { static const Repeat REPEAT = Repeat(); return REPEAT; }

//=========================================================================

Repeat::Repeat() = default;
Repeat::Repeat( const RepeatDate& r) : type_(new RepeatDate(r)) {}
Repeat::Repeat( const RepeatInteger& r) : type_(new RepeatInteger(r)) {}
Repeat::Repeat( const RepeatEnumerated& r) : type_(new RepeatEnumerated(r)) {}
Repeat::Repeat( const RepeatString& r) : type_(new RepeatString(r)) {}
Repeat::Repeat( const RepeatDay& r) : type_(new RepeatDay(r)) {}

Repeat::~Repeat() = default;

Repeat::Repeat( const Repeat& rhs)
{
	if ( rhs.type_) {
	   type_.reset( rhs.type_->clone());
	}
}

Repeat& Repeat::operator=(const Repeat& rhs)
{
   type_.reset(nullptr);
	if ( rhs.type_) {
      type_.reset( rhs.type_->clone());
	}
	return *this;
}

bool Repeat::operator==(const Repeat& rhs) const
{
	if (!type_ && rhs.type_) return false;
	if (type_ && !rhs.type_) return false;
	if (!type_ && !rhs.type_) return true	;
	return type_->compare(rhs.type_.get());
}

const std::string& Repeat::name() const {
   return (type_.get()) ? type_->name() : Str::EMPTY();
}

std::ostream& Repeat::print( std::ostream& os ) const {
	if (type_) {
		Indentor in;
		Indentor::indent(os) << toString() << "\n";
	}
	return os;
}

// =========================================================================
RepeatBase::~RepeatBase() = default;

void RepeatBase::incr_state_change_no()
{
	state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "RepeatBase::incr_state_change_no()\n";
#endif
}

void  RepeatBase::update_repeat_genvar() const
{
   // **** reset name since generated variables are not persisted
   var_.set_name( name_ );

   // valueAsString() use the last_valid_value() which should always be in range.
   // Note repeat::value() can be on e past the last valid value, at expiration of Repeat loop
   //      However Repeat::last_valid_value() will just return the last valid value.
   var_.set_value( valueAsString() );
}

// =============================================================

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

void RepeatDate::gen_variables(std::vector<Variable>& vec) const
{
   vec.push_back(yyyy_);
   vec.push_back(mm_);
   vec.push_back(dom_);
   vec.push_back(dow_);
   vec.push_back(julian_);
   RepeatBase::gen_variables(vec);
}

const Variable& RepeatDate::find_gen_variable(const std::string& name) const
{
   if (name == name_) return var_;
   if (name == yyyy_.name()) return yyyy_;
   if (name == mm_.name()) return mm_;
   if (name == dom_.name()) return dom_;
   if (name == dow_.name()) return dow_;
   if (name == julian_.name()) return julian_;
   return Variable::EMPTY();
}

void RepeatDate::update_repeat_genvar() const
{
   RepeatBase::update_repeat_genvar();

   yyyy_.set_name( name_ + "_YYYY");
   mm_.set_name( name_ + "_MM");
   dom_.set_name( name_ + "_DD");
   dow_.set_name( name_ + "_DOW");
   julian_.set_name( name_ + "_JULIAN");

   std::string date_as_string = valueAsString();
   boost::gregorian::date the_date(from_undelimited_string(date_as_string));
   if (the_date.is_special()) {
      cout << "RepeatDate::update_repeat_genvar(): error the_date.is_special() " << date_as_string << "\n";
   }

   //int day_of_year  = the_date.day_of_year();
   int day_of_week  = the_date.day_of_week().as_number();
   int day_of_month = the_date.day();
   int month        = the_date.month();
   int year         = the_date.year();

   yyyy_.set_value(boost::lexical_cast<std::string>(year));
   mm_.set_value(boost::lexical_cast<std::string>(month));
   dom_.set_value(boost::lexical_cast<std::string>(day_of_month));
   dow_.set_value(boost::lexical_cast<std::string>(day_of_week));

   long last_value = last_valid_value();
   long julian = Cal::date_to_julian( last_value );
   julian_.set_value(boost::lexical_cast<std::string>(julian));
}

bool RepeatDate::compare(RepeatBase* rb) const
{
	auto* rhs = dynamic_cast<RepeatDate*>(rb);
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
   long julian = Cal::date_to_julian(last_value);
   julian -= val;
   return Cal::julian_to_date(julian);
}

long RepeatDate::last_valid_value_plus(int val) const
{
   long last_value = last_valid_value();
   long julian = Cal::date_to_julian(last_value);
   julian += val;
   return Cal::julian_to_date(julian);
}

void RepeatDate::reset() {
	value_ = start_;
	incr_state_change_no();
}

std::string RepeatDate::toString() const
{
	std::string ret = "repeat date ";  ret += name_; ret += " ";
   ret += boost::lexical_cast<std::string>(start_); ret += " ";
   ret += boost::lexical_cast<std::string>(end_);   ret += " ";
   ret += boost::lexical_cast<std::string>(delta_);

	if (!PrintStyle::defsStyle() && (value_ != start_)) {
	   ret += " # ";
	   ret += boost::lexical_cast<std::string>(value_);
	}
	return ret;
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
   long julian = Cal::date_to_julian(value_);
   julian += delta_;
   value_ = Cal::julian_to_date(julian);

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
   long julian_new_date = Cal::date_to_julian(the_new_date);
   long julian_start = Cal::date_to_julian(start_);
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
RepeatInteger::RepeatInteger() = default;

bool RepeatInteger::compare(RepeatBase* rb) const
{
	auto* rhs = dynamic_cast<RepeatInteger*>(rb);
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
   std::string ret = "repeat integer ";  ret += name_; ret += " ";
   ret += boost::lexical_cast<std::string>(start_);  ret += " ";
   ret += boost::lexical_cast<std::string>(end_);
   if (delta_ != 1) { ret += " ";  ret += boost::lexical_cast<std::string>(delta_); }

   if (!PrintStyle::defsStyle() && (value_ != start_)) {
      ret += " # ";
      ret += boost::lexical_cast<std::string>(value_);
   }
   return ret;
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
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "RepeatInteger::operator==( name_(" << name_ << ") != rhs.name_(" << rhs.name_ << "))" << "\n";
      }
#endif
		return false;
	}
	if (start_ != rhs.start_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "RepeatInteger::operator==( start_(" << start_ << ") != rhs.start_(" << rhs.start_ << "))" << "\n";
      }
#endif
		return false;
	}
	if (end_ != rhs.end_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "RepeatInteger::operator==( end_(" << end_ << ") != rhs.end_(" << rhs.end_ << "))" << "\n";
      }
#endif
		return false;
	}
	if (delta_ != rhs.delta_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "RepeatInteger::operator==( delta_(" << delta_ << ") != rhs.delta_(" << rhs.delta_ << "))" << "\n";
      }
#endif
		return false;
	}
   if (value_ != rhs.value_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "RepeatInteger::operator==( value_(" << value_ << ") != rhs.value_(" << rhs.value_ << "))" << "\n";
      }
#endif
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

//======================================================================================

RepeatEnumerated::RepeatEnumerated( const std::string& variable, const std::vector<std::string>& theEnums)
: RepeatBase(variable), theEnums_(theEnums), currentIndex_(0)
{
   if ( !Str::valid_name( variable ) ) {
      throw std::runtime_error("RepeatEnumerated: Invalid name: " + variable);
   }
   if (theEnums.empty()) {
      throw std::runtime_error("RepeatEnumerated: " + variable + " is empty");
   }
}

int RepeatEnumerated::end() const   {
	if ( theEnums_.empty() ) return 0;
	return static_cast<int>(theEnums_.size()-1);
}

bool RepeatEnumerated::compare(RepeatBase* rb) const
{
	auto* rhs = dynamic_cast<RepeatEnumerated*>(rb);
	if(!rhs) return false;
	return operator==(*rhs);
}

std::string RepeatEnumerated::toString() const
{
   std::string ret = "repeat enumerated ";  ret += name_;
   BOOST_FOREACH(const string& s, theEnums_) { ret += " \""; ret += s; ret += "\""; }
   if (!PrintStyle::defsStyle() && (currentIndex_ != 0)) {
      ret += " # ";
      ret += boost::lexical_cast<std::string>(currentIndex_);
   }
   return ret;
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
 		auto the_new_value = boost::lexical_cast< long >( newValue );
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

//======================================================================================

RepeatString::RepeatString( const std::string& variable, const std::vector<std::string>& theEnums)
: RepeatBase(variable), theStrings_(theEnums), currentIndex_(0)
{
   if ( !Str::valid_name( variable ) ) {
      throw std::runtime_error("RepeatString:: Invalid name: " + variable);
   }
   if (theEnums.empty()) {
      throw std::runtime_error("RepeatString : " + variable + " is empty");
   }
}

int RepeatString::end() const   {
	if ( theStrings_.empty() ) return 0;
	return static_cast<int>(theStrings_.size()-1);
}

bool RepeatString::compare(RepeatBase* rb) const
{
	auto* rhs = dynamic_cast<RepeatString*>(rb);
	if(!rhs) return false;
	return operator==(*rhs);
}

std::string RepeatString::toString() const
{
   std::string ret = "repeat string ";  ret += name_;
   BOOST_FOREACH(const string& s, theStrings_) { ret += " \""; ret += s; ret += "\""; }
   if (!PrintStyle::defsStyle() && (currentIndex_ != 0)) {
      ret += " # ";
      ret += boost::lexical_cast<std::string>(value());
   }
   return ret;
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

//=======================================================================================

bool RepeatDay::compare(RepeatBase* rb) const
{
	auto* rhs = dynamic_cast<RepeatDay*>(rb);
	if(!rhs) return false;
	return operator==(*rhs);
}

std::string RepeatDay::toString() const
{
   std::string ret = "repeat day ";
   ret += boost::lexical_cast<std::string>(step_);
   return ret;
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

CEREAL_REGISTER_TYPE(RepeatDate);
CEREAL_REGISTER_TYPE(RepeatInteger);
CEREAL_REGISTER_TYPE(RepeatEnumerated);
CEREAL_REGISTER_TYPE(RepeatString);
CEREAL_REGISTER_TYPE(RepeatDay);
