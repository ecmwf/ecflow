//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include "InlimitParser.hpp"
#include "Extract.hpp"
#include "Node.hpp"

using namespace std;

bool InlimitParser::doParse(
                             const std::string& line,
                             std::vector<std::string >& lineTokens )
{
	if ( lineTokens.size() < 2 )
		throw std::runtime_error( "InlimitParser::doParse: Invalid inlimit :" + line );

	string path;
	string limitName;
	if ( !Extract::pathAndName( lineTokens[1], path, limitName ) ) {
		throw std::runtime_error( "InlimitParser::doParse: Invalid inlimit : " + line );
	}

	//extract priority, if third token is not a comment it must be a priority
	int tokens = Extract::optionalInt( lineTokens, 2, 1, "Invalid in limit : " + line );

	if ( !nodeStack().empty() ) {
		Node* node = nodeStack_top();

		//     		cerr << "limitName=" << limitName << " path=" << path << "\n";
		node->addInLimit( InLimit( limitName, path, tokens ) ) ;
	}
	return true;
}
