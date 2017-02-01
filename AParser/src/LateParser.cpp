//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #13 $ 
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

#include "LateParser.hpp"
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
	size_t start_index = 1;
	LateAttr::parse(lateAttr,line,lineTokens,start_index);

	// state
	if (rootParser()->get_file_type() != PrintStyle::DEFS && lineTokens[lineTokens.size()-1] == "late") {
	   lateAttr.setLate(true);
	}

	nodeStack_top()->addLate( lateAttr  ) ;
	return true;
}
