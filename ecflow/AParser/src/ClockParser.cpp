//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #22 $ 
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
#include <stdexcept>

#include "ClockParser.hpp"
#include "DateParser.hpp"
#include "TimeSeries.hpp"
#include "Extract.hpp"
#include "Str.hpp"
#include "Suite.hpp"

using namespace ecf;
using namespace std;

bool ClockParser::doParse( const std::string& line,
                           std::vector<std::string >& lineTokens )
{
	// clock hybrid   # a comment
	// clock hybrid
	// clock real                  #a comment
	// clock real 300
	// clock real +01:00
	// clock real 20.1.2007
	// clock real 20.1.2007 +01:00

	// For Testing we allow calendar increment to be change on a per suite basis
	// -c is optional
	// clock real 20.1.2007 +01:00
	// clock hybrid

	// Allow clock to be stopped/started when the server is stopped/started
	// and hence always honour time dependencies. See Calendar.h for more details
	// This is optional.( Need to advertise this functionality)
	// clock real 20.1.2007 +01:00 -s
	// clock hybrid -s
	if ( lineTokens.size() < 2 ) {
		throw std::runtime_error( "ClockParser::doParse: Invalid clock :" + line );
	}
	if ( nodeStack().empty() ) {
		throw std::runtime_error("ClockParser::doParse: Could not add clock as node stack is empty at line: " + line );
	}

	bool hybrid = true ;
	if ( lineTokens[1] == "real" )         hybrid = false ;
	else if ( lineTokens[1] == "hybrid" )  hybrid = true;
	else throw std::runtime_error( "Invalid clock :" + line );


	ClockAttr clockAttr(hybrid);

	if ( lineTokens.size() >= 3 && lineTokens[2][0] != '#' ) {
		// if third token is not a comment the time must be of the form
		// clock real 300
		// clock real +01:00
		// clock real 20.1.2007
		// clock real 20.1.2007

		if ( lineTokens[2].find(".") != std::string::npos ) {
			// clock real 20.1.2007

			// If 0 returned then day,month,year is of form *, and not valid
			int day,month,year;
			DateAttr::getDate(lineTokens[2],day,month,year);

			// This will throw std::aout_of_range for an invalid clock date. Note no wild carding allowed.
			clockAttr.date(day,month,year); // this will check the date

		   if ( lineTokens.size() >= 4 && lineTokens[3][0] != '#' ) {
		      // clock real 20.1.2007 +01:00
		      // clock real 20.1.2007 +300
		      // clock real 20.1.2007 350
		      extractTheGain(lineTokens[3], clockAttr);
		   }
		}
		else {
			// clock real 300
			// clock real +01:00
			extractTheGain(lineTokens[2], clockAttr);
		}
	}

	// Look for the optional -s  , i.e start/stop calendar when the server starts/stops
	// 	clock real 20.1.2007 +01:00  -s
	// 	clock hybrid -s
	size_t line_token_size = lineTokens.size();
 	for(size_t i = 2; i < line_token_size; i++ ) {
 	   if (lineTokens[i] == "-s") {
 			clockAttr.startStopWithServer(true);
			break;
		}
		// Reached the comment, no more valid tokens possible
		if (lineTokens[i][0] == '#') {
			// handles comments of the form:
			//    # comment
			//    #comment
			// since we check the first character
			break;
		}
	}

 	Suite* suite =  nodeStack_top()->isSuite();
 	if (!suite) throw std::runtime_error("Clock can only be added to suites and not " + nodeStack_top()->debugType()  );

 	suite->addClock(clockAttr);

	return true;
}

void ClockParser::extractTheGain(const std::string& theGainToken, ClockAttr& clockAttr)
{
 	if ( theGainToken.find(Str::COLON()) != std::string::npos ) {
		// clock real +01:00
		// clock real  01:36
 		int hour,min ;
		bool positiveGain = TimeSeries::getTime(theGainToken,hour,min);
		clockAttr.set_gain(hour,min,positiveGain);
		return;
	}

	long theGain = 0;
	std::string theGainStr = theGainToken;
	bool positiveGain = false;
	if ( theGainStr[0] == '+') {
		positiveGain = true;
		theGainStr.erase(theGainStr.begin());
	}
	theGain =  Extract::theInt(theGainStr,"Invalid clock gain:" + theGainToken);
	clockAttr.set_gain_in_seconds(theGain,positiveGain);
}
