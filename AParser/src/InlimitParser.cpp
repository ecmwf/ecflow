//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $ 
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

#include "InlimitParser.hpp"
#include "Extract.hpp"
#include "Node.hpp"
#include "DefsStructureParser.hpp"

using namespace std;

bool InlimitParser::doParse( const std::string& line, std::vector<std::string >& lineTokens )
{
   // inlimit /suite:queue1
   // inlimit disk 50
   // inlimit -n /suite:queue1 2
   // inlimit -n fam
   size_t lineTokens_size = lineTokens.size();
	if ( lineTokens_size < 2 )
		throw std::runtime_error( "InlimitParser::doParse: Invalid inlimit :" + line );

   if ( nodeStack().empty() )
      throw std::runtime_error("InlimitParser::doParse: Could not add inlimit as node stack is empty at line: " + line );

	bool limit_this_node_only = false;
	int token_pos = 1;
	if (lineTokens[token_pos] == "-n") {
	   limit_this_node_only = true;
	   token_pos++;
	}

	string path_to_node_holding_the_limit;
	string limitName;
	if ( !Extract::pathAndName( lineTokens[token_pos], path_to_node_holding_the_limit, limitName ) ) {
		throw std::runtime_error( "InlimitParser::doParse: Invalid inlimit : " + line );
	}

	token_pos++;
	int tokens = Extract::optionalInt( lineTokens, token_pos, 1, "Invalid in limit : " + line );

	InLimit inlimit(limitName,path_to_node_holding_the_limit,tokens,limit_this_node_only);
	if (rootParser()->get_file_type() != PrintStyle::DEFS) {
	   token_pos++;
	   bool incremented = false;
	   for(size_t i = token_pos; i < lineTokens_size; i++) {
	      // see InLimit::print(..) is to why "incremented:1"
	      if (lineTokens[i].find("incremented:1") != std::string::npos ) {
	         incremented = true;
	         break;
	      }
	   }
	   inlimit.set_incremented(incremented);
	}

	//  cout << inlimit.toString() << "\n";
	Node* node = nodeStack_top();
	node->addInLimit(inlimit);

	return true;
}
