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

#include "LimitParser.hpp"
#include "Extract.hpp"
#include "Node.hpp"
#include "Limit.hpp"
#include "DefsStructureParser.hpp"

using namespace std;

bool LimitParser::doParse(
                           const std::string& line,
                           std::vector<std::string >& lineTokens )
{
   // limit name the_limit # value path1 path2
	if ( lineTokens.size() < 3 )
		throw std::runtime_error( "LimitParser::doParse: Invalid limit " + line );

   if ( nodeStack().empty() )
      throw std::runtime_error("LimitParser::doParse: Could not add limit as node stack is empty at line: " + line );

   int limitValue = Extract::theInt( lineTokens[2], "LimitParser::doParse: Invalid limit value: " + line );

   Node* node = nodeStack_top();

   if (rootParser()->get_file_type() != PrintStyle::DEFS) {
      // state
      int value = 0;
      std::set<std::string> paths;
      bool comment_fnd  = false;
      bool value_processed = false;
      for(size_t i = 3; i < lineTokens.size(); i++) {
         if (comment_fnd) {
            if (!value_processed) {
               value = Extract::theInt(lineTokens[i],"LimitParser::doParse: Could not extract limit value: " + lineTokens[i]);
               value_processed = true;
            }
            else {
               paths.insert(lineTokens[i]);
            }
         }
         if (lineTokens[i] == "#") comment_fnd = true;
      }

      node->addLimit( Limit( lineTokens[1], limitValue, value, paths ) ) ;
   }
   else {
      // structure
      node->addLimit( Limit( lineTokens[1], limitValue ) ) ;
   }
	return true;
}
