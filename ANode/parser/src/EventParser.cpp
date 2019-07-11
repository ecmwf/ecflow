//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #17 $ 
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
#include <boost/lexical_cast.hpp>
#include "EventParser.hpp"
#include "DefsStructureParser.hpp"
#include "Str.hpp"
#include "Node.hpp"

using namespace std;
using namespace ecf;

bool EventParser::doParse( const std::string& line,
                           std::vector<std::string >& lineTokens )
{
	if ( lineTokens.size() < 2 ) throw std::runtime_error( "EventParser::doParse: Invalid event : " + line );

   // Events added to suite/family can not be signaled by child command
   // However alter/force should allow events to set/cleared on Family/suite
	if ( nodeStack().empty() ) {
		throw std::runtime_error("EventParser::doParse: Could not add event as node stack is empty at line: " + line );
	}

	// ===============================================================
	// Don't use -1, to represent that no number was specified, as on
	// AIX portable binary archive can't cope with this
	// use std::numeric_limits<int>::max()
	// THIS HAS TO BE THE SAME AS THE Event() constructor
	// ================================================================
	string name;
	bool initial_value = false;
	int number = std::numeric_limits<int>::max();

	// Test for numeric, and then casting, is ****faster***** than relying on exception alone
	if ( lineTokens[1].find_first_of( Str::NUMERIC() ) == 0 ) {
	   //   event 0
	   //   event 1 eventName
	   //   event 2 eventNamea
		try {
			number = boost::lexical_cast< int >( lineTokens[1] );
			if ( lineTokens.size() >= 3 && lineTokens[2][0] != '#' ) {
				name = lineTokens[2];
			   // event 1 eventName set
	         if (lineTokens.size() >=4 && lineTokens[3] == "set") initial_value = true;
			}
			else {
			   // event 0 set
	         if (lineTokens.size() >=3 && lineTokens[2] == "set") initial_value = true;
			}
		}
		catch ( boost::bad_lexical_cast&  ) {
			name = lineTokens[1];
	      if (lineTokens.size() >=3 && lineTokens[2] == "set") initial_value = true;
 		}
	}
	else {
      // event name
      // event name set
		name = lineTokens[1];
		if (lineTokens.size() >=3 && lineTokens[2] == "set") initial_value = true;
 	}

	// structure
   bool check = (rootParser()->get_file_type() != PrintStyle::NET);
   Event event(number,name,initial_value,check);

   // state
   if (rootParser()->get_file_type() != PrintStyle::DEFS) {
      if (lineTokens[lineTokens.size()-1] == Event::SET()) {
         event.set_value(true);
      }
      else if (lineTokens[lineTokens.size()-1] == Event::CLEAR()) {
         event.set_value(false);
      }
   }

   nodeStack_top()->addEvent( event, check ) ;

	return true;
}
