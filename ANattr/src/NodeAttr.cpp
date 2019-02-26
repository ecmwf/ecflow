//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #67 $ 
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

#include <cassert>
#include <sstream>
#include <ostream>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "NodeAttr.hpp"
#include "Indentor.hpp"
#include "Calendar.hpp"
#include "PrintStyle.hpp"
#include "Str.hpp"
#include "Ecf.hpp"
#include "Serialization.hpp"

using namespace std;
using namespace ecf;

const std::string& Event::SET() { static const std::string SET = "set"; return SET; }
const std::string& Event::CLEAR() { static const std::string CLEAR = "clear"; return CLEAR; }
const Event& Event::EMPTY() { static const Event EVENT = Event(); return EVENT ; }
const Meter& Meter::EMPTY() { static const Meter METER = Meter(); return METER ; }
const Label& Label::EMPTY() { static const Label LABEL = Label(); return LABEL ; }

////////////////////////////////////////////////////////////////////////////////////////////

Event::Event( int number, const std::string& eventName )
: v_( false ), number_( number ), n_( eventName ), used_( false ), state_change_no_( 0 )
{
   if ( !eventName.empty() ) {
      string msg;
      if ( !Str::valid_name( eventName, msg ) ) {
         throw std::runtime_error( "Event::Event: Invalid event name : " + msg );
      }
   }
}

Event::Event(  const std::string& eventName )
: v_( false ), number_( std::numeric_limits<int>::max() ), n_( eventName ), used_( false ), state_change_no_( 0 )
{
   if ( eventName.empty() ) {
      throw std::runtime_error( "Event::Event: Invalid event name : name must be specified if no number supplied");
   }

   // If the eventName is a integer, then treat it as such, by setting number_ and clearing n_
   // This was added after migration failed, since *python* api allowed:
   //       ta.add_event(1);
   //       ta.add_event("1");
   // and when we called ecflow_client --migrate/--get it generated
   //       event 1
   //       event 1
   // which then did *not* load.
   //
   // Test for numeric, and then casting, is ****faster***** than relying on exception alone
   if ( eventName.find_first_of( Str::NUMERIC() ) != std::string::npos ) {
      try {
         number_ = boost::lexical_cast< int >( eventName );
         n_.clear();
         return;
      }
      catch ( boost::bad_lexical_cast&  ) {
         // cast failed, a real string, carry on
      }
   }

   string msg;
   if ( !Str::valid_name( eventName, msg ) ) {
      throw std::runtime_error( "Event::Event: Invalid event name : " + msg );
   }
}

void Event::set_value( bool b ) {
   v_ = b;
   state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "Event::set_value\n";
#endif
}

std::string Event::name_or_number() const {
   if ( n_.empty() ) {
      std::stringstream ss;
      ss << number_;
      return ss.str();
   }
   return n_;
}

bool Event::operator==( const Event& rhs ) const {
   if ( v_ != rhs.v_ ) {
      return false;
   }
   if ( number_ != rhs.number_ ) {
      return false;
   }
   if ( n_ != rhs.n_ ) {
      return false;
   }
   return true;
}

void Event::print( std::string& os ) const {
   Indentor in;
   Indentor::indent( os ); write(os);
   if ( !PrintStyle::defsStyle() ) {
      if (v_) { os += " # " ; os += Event::SET(); }
   }
   os += "\n";
}

std::string Event::toString() const {
   std::string ret;
   write(ret);
   return ret;
}

void Event::write(std::string& ret) const
{
   ret += "event ";
   if ( number_ == std::numeric_limits< int >::max() )  ret += n_;
   else {
      ret += boost::lexical_cast<std::string>(number_);
      ret += " ";
      ret += n_;
   }
}

std::string Event::dump() const {
   std::stringstream ss;
   ss << toString() << " value(" << v_ << ")  used(" << used_ << ")";
   return ss.str();
}

bool Event::isValidState( const std::string& state ) {
   if ( state == Event::SET() )
      return true;
   if ( state == Event::CLEAR() )
      return true;
   return false;
}

////////////////////////////////////////////////////////////////////////////////////////////

Meter::Meter( const std::string& name, int min, int max, int colorChange, int value ) :
	         min_( min ), max_( max ), v_( value ), cc_( colorChange ),
	         n_( name ), used_( false ), state_change_no_( 0 )
{
   if ( !Str::valid_name( name ) ) {
      throw std::runtime_error("Meter::Meter: Invalid Meter name: " + name);
   }

   if ( min > max )
      throw std::out_of_range( "Meter::Meter: Invalid Meter(name,min,max,color_change) : min must be less than max" );

   if (colorChange == std::numeric_limits<int>::max()) {
      cc_ =  max_;
   }

   if (value == std::numeric_limits<int>::max()) {
      v_ =  min_;
   }

   if ( cc_ < min || cc_ > max ) {
      std::stringstream ss;
      ss << "Meter::Meter: Invalid Meter(name,min,max,color_change) color_change(" << cc_ << ") must be between min(" << min_ << ") and max(" << max_ << ")";
      throw std::out_of_range( ss.str() );
   }
}

void Meter::set_value( int v ) {

   if (!isValidValue( v )) {
      std::stringstream ss;
      ss << "Meter::set_value(int): The meter(" << n_ << ") value must be in the range[" << min() << "->" << max() << "] but found '" << v << "'";
      throw std::runtime_error( ss.str() );
   }

   v_ = v;
   state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "Meter::set_value\n";
#endif
}

bool Meter::operator==( const Meter& rhs ) const {
   if ( v_ != rhs.v_ ) {
      return false;
   }
   if ( min_ != rhs.min_ ) {
      return false;
   }
   if ( max_ != rhs.max_ ) {
      return false;
   }
   if ( cc_ != rhs.cc_ ) {
      return false;
   }
   if ( n_ != rhs.n_ ) {
      return false;
   }
   return true;
}

void Meter::print( std::string& os ) const {
   Indentor in;
   Indentor::indent( os ) ; write(os);
   if ( !PrintStyle::defsStyle() ) {
      if (v_ != min_) { os += " # " ; os += boost::lexical_cast<std::string>(v_); }
   }
   os += "\n";
}

std::string Meter::toString() const {
   std::string ret;
   write(ret);
   return ret;
}

void Meter::write(std::string& ret) const {
   ret += "meter ";
   ret += n_; ret += " ";
   ret += boost::lexical_cast<std::string>(min_); ret += " ";
   ret += boost::lexical_cast<std::string>(max_); ret += " ";
   ret += boost::lexical_cast<std::string>(cc_);
}

std::string Meter::dump() const {
   std::stringstream ss;
   ss << "meter " << n_ << " min(" << min_ << ") max (" << max_
            << ") colorChange(" << cc_ << ") value(" << v_
            << ") used(" << used_ << ")";
   return ss.str();
}

/////////////////////////////////////////////////////////////////////////////////////////////

Label::Label(const std::string& name, const std::string& value, const std::string& new_value)
: n_(name),v_(value),new_v_(new_value),state_change_no_(0)
{
   if ( !Str::valid_name( n_ ) ) {
      throw std::runtime_error("Label::Label: Invalid Label name :" + n_);
   }
}

void Label::print( std::string& os ) const {

   Indentor in;
   Indentor::indent( os ) ; write(os);
   if (!PrintStyle::defsStyle()) {
      if (!new_v_.empty()) {
         if (new_v_.find("\n") == std::string::npos) {
            os += " # \"" ; os += new_v_ ; os += "\"";
         }
         else {
            std::string value = new_v_;
            Str::replaceall(value,"\n","\\n");
            os += " # \"" ; os += value ; os += "\"";
         }
      }
   }
   os += "\n";
}

std::string Label::toString() const {
   // parsing always STRIPS the quotes, hence add them back
   std::string ret; ret.reserve(n_.size() + v_.size() + 10);
   write(ret);
   return ret;
}

void Label::write(std::string& ret) const {
   // parsing always STRIPS the quotes, hence add them back
   ret += "label ";
   ret += n_;
   ret += " \"";
   if (v_.find("\n") == std::string::npos) ret += v_;
   else  {
      // replace \n, otherwise re-parse will fail
      std::string value = v_;
      Str::replaceall(value,"\n","\\n");
      ret += value;
   }
   ret += "\"";
}

std::string Label::dump() const {
   std::stringstream ss;
   ss << toString() << " : \"" << new_v_  << "\"";
   return ss.str();
}

void Label::set_new_value( const std::string& l ) {
   new_v_ = l;
   state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "Label::set_new_value\n";
#endif
}

void Label::reset() {
   new_v_.clear();
   state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "Label::reset()\n";
#endif
}


void Label::parse(const std::string& line, std::vector<std::string >& lineTokens, bool parse_state)
{
   parse(line,lineTokens,parse_state,n_,v_,new_v_);
}

void Label::parse(const std::string& line, std::vector<std::string >& lineTokens, bool parse_state,
                  std::string& the_name,std::string& the_value, std::string& the_new_value)
{
   if ( lineTokens.size() < 3 )
       throw std::runtime_error( "Label::parse: Invalid label :" + line );

    the_name = lineTokens[1];

    // parsing will always STRIP single or double quotes, print will add double quotes
    // label simple_label 'ecgems'
    if ( lineTokens.size() == 3 ) {
       Str::removeQuotes(lineTokens[2]);
       Str::removeSingleQuotes(lineTokens[2]);
       the_value = lineTokens[2];
       if (the_value.find("\\n") != std::string::npos) {
          Str::replaceall(the_value,"\\n","\n");
       }
    }
    else {

       // label complex_label "smsfetch -F %ECF_FILES% -I %ECF_INCLUDE%"  # fred
       // label simple_label "fred" #  "smsfetch -F %ECF_FILES% -I %ECF_INCLUDE%"
       std::string value; value.reserve(line.size());
       size_t line_token_size = lineTokens.size();
       for (size_t i = 2; i < line_token_size; ++i) {
          if ( lineTokens[i].at( 0 ) == '#' ) break;
          if ( i != 2 ) value += " ";
          value += lineTokens[i];
       }

       Str::removeQuotes(value);
       Str::removeSingleQuotes(value);
       the_value = value;
       if (the_value.find("\\n") != std::string::npos) {
          Str::replaceall(the_value,"\\n","\n");
       }


       // state
       if (parse_state) {
          // label name "value" # "new  value"
          bool comment_fnd = false;
          size_t first_quote_after_comment = 0;
          size_t last_quote_after_comment = 0;
          for(size_t i = line.size()-1; i > 0; i--) {
             if (line[i] == '#') { comment_fnd = true; break; }
             if (line[i] == '"') {
                if (last_quote_after_comment == 0) last_quote_after_comment = i;
                first_quote_after_comment = i;
             }
          }
          if (comment_fnd && first_quote_after_comment != last_quote_after_comment) {
             std::string new_value = line.substr(first_quote_after_comment+1,last_quote_after_comment-first_quote_after_comment-1);
             //std::cout << "new label = '" << new_value << "'\n";
             the_new_value = new_value;

             if (the_new_value.find("\\n") != std::string::npos) {
                Str::replaceall(the_new_value,"\\n","\n");
             }
          }
       }
    }
}


template<class Archive>
void Label::serialize(Archive & ar)
{
   ar( CEREAL_NVP(n_));
   CEREAL_OPTIONAL_NVP(ar,v_,     [this](){return !v_.empty();});
   CEREAL_OPTIONAL_NVP(ar,new_v_, [this](){return !new_v_.empty();});  // conditionally save
}

template<class Archive>
void Event::serialize(Archive & ar)
{
   CEREAL_OPTIONAL_NVP(ar,number_,[this](){return number_ != std::numeric_limits<int>::max();});
   CEREAL_OPTIONAL_NVP(ar,n_,     [this](){return !n_.empty();});
   CEREAL_OPTIONAL_NVP(ar,v_,     [this](){return v_;});
}

template<class Archive>
void Meter::serialize(Archive & ar)
{
   ar( CEREAL_NVP(min_),
       CEREAL_NVP(max_),
       CEREAL_NVP(v_),
       CEREAL_NVP(n_),
       CEREAL_NVP(cc_)
   );
}

CEREAL_TEMPLATE_SPECIALIZE(Label);
CEREAL_TEMPLATE_SPECIALIZE(Event);
CEREAL_TEMPLATE_SPECIALIZE(Meter);
