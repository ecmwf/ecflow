//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #11 $
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

#include "../../ANode/parser/src/AutoArchiveParser.hpp"

#include "AutoArchiveAttr.hpp"

#include "TimeSeries.hpp"
#include "Extract.hpp"
#include "Node.hpp"

using namespace ecf;
using namespace std;

bool AutoArchiveParser::doParse( const std::string& line, std::vector<std::string >& lineTokens )
{
   // autoarchive +01:00    # archive one hour after complete
   // autoarchive 01:00     # archive at 1 am in morning after complete
   // autoarchive 10        # archive 10 days after complete
   // autoarchive 0         # archive immediately after complete

   if ( lineTokens.size() < 2 )  throw std::runtime_error( "AutoArchiveParser::doParse: Invalid autoarchive :" + line );
   if ( nodeStack().empty() ) throw std::runtime_error("AutoArchiveParser::doParse: Could not add autoarchive as node stack is empty at line: " + line );

   if (lineTokens[1].find_first_of(':') == string::npos) {
      // Must be of the form:
      // autoarchive 10        # archive 10 days after complete
      // autoarchive 0         # archive immediately after complete
      int days = Extract::theInt(lineTokens[1],"invalid autoarchive " + line) ;

      nodeStack_top()->add_autoarchive( AutoArchiveAttr( days ) ) ;
   }
   else {
      // Must be of the form:
      // autoarchive +01:00    # archive one hour after complete
      // autoarchive 01:00     # archive at 1 am in morning after complete
      int hour = 0;
      int min = 0;
      bool relative = TimeSeries::getTime(lineTokens[1],hour,min);

      nodeStack_top()->add_autoarchive( AutoArchiveAttr( TimeSlot( hour, min), relative  ) ) ;
   }
   return true;
}
