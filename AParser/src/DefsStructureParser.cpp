//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #26 $ 
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
#include <sstream>

#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>
#include <boost/token_functions.hpp>

#include "DefsStructureParser.hpp"
#include "DefsParser.hpp"
#include "Defs.hpp"
#include "Version.hpp"
#include "Str.hpp"

//#define DEBUG_PARSER 1

using namespace ecf;
using namespace std;
using namespace boost;

/////////////////////////////////////////////////////////////////////////////////////
DefsStructureParser::DefsStructureParser(Defs* defsfile,const std::string& file_name)
: defsfile_(defsfile),defsParser_(new DefsParser(this)),
  infile_(file_name),
  lineNumber_(0),
  file_type_(PrintStyle::DEFS)
{
}

DefsStructureParser::~DefsStructureParser()
{
#ifdef SHOW_PARSER_STATS
	defsParser_->printStats();
#endif
}

bool DefsStructureParser::doParse(std::string& errorMsg,std::string& warningMsg)
{
   if (!do_parse_only(errorMsg)) {
      return false;
   }

   if (file_type_ == PrintStyle::MIGRATE) {
      return true;
   }

 	// Now parse the trigger/complete expressions and resolve in-limits
	return defsfile_->check(errorMsg,warningMsg);
}

//#define DO_STATS  1
bool DefsStructureParser::do_parse_only(std::string& errorMsg)
{
   if ( !infile_.ok() ) {
      std::stringstream ss;
      ss << "Unable to open file! " << infile_.file_name() << "\n\n";
      ss << Version::description() << "\n";
      errorMsg = ss.str();
      return false;
   }

   std::vector< std::string > lineTokens; lineTokens.reserve(30); // derived from 3199.def & DO_STATS
   string line;                           line.reserve(350);      // derived from 3199.def & DO_STATS
#ifdef DO_STATS
   size_t max_line_size = 0; max_no_of_tokens = 0;
#endif
   while ( infile_.good() ) {

      getNextLine( line ); // will increment lineNumer_
#ifdef DO_STATS
      max_line_size = std::max(max_line_size,line.size());
#endif

      lineTokens.clear();  // This is re-used, hence clear up front
      Str::split(line, lineTokens);
      if (lineTokens.empty()) continue;  // ignore empty lines
#ifdef DO_STATS
      max_no_of_tokens = std::max(max_no_of_tokens,lineTokens.size());
#endif

      // Process each line, according to the parser which is on *top* of the stack
      // If the *top* of the stack is empty use the DefsParser
      Parser* theCurrentParser  = (nodeStack_.empty()) ? defsParser_.get() : const_cast<Parser*>(nodeStack_.top().second) ;
      if ( theCurrentParser == NULL ) {
         std::stringstream ss;
         ss << "No parser found: Could not parse '" << line << "' around line number " << lineNumber_ << "\n";
         ss << Version::description() << "\n\n";
         errorMsg = ss.str();
         return false;
      }

      try {
         // Note: if the chosen parser does not recognise first token then the parent parser has a go at parsing.
         //       If first token begins with '#' it is ignored
         // cout << "DefsStructureParser::currentParser() = " << theCurrentParser->keyword() << "\n";
         theCurrentParser->doParse(line,lineTokens);
      }
      catch ( std::exception& e) {
         std::stringstream ss;
         ss << e.what() << "\n";
         ss << "Could not parse '" << line << "' around line number " << lineNumber_ << "\n";
         ss << Version::description() << "\n\n";
         errorMsg = ss.str();
         return false;
      }
   }
#ifdef DO_STATS
   cout << "max line size = " << max_line_size << "\n";
   cout << "max token size = " << max_no_of_tokens << "\n";
#endif
   return true;
}

void DefsStructureParser::getNextLine(std::string& line)
{
	// *ALL* the handling of multiple statements per line are handled in this function
	// The presence of ';' signals multiple statements per line.
	if (multi_statements_per_line_vec_.empty()) {
		infile_.getline(line);
		lineNumber_++;

		if (!line.empty()) {

			// See if there are multi statements per line, ie task a; task b; task b # comment
			if (line.find(';') != std::string::npos) {

			   // ignore lines which have ';' but start with a comment.  i.e.
			   //     # task a, task b
	         /// calling trim can be very expensive, hence avoid if possible
	         std::string::size_type first_non_space_char_pos = line.find_first_not_of(' ');
	         if (first_non_space_char_pos != std::string::npos && line[first_non_space_char_pos] == '#') {
	            // found leading_comment can ignore this line
	            return;
	         }


	         // Handle multiple statement with # comment at the end
	         //      suite fred; task a; task b; endsuite   # suite fred; task a; task b; endsuite
	         // Remove comment at the end, to avoid adding to list of tokens
	         std::string::size_type commentPos = line.find('#');
	         if ( commentPos !=  std::string::npos) line = line.substr(0,commentPos);

	         char_separator<char> sep(";");
	         typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	         tokenizer tokens(line, sep);
	         std::copy(tokens.begin(), tokens.end(), back_inserter(multi_statements_per_line_vec_));
	         assert( !multi_statements_per_line_vec_.empty());

	         if ( semiColonInEditVariable() ) {
	            // clear multi_statements_per_line_vec_ since we can't cope with ';' in variable value
	            // hence    edit VAR1 'A;B';   edit VAR2 "b;C"
	            // will be treated as one variable and not two
	            // TODO need more sophisticated parsing
	            multi_statements_per_line_vec_.clear();
	         }
	         else {
	            line = *(multi_statements_per_line_vec_.begin());
	            multi_statements_per_line_vec_.erase( multi_statements_per_line_vec_.begin() );
	         }
			}
		}
	}
	else {
		line = *(multi_statements_per_line_vec_.begin());
 		multi_statements_per_line_vec_.erase( multi_statements_per_line_vec_.begin() );
 	}

#ifdef DEBUG_PARSER
	Parser* theParser = currentParser();
	if ( theParser == NULL) {
	   cout << lineNumber_ << ": '" << line
	            << "'              parser( NULL ) node(";
	}
	else {
	   cout << lineNumber_ << ": '" << line
	            << "'              parser(" << theParser->keyword() << ") ";
	   if (theParser->parent()) cout << " parent_parser(" << theParser->parent()->keyword() << ")";
	   cout << " node(";
	}

	if (!nodeStack_.empty())
	   cout << nodeStack_top()->debugType() << " : " << nodeStack_top()->name() << ")\n";
	else
	   cout << "NULL)\n";
#endif
}

bool DefsStructureParser::semiColonInEditVariable()
{
 	if ( multi_statements_per_line_vec_[0].find("edit") != std::string::npos) {
		// all statements must start with a edit, else we have a semi colon inside variable
		//    edit A fred;        edit B bill         # valid
		//    edit A 'fred;bill'; edit B 'bill;bill'  # Can't cope with this, will be ONE variable !!!!
		for(size_t i = 0; i < multi_statements_per_line_vec_.size(); i++) {
			boost::algorithm::trim(multi_statements_per_line_vec_[i]);
			if (multi_statements_per_line_vec_[i].find("edit") != 0) {
				return true;
 			}
		}
	}
 	return false;
}

