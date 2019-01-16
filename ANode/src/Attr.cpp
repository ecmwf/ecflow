//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #20 $
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

#include <assert.h>
#include "Attr.hpp"

namespace ecf {

const char* Attr::to_string( Attr::Type s ) {
   switch ( s ) {
      case Attr::EVENT:
         return "event";
         break;
      case Attr::METER:
         return "meter";
         break;
      case Attr::LABEL:
         return "label";
         break;
      case Attr::LIMIT:
         return "limit";
         break;
      case Attr::VARIABLE:
         return "variable";
         break;
      case Attr::ALL:
          return "all";
          break;
      case Attr::UNKNOWN:
          return "unknown";
          break;
      default:
         assert(false); break;
   }
   assert(false);
   return NULL;
}

Attr::Type Attr::to_attr( const std::string& str ) {
   if ( str == "event" )    return Attr::EVENT;
   if ( str == "meter" )    return Attr::METER;
   if ( str == "label" )    return Attr::LABEL;
   if ( str == "limit" )    return Attr::LIMIT;
   if ( str == "variable" ) return Attr::VARIABLE;
   if ( str == "all" )      return Attr::ALL;
   return Attr::UNKNOWN;
}

bool Attr::is_valid( const std::string& str ) {
   return (to_attr(str) == Attr::UNKNOWN) ? false : true;
}

std::vector< std::string > Attr::all_attrs() {
   std::vector<std::string> vec;
   vec.reserve( 6 );
   vec.push_back( "event" );
   vec.push_back( "meter" );
   vec.push_back( "label" );
   vec.push_back( "limit" );
   vec.push_back( "variable" );
   vec.push_back( "all" );
   return vec;
}

std::vector<Attr::Type> Attr::attrs()
{
   std::vector<Attr::Type> vec;
   vec.reserve(6);
   vec.push_back( Attr::UNKNOWN    );
   vec.push_back( Attr::EVENT   );
   vec.push_back( Attr::METER     );
   vec.push_back( Attr::LABEL    );
   vec.push_back( Attr::LIMIT  );
   vec.push_back( Attr::VARIABLE     );
   vec.push_back( Attr::ALL     );
   return vec;
}
}
