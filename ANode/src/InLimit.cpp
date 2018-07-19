//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #64 $
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

#include <boost/lexical_cast.hpp>

#include <assert.h>
#include <sstream>
#include <stdexcept>
#include <iostream>

#include "InLimit.hpp"
#include "Limit.hpp"
#include "Indentor.hpp"
#include "PrintStyle.hpp"
#include "Str.hpp"
#include "Ecf.hpp"

using namespace std;
using namespace ecf;

/////////////////////////////////////////////////////////////////////////////////////////////

InLimit::InLimit(const std::string& name, const std::string& pathToNode, int tokens,bool limit_this_node_only)
: n_(name),path_(pathToNode),tokens_(tokens),limit_this_node_only_(limit_this_node_only),incremented_(false)
{
   if ( !Str::valid_name( name ) ) {
      throw std::runtime_error("InLimit::InLimit: Invalid InLimit name: " + name);
   }
}

bool InLimit::operator==( const InLimit& rhs ) const
{
   if ( path_ != rhs.path_ ) {
#ifdef DEBUG
      if (Ecf::debug_equality())   std::cout << "InLimit::operator==   path_ != rhs.path_\n";
#endif
      return false;
   }
   if ( n_ != rhs.n_ ) {
#ifdef DEBUG
      if (Ecf::debug_equality())  std::cout << "InLimit::operator==     n_ != rhs.n_\n";
#endif
      return false;
   }
   if ( tokens_ != rhs.tokens_ ) {
#ifdef DEBUG
      if (Ecf::debug_equality()) std::cout << "InLimit::operator==    tokens_(" << tokens_  << ") != rhs.tokens_(" << rhs.tokens_ << ")\n";
#endif
      return false;
   }

   if ( limit_this_node_only_ != rhs.limit_this_node_only_ ) {
#ifdef DEBUG
      if (Ecf::debug_equality())std::cout << "InLimit::operator==    limit_this_node_only_(" << limit_this_node_only_  << ") != rhs.limit_this_node_only_(" << rhs.limit_this_node_only_ << ")\n";
#endif
      return false;
   }
   if ( incremented_ != rhs.incremented_ ) {
#ifdef DEBUG
      if (Ecf::debug_equality())std::cout << "InLimit::operator==    incremented_(" << incremented_  << ") != rhs.incremented_(" << rhs.incremented_ << ")\n";
#endif
      return false;
   }

   // Note: comparison does not look at Limit pointers
   return true;
}

std::ostream& InLimit::print( std::ostream& os ) const {
   Indentor in;
   Indentor::indent( os ) << toString();

   if (!PrintStyle::defsStyle()) {

      // write state; See InlimitParser::doParse for read state part
      if (incremented_) {
         os << " # incremented:" << incremented_;
      }

      if ( PrintStyle::getStyle() == PrintStyle::STATE) {
         if ( limit() )
            os << " # referenced limit(value) " << limit()->theLimit() << "(" << limit()->value() << ")";
      }
   }

   os << "\n";
   return os;
}

std::string InLimit::toString() const {
   std::string ret = "inlimit ";
   if (limit_this_node_only_) ret += "-n ";
   if ( path_.empty() )  ret += n_;
   else                      { ret += path_; ret += Str::COLON(); ret += n_; }
   if ( tokens_ != 1 )       { ret += " "; ret += boost::lexical_cast<std::string>(tokens_); }
   return ret;
}

