//============================================================================
// Name        :
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
#include <stdexcept>

#include "CalendarParser.hpp"
#include "Str.hpp"
#include "Suite.hpp"

using namespace ecf;
using namespace std;

bool CalendarParser::doParse( const std::string& line, std::vector<std::string >& lineTokens )
{
   if ( lineTokens.size() < 2 ) {
      throw std::runtime_error( "CalendarParser::doParse: Invalid calendar :" + line );
   }
   if ( nodeStack().empty() ) {
      throw std::runtime_error("CalendarParser::doParse: Could not add calendar as node stack is empty at line: " + line );
   }

   Suite* suite =  nodeStack_top()->isSuite();
   if (!suite) throw std::runtime_error("Calendar can only be added to suites and not " + nodeStack_top()->debugType()  );
   suite->set_calendar().read_state(line,lineTokens);

   return true;
}

