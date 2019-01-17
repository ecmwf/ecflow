//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $ 
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

#include <iostream>
#include "DefsStructureParser.hpp"
#include "DateParser.hpp"
#include "Node.hpp"

using namespace ecf;
using namespace std;

bool DateParser::doParse( const std::string& line,
                          std::vector<std::string >& lineTokens )
{
	//  date 15.11.2009 # <value>   // with PersistStyle::STATE & MIGRATE
	//  date 15.*.*
	//  date *.1.*
	if ( lineTokens.size() < 2 ) {
		throw std::runtime_error( "DateParser::doParse: Invalid date :" + line );
	}

	if ( nodeStack().empty() ) {
		throw std::runtime_error("DateParser::doParse: Could not add date as node stack is empty at line: " + line );
	}

	// DateAttr::create can throw for invalid dates
	DateAttr date = DateAttr::create( lineTokens[1]) ;

	// state
	if (lineTokens.size() == 4 && rootParser()->get_file_type() != PrintStyle::DEFS) {
	   if (lineTokens[3] == "free") {
	      date.setFree();
	   }
	}

 	nodeStack_top()->addDate( date );

	return true;
}
