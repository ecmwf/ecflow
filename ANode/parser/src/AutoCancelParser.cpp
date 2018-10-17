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

#include "AutoCancelParser.hpp"
#include "AutoCancelAttr.hpp"
#include "TimeSeries.hpp"
#include "Extract.hpp"
#include "Node.hpp"

using namespace ecf;
using namespace std;

bool AutoCancelParser::doParse( const std::string& line, std::vector<std::string >& lineTokens )
{
	// autocancel +01:00    # cancel one hour after complete
	// autocancel 01:00     # cancel at 1 am in morning after complete
	// autocancel 10        # cancel 10 days after complete
	// autocancel 0         # cancel immediately after complete

	if ( lineTokens.size() < 2 )  throw std::runtime_error( "AutoCancelParser::doParse: Invalid autocancel :" + line );
	if ( nodeStack().empty() ) throw std::runtime_error("AutoCancelParser::doParse: Could not add autocancel as node stack is empty at line: " + line );

 	if (lineTokens[1].find_first_of(':') == string::npos) {
		// Must be of the form:
		// autocancel 10        # cancel 10 days after complete
		// autocancel 0         # cancel immediately after complete
		int days = Extract::theInt(lineTokens[1],"invalid autocancel " + line) ;

		nodeStack_top()->addAutoCancel( AutoCancelAttr( days ) ) ;
  	}
	else {
		// Must be of the form:
		// autocancel +01:00    # cancel one hour after complete
		// autocancel 01:00     # cancel at 1 am in morning after complete
 		int hour = 0;
		int min = 0;
		bool relative = TimeSeries::getTime(lineTokens[1],hour,min);

		nodeStack_top()->addAutoCancel( AutoCancelAttr( TimeSlot( hour, min), relative  ) ) ;
 	}
	return true;
}


