//============================================================================
// Name        : NodeTree.cpp
// Author      : Avi
// Revision    : $Revision: #21 $ 
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

#include <ostream>
#include <cassert>
#include <sstream>

#include "ClockAttr.hpp"
#include "DateAttr.hpp"
#include "Indentor.hpp"
#include "Calendar.hpp"
#include "Ecf.hpp"
#include "Serialization.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

//==========================================================================================

ClockAttr::ClockAttr(const boost::posix_time::ptime& time, bool hybrid,bool positiveGain)
: hybrid_(hybrid), positiveGain_(positiveGain), startStopWithServer_(false), end_clock_(false),
  gain_(0), day_(0),month_(0),year_(0),
  state_change_no_(Ecf::incr_state_change_no())
{
	boost::gregorian::date theDate = time.date();
	day_ = theDate.day();
	month_ = theDate.month();
	year_ = theDate.year();

	tm t = to_tm(time);
	gain_ = t.tm_hour *3600 + t.tm_min*60 + t.tm_sec;
}

ClockAttr::ClockAttr(int day, int month, int year, bool hybrid )
: hybrid_(hybrid), positiveGain_(false), startStopWithServer_(false), end_clock_(false),
  gain_(0), day_(day),month_(month),year_(year),
  state_change_no_(Ecf::incr_state_change_no())
{
	// Will throw std::out_of_range exception
	DateAttr::checkDate(day,month,year,false /* for calendars we don't allow wild carding */);
}

ClockAttr::ClockAttr(bool hybrid)
: hybrid_(hybrid),
  state_change_no_(Ecf::incr_state_change_no()) {}

std::ostream& ClockAttr::print(std::ostream& os) const
{
	Indentor in;
	Indentor::indent(os) << toString() << "\n";
	return os;
}

std::string ClockAttr::toString() const
{
	std::stringstream ss;
	if (!end_clock_) {
	   ss << "clock ";
	   if (hybrid_) ss << "hybrid ";
	   else         ss << "real ";
	}
	else  {
	   ss << "endclock ";
	}

	if (day_ != 0) ss << day_ << "." << month_ << "." << year_ << " ";

	if (gain_ != 0) {
		if (positiveGain_) ss << "+";
		ss << gain_;
	}

	if ( startStopWithServer_)  ss << " -s";

 	return ss.str();
}

bool ClockAttr::operator==(const ClockAttr& rhs) const
{
	if (hybrid_ != rhs.hybrid_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "hybrid_ != rhs.hybrid_\n";
      }
#endif
	   return false;
	}

	if (startStopWithServer_ != rhs.startStopWithServer_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "startStopWithServer_ (" <<  startStopWithServer_  << ") != rhs.startStopWithServer_ (" << rhs.startStopWithServer_  << ")\n";
      }
#endif
	   return false;
	}

	if (day_ != rhs.day_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "day(" << day_ << ") != rhs.day(" << rhs.day_ << "\n";
      }
#endif
	   return false;
	}

	if (month_ != rhs.month_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "month_(" << month_ << ") != rhs.month_(" << rhs.month_ << "\n";
      }
#endif
      return false;
	}

	if (year_ != rhs.year_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "year_(" << year_ << ") != rhs.year_(" << rhs.year_ << "\n";
      }
#endif
      return false;
	}

	if (gain_ != rhs.gain_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "gain_(" << gain_ << ") != rhs.gain_(" << rhs.gain_ << "\n";
      }
#endif
      return false;
	}

   if (positiveGain_ != rhs.positiveGain_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "positiveGain_(" << positiveGain_ << ") != rhs.positiveGain_(" << rhs.positiveGain_ << ")\n";
      }
#endif
      return false;
   }

	return true;
}

void ClockAttr::date(int day, int month, int year)
{
	// Will throw std::out_of_range exception
	DateAttr::checkDate(day,month,year,false /* for calendars we don't allow wild carding */);
	day_ = day;
	month_ = month;
	year_ = year;
	state_change_no_ =  Ecf::incr_state_change_no();
}

void ClockAttr::set_gain(int hour,int min,bool positiveGain)
{
	positiveGain_ = positiveGain;
	gain_ = (hour * 3600) + min *60;
   state_change_no_ =  Ecf::incr_state_change_no();
}

void ClockAttr::set_gain_in_seconds(long theGain,bool positiveGain)
{
	positiveGain_ = positiveGain;
	gain_ =  theGain;
   state_change_no_ =  Ecf::incr_state_change_no();
}

void ClockAttr::startStopWithServer(bool f) {
   startStopWithServer_ = f;
   state_change_no_ =  Ecf::incr_state_change_no();
}

void ClockAttr::hybrid( bool f ) {
   hybrid_ = f;
   state_change_no_ =  Ecf::incr_state_change_no();
}

void ClockAttr::sync() {
   // When begin_calendar() is called we will sync with computer clock.
   positiveGain_ = false;
   gain_ = 0;
   day_ = 0;
   month_ = 0 ;
   year_ = 0;
   state_change_no_ =  Ecf::incr_state_change_no();
}

void ClockAttr::init_calendar(ecf::Calendar& calendar)
{
	Calendar::Clock_t clockType = (hybrid_) ? Calendar::HYBRID : Calendar::REAL;
   calendar.init(clockType, startStopWithServer_);
}

void ClockAttr::begin_calendar(ecf::Calendar& calendar) const
{
   calendar.begin(ptime());
}

boost::posix_time::ptime ClockAttr::ptime() const
{
   if (day_ != 0) {
      // Use the date given. ie we start from midnight on the given day + gain.
      boost::gregorian::date theDate(year_,month_,day_);
      return {theDate, seconds(gain_)};
   }

   // Get the local time, second level resolution, based on the time zone settings of the computer.
   boost::posix_time::ptime the_time(Calendar::second_clock_time());
   the_time += seconds(gain_);
   return the_time;
}


template<class Archive>
void ClockAttr::serialize(Archive & ar, std::uint32_t const version )
{
   ar( CEREAL_NVP(hybrid_) );
   CEREAL_OPTIONAL_NVP(ar, positiveGain_,        [this](){return  positiveGain_;});
   CEREAL_OPTIONAL_NVP(ar, startStopWithServer_, [this](){return startStopWithServer_ ;}); // ??
   CEREAL_OPTIONAL_NVP(ar, gain_,                [this](){return gain_ != 0;});
   CEREAL_OPTIONAL_NVP(ar, day_,                 [this](){return day_ != 0;});
   CEREAL_OPTIONAL_NVP(ar, month_,               [this](){return month_ != 0;});
   CEREAL_OPTIONAL_NVP(ar, year_,                [this](){return year_ != 0;});
}
CEREAL_TEMPLATE_SPECIALIZE_V(ClockAttr);

