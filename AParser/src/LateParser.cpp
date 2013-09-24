//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #13 $ 
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

#include "LateParser.hpp"
#include "TimeSeries.hpp"
#include "LateAttr.hpp"
#include "DefsStructureParser.hpp"
#include "Node.hpp"

using namespace ecf;
using namespace std;

bool LateParser::doParse( const std::string& line, std::vector<std::string >& lineTokens )
{
	if ( lineTokens.size() < 3 ) throw std::runtime_error( "LateParser::doParse: Invalid late :" + line );

	// late -s +00:15  -a  20:00  -c +02:00     #The option can be in any order
	//  0   1    2      3   4     5     6        7     8     9  10 11 12  13
 	// late -s +00:15  -c +02:00                # not all options are needed
	//  0    1   2      3   4                   5

	LateAttr lateAttr; // lateAttr.isNull() will return true;
	assert(lateAttr.isNull());

	size_t line_token_size = lineTokens.size();
	for(size_t i = 1; i+1 < line_token_size; i += 2) {
	   if (lineTokens[i][0] == '#') break;

	   if ( lineTokens[i] == "-s") {
	      if ( !lateAttr.submitted().isNULL() ) throw std::runtime_error( "LateParser::doParse:2: Invalid late :" + line );
	      int hour = -1; int min = -1;
	      TimeSeries::getTime(lineTokens[i+1],hour,min);
	      lateAttr.addSubmitted( TimeSlot(hour,min) );
	   }
	   else if ( lineTokens[i] == "-a") {
	      if ( !lateAttr.active().isNULL() ) throw std::runtime_error( "LateParser::doParse:3: Invalid late :" + line );
	      int hour = -1; int min = -1;
	      TimeSeries::getTime(lineTokens[i+1],hour,min);
	      lateAttr.addActive( TimeSlot(hour,min) );
	   }
	   else if ( lineTokens[i] == "-c") {
	      if ( !lateAttr.complete().isNULL() ) throw std::runtime_error( "LateParser::doParse:4: Invalid late :" + line );
	      int hour = -1; int min = -1;
	      bool relative = TimeSeries::getTime(lineTokens[i+1],hour,min);
	      lateAttr.addComplete( TimeSlot(hour,min), relative );
	   }
	   else throw std::runtime_error( "LateParser::doParse:5: Invalid late :" + line );
	}

	if (lateAttr.isNull()) {
	   throw std::runtime_error( "LateParser::doParse:6: Invalid late :" + line );
	}

	// state
	if (rootParser()->get_file_type() != PrintStyle::DEFS && lineTokens[line_token_size-1] == "late") {
	   lateAttr.setLate(true);
	}

	nodeStack_top()->addLate( lateAttr  ) ;
	return true;
}
