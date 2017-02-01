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

#include <stdexcept>
#include "DayParser.hpp"
#include "DefsStructureParser.hpp"
#include "Node.hpp"

using namespace std;

bool DayParser::doParse( const std::string& line, std::vector<std::string >& lineTokens )
{
	//  day monday  # free
	//  day tuesday
	if ( lineTokens.size() < 2 ) {
		throw std::runtime_error( "DayParser::doParse: Invalid day :" + line );
	}
	if ( nodeStack().empty() ) {
		throw std::runtime_error("DayParser::doParse: Could not add day as node stack is empty at line: " + line );
	}

	DayAttr day = DayAttr::create( lineTokens[1] );

   // state
   if (lineTokens.size() == 4 && rootParser()->get_file_type() != PrintStyle::DEFS) {
      if (lineTokens[3] == "free") {
         day.setFree();
      }
   }

 	nodeStack_top()->addDay( day );

	return true;
}
