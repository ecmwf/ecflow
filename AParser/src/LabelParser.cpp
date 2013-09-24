//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $ 
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

#include "LabelParser.hpp"
#include "Node.hpp"
#include "Str.hpp"
#include "DefsStructureParser.hpp"

using namespace std;
using namespace ecf;

bool LabelParser::doParse( const std::string& line,
                           std::vector<std::string >& lineTokens )
{
	if ( lineTokens.size() < 3 )
		throw std::runtime_error( "LabelParser::doParse: Invalid label :" + line );

	if ( nodeStack().empty() ) {
		throw std::runtime_error("LabelParser::doParse: Could not add label as node stack is empty at line: " + line );
	}

	// parsing will always STRIP single or double quotes, print will add double quotes
	// label simple_label 'ecgems'
	if ( lineTokens.size() == 3 ) {
		Str::removeQuotes(lineTokens[2]);
		Str::removeSingleQuotes(lineTokens[2]);
	 	nodeStack_top()->addLabel( Label( lineTokens[1], lineTokens[2] ) );
 	}
	else {

		// label complex_label "smsfetch -F %ECF_FILES% -I %ECF_INCLUDE%"  # fred
      // label simple_label "fred" #  "smsfetch -F %ECF_FILES% -I %ECF_INCLUDE%"
		std::string value; value.reserve(line.size());
	   size_t line_token_size = lineTokens.size();
		for (size_t i = 2; i < line_token_size; ++i) {
			if ( lineTokens[i].at( 0 ) == '#' ) break;
			if ( i != 2 ) value += " ";
			value += lineTokens[i];
		}

		Str::removeQuotes(value);
		Str::removeSingleQuotes(value);
		Label label( lineTokens[1], value );

		// state
	   if (rootParser()->get_file_type() != PrintStyle::DEFS) {
	      // label name "value" # "new  value"
	      bool comment_fnd = false;
         size_t first_quote_after_comment = 0;
         size_t last_quote_after_comment = 0;
         for(size_t i = line.size()-1; i > 0; i--) {
            if (line[i] == '#') { comment_fnd = true; break; }
            if (line[i] == '"') {
               if (last_quote_after_comment == 0) last_quote_after_comment = i;
               first_quote_after_comment = i;
            }
         }
         if (comment_fnd && first_quote_after_comment != last_quote_after_comment) {
            std::string new_value = line.substr(first_quote_after_comment+1,last_quote_after_comment-first_quote_after_comment-1);
            //std::cout << "new label = '" << new_value << "'\n";
            label.set_new_value(new_value);
         }
	   }

		nodeStack_top()->addLabel( label );
 	}

	return true;
}
