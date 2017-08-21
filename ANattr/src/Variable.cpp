//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #56 $
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

#include <sstream>
#include <stdexcept>

#include "Variable.hpp"
#include "Indentor.hpp"
#include "Str.hpp"

using namespace std;
using namespace ecf;

// init static's
const Variable& Variable::EMPTY() { static const Variable VARIABLE = Variable(); return VARIABLE; }

////////////////////////////////////////////////////////////////////////////////////////////

Variable::Variable(const std::string& name, const std::string& value)
: name_(name), value_(value)
{
   std::string msg;
   if ( !Str::valid_name( name,msg ) ) {
      throw std::runtime_error("Variable::Variable: Invalid Variable name: " + msg);
   }
}

int Variable::value() const
{
   // see if the value is convertible to a integer
   return Str::to_int( value_, 0/* value to return if conversion fails*/);
}

bool Variable::operator==( const Variable& rhs ) const {
   if ( value_ != rhs.value_ )
      return false;
   if ( name_ != rhs.name_ )
      return false;
   return true;
}

std::ostream& Variable::print( std::ostream& os ) const {
   // see notes in VariableParser.h
   //               Hence we do the following:
   //                  a/ On parsing always remove quotes ie single or double
   //                  b/ On serialising always add single quotes
   Indentor in;
   return Indentor::indent( os ) << toString() << "\n";
}

std::ostream& Variable::print_server_variable( std::ostream& os ) const {
   // see notes in VariableParser.h
   //               Hence we do the following:
   //                  a/ On parsing always remove quotes ie single or double
   //                  b/ On serialising always add single quotes
   Indentor in;
   return Indentor::indent( os ) << toString() << " # server\n";
}

std::ostream& Variable::print_generated( std::ostream& os ) const {
   Indentor in;
   return Indentor::indent( os ) << "# " << toString() << "\n";
}

std::string Variable::toString() const
{
   std::string ret; ret.reserve(name_.size() + value_.size() + 8);
   ret += "edit ";
   ret += name_;
   ret += " '";
   if (value_.find("\n") == std::string::npos) ret += value_;
   else {
      // replace \n, otherwise re-parse will fail
      std::string value = value_;
      Str::replaceall(value,"\n","\\n");
      ret += value;
   }
   ret += "'";
   return ret;
}


std::string Variable::dump() const
{
   std::stringstream ss;
   ss << toString() << " value(" << value() << ")";
   return ss.str();
}

