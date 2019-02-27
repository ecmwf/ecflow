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
#include "MeterParser.hpp"
#include "Extract.hpp"
#include "Node.hpp"
#include "DefsStructureParser.hpp"

using namespace ecf;
using namespace std;

bool MeterParser::doParse( const std::string& line, std::vector<std::string >& lineTokens )
{
   // meter 0 100 100 # value
	if ( lineTokens.size() < 4 )
		throw std::runtime_error("MeterParser::doParse: Invalid meter :" + line );

	if ( nodeStack().empty() ) {
		throw std::runtime_error("MeterParser::doParse: Could not add meter as node stack is empty at line: " + line );
	}

	int min = Extract::theInt( lineTokens[2], "Invalid meter : " + line );
	int max = Extract::theInt( lineTokens[3], "Invalid meter : " + line );
	int colorChange = Extract::optionalInt( lineTokens, 4, std::numeric_limits<int>::max(), "Invalid meter : " + line );

   // state
	int value = std::numeric_limits<int>::max();
   if (rootParser()->get_file_type() != PrintStyle::DEFS) {
      bool comment_fnd =  false;
      for(size_t i = 3; i < lineTokens.size(); i++) {
         if (comment_fnd) {
            // token after comment is the value
            value = Extract::theInt(lineTokens[i],"MeterParser::doParse, could not extract meter value");
            break;
         }
         if (lineTokens[i] == "#") comment_fnd = true;
      }
   }

   bool check = (rootParser()->get_file_type() != PrintStyle::NET);

	nodeStack_top()->add_meter( lineTokens[1], min, max, colorChange, value, check  ) ;

	return true;
}
