//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #6 $ 
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
#include <iostream>
#include <sstream>

#include "LogVerification.hpp"
#include "File.hpp"
#include "Str.hpp"
#include "NState.hpp"

using namespace std;

namespace ecf {

bool LogVerification::extractNodePathAndState(	const std::string& logfile,
									std::vector< std::pair< std::string, std::string > >& pathStateVec,
									std::string& errorMsg )
{
	// The log file format we are interested is :
	//       0          1               2                 3
	// LOG:[14:37:00 20.5.2010]    submitted: /test_abort_cmd/family0/abort duration_(00:00:00) initTime_(2010-May-20 14:37:00) suiteTime_(2010-May-20 14:37:00)

	// Open log file, and collate of the node paths and corresponding states
	std::vector<std::string> lines;
	if (!File::splitFileIntoLines(logfile,lines)) {
		errorMsg = "Could not open log file " + logfile + " for test verification";
		return false;
	}

  	int line_number = 0;
	for(std::vector<std::string>::iterator i=lines.begin(); i!=lines.end(); ++i) {

		line_number++;
		if ((*i).find("LOG:") == std::string::npos) {
			continue; // State changes have type Log::LOG
		}

		std::vector< std::string > lineTokens;
	    Str::split(*i, lineTokens);
	    if (lineTokens.size() < 3) {
	    	continue;
	    }

		std::string theState = lineTokens[2];
		theState.erase( theState.size() -1); // remove ':' at the end
 		if (!NState::isValid( theState )) {
			continue;
		}
		// cout << line_number << Str::COLON() << *i << "\n";

 		pathStateVec.push_back( std::make_pair(lineTokens[3], theState ) );
	}
	return true;
}

static bool searchVec(
                      const std::vector< std::pair< std::string, std::string > >& goldenLines,
                      const std::pair< std::string, std::string >& line,
                      size_t start_index)
{
	for(size_t i = start_index; i < goldenLines.size(); i++) {
		if ( goldenLines[i] == line) {
			return true;
		}
	}
	return false;
}

bool LogVerification::compareNodeStates( const std::string& logfile, const std::string& goldenRefLogFile, std::string& errorMsg)
{
	// Open log files, and extract just the state changes
	std::vector< std::pair< std::string, std::string > > lines,goldenLines;
	if (!extractNodePathAndState(logfile,lines,errorMsg)) return false;
	if (!extractNodePathAndState(goldenRefLogFile,goldenLines,errorMsg)) return false;

	if ( lines  != goldenLines) {
		std::stringstream ss;
		ss << "Log file " << logfile << " does not match golden reference file " << goldenRefLogFile << "\n";
		if (lines.size() != goldenLines.size())  ss << "Expected log file size " << goldenLines.size() << " but found " << lines.size() <<"\n";

		bool errorFnd = false;
		for(size_t i=0; i < lines.size() || i < goldenLines.size(); ++i) {

			std::string theLine,theGoldenLine;
			if (i < lines.size() )      theLine       =       lines[i].second + Str::COLON() + lines[i].first;
			if (i < goldenLines.size()) theGoldenLine = goldenLines[i].second + Str::COLON() + goldenLines[i].first;

			if (i < lines.size() && i < goldenLines.size()) {

				if (lines[i] != goldenLines[i]) {
					// Please note that we can't do an exact compare for certain state changes
					// 	 	submitted -->active
					//  	active    -->complete
					// Since this can be OS/scheduler dependent and hence order dependent
					// To compensate look search Golden file
					if (lines[i].second == "submitted" || lines[i].second == "active" || lines[i].second == "complete") {
						// search for line in golden file
						if (searchVec(goldenLines,lines[i],0)) {
							continue;
						}
					}

					ss << "Mismatch at " << i << " log(" << theLine << ")   golden(" << theGoldenLine << ")\n";
					errorFnd = true;
				}
				else {
					ss << "            " << i << " log(" << theLine << ")   golden(" << theGoldenLine << ")\n";
				}
			}
			else {
				errorFnd = true;
				ss << "Mismatch at " << i ;
				if (i < lines.size() ) ss << " log(" << theLine << ")   ";
				else                   ss << " log( ---- )   ";

				if (i < goldenLines.size()) ss << "golden(" << theGoldenLine << ")\n";
				else                        ss << "golden( --- )\n";
			}
		}
		if (errorFnd) errorMsg = ss.str();
	}

	return errorMsg.empty();
}

}
