//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #16 $ 
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

#include "LateAttr.hpp"
#include "Indentor.hpp"
#include "Calendar.hpp"
#include "Ecf.hpp"
#include "PrintStyle.hpp"
#include "TimeSeries.hpp"
#include "Str.hpp"
#include "Serialization.hpp"

using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

namespace ecf {

LateAttr::LateAttr() = default;

std::ostream& LateAttr::print(std::ostream& os) const
{
	Indentor in;
	Indentor::indent(os) << toString();
   if (!PrintStyle::defsStyle()) {
      if (isLate_) os << " # late";
   }
   os << "\n";
   return os;
}

std::string LateAttr::toString() const
{
   std::string ret = "late";
   if (!s_.isNULL()) {
      ret += " -s +";
      ret += s_.toString();
   }
   if (!a_.isNULL()) {
      ret += " -a ";
      ret += a_.toString();
   }
   if (!c_.isNULL()) {
      ret += " -c ";
      if (c_is_rel_) ret += "+";
      ret += c_.toString();
   }
   return ret;
}

bool LateAttr::operator==(const LateAttr& rhs) const
{
 	if ( c_is_rel_ != rhs.c_is_rel_) return false;
	if ( s_ != rhs.s_) return false;
	if ( a_ != rhs.a_) return false;
   if ( c_ != rhs.c_) return false;
   if ( isLate_ != rhs.isLate_) return false;
	return true;
}

bool LateAttr::isNull() const
{
	return ( s_.isNULL() && a_.isNULL() && c_.isNULL());
}

void LateAttr::checkForLateness( const std::pair<NState,boost::posix_time::time_duration>&  state,
                                 const ecf::Calendar& calendar )
{
   if (isLate_ || isNull()) {
      return;
   }
   if (check_for_lateness(state,calendar)) {
      setLate(true);
   }
}

bool LateAttr::check_for_lateness( const std::pair<NState,boost::posix_time::time_duration>&  state, const ecf::Calendar& calendar ) const
{
   if (isNull()) return false;

   if (state.first == NState::SUBMITTED || state.first == NState::QUEUED) {

      // Submitted is always relative, ASSUME this means relative to suite start
      if (state.first == NState::SUBMITTED && !s_.isNULL()) {

         // ECFLOW-322
         // The late attr check for being late in submitted state, is always *RELATIVE*
         // Previously we had:
         //
         //       if ( calendar.duration() >= s_.duration()  ) {
         //          setLate(true);
         //          return;
         //       }
         // This is incorrect since calendar.duration() is essentially duration until
         // the last call to Calendar::init() i.e suite duration time. Hence late was
         // set straight away.
         //
         // to check for submitted, we need the duration *after* state went into submitted state
         // state.second is when state went SUBMITTED, relative to suite start
         boost::posix_time::time_duration time_in_submitted_state = calendar.duration() - state.second ;
         if ( time_in_submitted_state >= s_.duration()) {
            return true;
         }
      }

      // In Submitted or queued state, check for active, in REAL time
      if (!a_.isNULL() && calendar.suiteTime().time_of_day() >= a_.duration()) {
         return true;
      }
   }
   else if ( state.first == NState::ACTIVE && !c_.isNULL()) {
      if ( c_is_rel_) {
         // to check for complete, we need the duration when state went into active state
         // state.second is when state went ACTIVE, relative to suite start
         boost::posix_time::time_duration runtime = calendar.duration() - state.second ;
         if ( runtime >= c_.duration()) {
            return true;
         }
      }
      else {
         // Real time
         if (calendar.suiteTime().time_of_day() >= c_.duration()) {
            return true;
         }
      }
   }
   return false;
}

void LateAttr::setLate(bool f)
{
   if (f != isLate_) {
      // minimise changes to state_change_no_
      isLate_ = f;
      state_change_no_ = Ecf::incr_state_change_no();
   }

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "LateAttr::setLate changed=" << (f != isLate_) << "\n";
#endif
}

void LateAttr::override_with(LateAttr* in_late)
{
   if (in_late) {
      if (!in_late->submitted().isNULL()) s_ = in_late->submitted();
      if (!in_late->active().isNULL()) a_ = in_late->active();
      if (!in_late->complete().isNULL()) c_ = in_late->complete();
      c_is_rel_ = in_late->complete_is_relative();

      // DO NOT override isLate_, because if the parent is late, we do not want *ALL* children to be set late
      // isLate_ = in_late->isLate();
   }
}

void LateAttr::parse(LateAttr& lateAttr, const std::string& line, const std::vector<std::string>& lineTokens, size_t index )
{
   // late -s +00:15  -a  20:00  -c +02:00     #The option can be in any order
   //  0   1    2      3   4     5     6        7     8     9  10 11 12  13
   // late -s +00:15  -c +02:00                # not all options are needed
   //  0    1   2      3   4                   5

   assert(lateAttr.isNull());

   size_t line_token_size = lineTokens.size();
   for(size_t i = index; i < line_token_size; i += 1) {
      if (lineTokens[i][0] == '#') break;

      if ( lineTokens[i] == "-s") {
         if ( !lateAttr.submitted().isNULL() ) throw std::runtime_error( "LateParser::doParse: Invalid late, submitted specified twice :" + line );
         if (i + 1 < line_token_size) {
            int hour = -1; int min = -1;
            TimeSeries::getTime(lineTokens[i+1],hour,min);
            lateAttr.addSubmitted( TimeSlot(hour,min) );
            i++;
         }
         else throw std::runtime_error( "LateParser::doParse: Invalid late, submitted time not specified :" + line );
      }
      else if ( lineTokens[i] == "-a") {
         if ( !lateAttr.active().isNULL() ) throw std::runtime_error( "LateParser::doParse: Invalid late, active specified twice :" + line );
         if (i + 1 < line_token_size) {
            int hour = -1; int min = -1;
            TimeSeries::getTime(lineTokens[i+1],hour,min);
            lateAttr.addActive( TimeSlot(hour,min) );
            i++;
         }
         else throw std::runtime_error( "LateParser::doParse: Invalid late, active time not specified :" + line );
      }
      else if ( lineTokens[i] == "-c") {
         if ( !lateAttr.complete().isNULL() ) throw std::runtime_error( "LateParser::doParse: Invalid late, complete specified twice :" + line );
         if (i + 1 < line_token_size) {
            int hour = -1; int min = -1;
            bool relative = TimeSeries::getTime(lineTokens[i+1],hour,min);
            lateAttr.addComplete( TimeSlot(hour,min), relative );
            i++;
         }
         else throw std::runtime_error( "LateParser::doParse: Invalid late, active time not specified :" + line );
      }
      else throw std::runtime_error( "LateParser::doParse:5: Invalid late :" + line );
   }

   if (lateAttr.isNull()) {
      throw std::runtime_error( "LateParser::doParse:6: Invalid late :" + line );
   }
}

LateAttr LateAttr::create(const std::string& lateString)
{
   std::vector<std::string> lineTokens;
   Str::split(lateString,lineTokens);

   if ( lineTokens.empty() ) {
      throw std::runtime_error("LateParser::create: empty string no late specified ?" + lateString );
   }

   // adjust the index
   size_t index = 0;
   if ( lineTokens[0] == "late") {
      index = 1;
   }

   LateAttr lateAttr;
   parse(lateAttr,lateString,lineTokens,index);
   return lateAttr;
}


template<class Archive>
void LateAttr::serialize(Archive & ar)
{
   CEREAL_OPTIONAL_NVP(ar, s_ ,       [this](){return !s_.isNULL();});
   CEREAL_OPTIONAL_NVP(ar, a_,        [this](){return !a_.isNULL();});
   CEREAL_OPTIONAL_NVP(ar, c_,        [this](){return !c_.isNULL();});
   CEREAL_OPTIONAL_NVP(ar, c_is_rel_, [this](){return c_is_rel_;});
   CEREAL_OPTIONAL_NVP(ar, isLate_,   [this](){return isLate_;});
}
CEREAL_TEMPLATE_SPECIALIZE(LateAttr);

}
