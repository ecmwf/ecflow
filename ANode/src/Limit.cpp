//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #64 $
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

#include "Limit.hpp"
#include "Indentor.hpp"
#include "PrintStyle.hpp"
#include "Str.hpp"
#include "Ecf.hpp"
#include "Suite.hpp"

using namespace std;
using namespace ecf;

/////////////////////////////////////////////////////////////////////////////////////////////

Limit::Limit(const std::string& name,int limit)
: state_change_no_(0),name_(name),theLimit_(limit),value_(0),node_(0)
{
   if ( !Str::valid_name( name ) ) {
      throw std::runtime_error("Limit::Limit: Invalid Limit name: " + name);
   }
}

Limit::Limit(const std::string& name,int limit, int value, const std::set<std::string>& paths)
: state_change_no_(0),name_(name),theLimit_(limit),value_(value),paths_(paths),node_(0)
{
   if ( !Str::valid_name( name ) ) {
      throw std::runtime_error("Limit::Limit: Invalid Limit name: " + name);
   }
}

Limit::Limit(const Limit& rhs)
: state_change_no_(0), name_(rhs.name_),theLimit_(rhs.theLimit_),value_(rhs.value_),paths_(rhs.paths_),node_(0)
{
}

bool Limit::operator==( const Limit& rhs ) const {
   if ( value_ != rhs.value_ ) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Limit::operator==value_(" << value_ << ") != rhs.value_(" << rhs.value_ << ") " << toString() << "   rhs(" << rhs.toString() << ")\n";
      }
#endif
      return false;
   }
   if ( theLimit_ != rhs.theLimit_ ) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Limit::operator==( theLimit_ != rhs.theLimit_) " << toString() << "   rhs(" << rhs.toString() << ")\n";
      }
#endif
      return false;
   }
   if ( name_ != rhs.name_ ) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Limit::operator==( name_ != rhs.name_ ) " << toString() << "   rhs(" << rhs.toString() << ")\n";
      }
#endif
      return false;
   }
   if ( paths_ != rhs.paths_ ) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Limit::operator==( paths_ != rhs.paths_ ) " << toString() << "   rhs(" << rhs.toString() << ")\n";
      }
#endif
      return false;
   }
   return true;
}

std::ostream& Limit::print( std::ostream& os ) const {
   Indentor in;
   Indentor::indent( os ) << toString();
   if (!PrintStyle::defsStyle()) {
      if (value_ != 0) {
         os << " # " << value_;
         for(std::set<std::string>::const_iterator i = paths_.begin(); i != paths_.end(); ++i) {
            os << " " << (*i);
         }
      }
   }
   os << "\n";
   return os;
}

std::string Limit::toString() const {
   std::stringstream ss;
   ss << "limit " << name_ << " " << theLimit_;
   return ss.str();
}

void Limit::decrement( int tokens ,  const std::string& abs_node_path) {

   // cout << "Limit::decrement name = " << name_ << " current value_ = " << value_ << " limit = " <<  theLimit_ << " consume tokens = " << tokens << "\n";
   // Note: we previously had 'if (value_ > 0) {
   //       However if the user had manually changed the value_, then we could be left with paths_,  that would never have been cleared
   if (delete_path(abs_node_path)) {
      // delete_path() will increment state_change_no
      value_ -= tokens;
      if ( value_ < 0 ) {
         value_ = 0;
         paths_.clear();
      }
   }

#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "Limit::decrement\n";
#endif
   // cout << "Limit::decrement name = " << name_ << " current value_ = " << value_ << "\n";
}

void Limit::increment( int tokens , const std::string& abs_node_path) {
   // cout << "Limit::increment name = " << name_ << " current value_ = " << value_ << " limit = " <<  theLimit_ << " consume tokens = " << tokens << "\n";

   // increment should keep increasing limit value, *EVEN* if over the limit. See ECFLOW-324
   // Note: previously we had:
   //     if ( value_ < theLimit_ ) {

   if (paths_.find(abs_node_path) == paths_.end()) {

      paths_.insert( abs_node_path );
      value_ += tokens;
      update_change_no();
   }

#ifdef DEBUG_STATE_CHANGE_NO
      std::cout << "Limit::increment\n";
#endif
   // cout << "Limit::increment name = " << name_ << " current value_ = " << value_ << "\n";
}

void Limit::setValue( int v )
{
   value_ = v;
   update_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "   Limit::setValue() value_ = " << value_ << "\n";
#endif
}

void Limit::setLimit(int v)
{
   theLimit_ = v;
   update_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "   Limit::setLimit() theLimit_ = " << value_ << "\n";
#endif
}

void Limit::set_paths(const std::set<std::string>& paths)
{
   paths_ = paths;
   update_change_no();
}

void Limit::set_state(int limit, int value, const std::set<std::string>& paths)
{
   value_ = value;
   theLimit_ = limit;
   paths_ = paths;
   update_change_no();
}

bool Limit::delete_path( const std::string& abs_node_path)
{
   std::set<std::string>::iterator i = paths_.find(abs_node_path);
   if (i != paths_.end()) {
      paths_.erase(i);
      update_change_no();
      return true;
   }

#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "Limit::delete_path() \n";
#endif
   return false;
}

void Limit::reset() {
   paths_.clear();
   setValue(0); // will increment state_change_no_
}

void Limit::update_change_no()
{
   state_change_no_ = Ecf::incr_state_change_no();
   if (node_) {
      Suite* suite = node_->suite();
      if (suite) suite->set_state_change_no(state_change_no_);
   }
}
