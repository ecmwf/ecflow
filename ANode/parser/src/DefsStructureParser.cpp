//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #26 $ 
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
#include <sstream>

#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>
#include <boost/token_functions.hpp>

#include "DefsStructureParser.hpp"
#include "Defs.hpp"
#include "Version.hpp"
#include "Str.hpp"

//#define DEBUG_PARSER 1

using namespace ecf;
using namespace std;
using namespace boost;

/////////////////////////////////////////////////////////////////////////////////////
DefsStructureParser::DefsStructureParser(Defs* defsfile,const std::string& file_name)
: parsing_node_string_(false),
  infile_(file_name),
  defsfile_(defsfile),
  defsParser_(this),
  lineNumber_(0),
  file_type_(PrintStyle::DEFS),
  defs_as_string_(Str::EMPTY())
{
   if ( !infile_.ok() ) {
      std::stringstream ss;
      ss << "DefsStructureParser::DefsStructureParser: Unable to open file! " << infile_.file_name() << "\n\n";
      ss << Version::description() << "\n";
      error_ = ss.str();
   }
}

DefsStructureParser::DefsStructureParser(Defs* defsfile, const std::string& str, bool)
: parsing_node_string_(false),
  infile_(""),
  defsfile_(defsfile),
  defsParser_(this),
  lineNumber_(0),
  file_type_(PrintStyle::DEFS),
  defs_as_string_(str)
{
   if ( defs_as_string_.empty() ) {
      std::stringstream ss;
      ss << "DefsStructureParser::DefsStructureParser :  Unable to parse empty string\n\n";
      ss << Version::description() << "\n";
      error_ = ss.str();
   }
}

DefsStructureParser::DefsStructureParser(const std::string& defs_node_string)
: parsing_node_string_(true),
  infile_(""),
  defsfile_(NULL),
  defsParser_(this,true/* only parse nodes */),
  lineNumber_(0),
  file_type_(PrintStyle::MIGRATE),
  defs_as_string_(defs_node_string )
{
   if ( defs_as_string_.empty() ) {
      std::stringstream ss;
      ss << "DefsStructureParser::DefsStructureParser :  Unable to parse empty string\n\n";
      ss << Version::description() << "\n";
      error_ = ss.str();
   }
}

DefsStructureParser::~DefsStructureParser()
{
#ifdef SHOW_PARSER_STATS
	defsParser_.printStats();
#endif
}

bool DefsStructureParser::doParse(std::string& errorMsg,std::string& warningMsg)
{
   if (!error_.empty()) {
      errorMsg = error_;
      return false;
   }

   if (defs_as_string_.empty()) {
      if (!do_parse_file(errorMsg)) {
         return false;
      }
   }
   else {
      if (!do_parse_string( errorMsg)) {
         return false;
      }
   }

   if (file_type_ == PrintStyle::MIGRATE || parsing_node_string_) {
      warningMsg += faults_;
      return true;
   }

   // Note:: if parsing_node_string_ == false, then defsfile_ == NULL
 	// Now parse the trigger/complete expressions and resolve in-limits
	return defsfile_->check(errorMsg,warningMsg);
}

bool DefsStructureParser::do_parse_file(std::string& errorMsg)
{
   std::vector< std::string > lineTokens; lineTokens.reserve(30); // derived from 3199.def
   string line;                           line.reserve(350);      // derived from 3199.def
   while ( infile_.good() ) {
      getNextLine( line ); // will increment lineNumer_
      if (!do_parse_line(line,lineTokens, errorMsg)) {
         return false;
      }
   }
   return true;
}

bool DefsStructureParser::do_parse_string(std::string& errorMsg)
{
   std::vector< std::string > lineTokens; lineTokens.reserve(30); // derived from 3199.def
   string line;                           line.reserve(350);      // derived from 3199.def
   while ( defs_as_string_.good() ) {
      getNextLine( line ); // will increment lineNumer_
      if (!do_parse_line(line,lineTokens, errorMsg)) {
         the_node_ptr_ = node_ptr();
         return false;
      }
   }
   return true;
}

bool DefsStructureParser::do_parse_line(const std::string& line,std::vector<std::string>& lineTokens,std::string& errorMsg)
{
   lineTokens.clear();                   // This is re-used, hence clear up front
   Str::split(line, lineTokens);
   if (lineTokens.empty()) return true;  // ignore empty lines

   // Process each line, according to the parser which is on *top* of the stack
   // If the *top* of the stack is empty use the DefsParser
   Parser* theCurrentParser  = (nodeStack_.empty()) ? &defsParser_ : const_cast<Parser*>(nodeStack_.top().second) ;
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
   return true;
}

void DefsStructureParser::getNextLine(std::string& line)
{
	// *ALL* the handling of multiple statements per line are handled in this function
	// The presence of ';' signals multiple statements per line.
	if (multi_statements_per_line_vec_.empty()) {
	   if (defs_as_string_.empty()) infile_.getline(line);
	   else                         defs_as_string_.getline(line);
		lineNumber_++;
	   if (file_type_ == PrintStyle::MIGRATE) {
	      return; // ignore multiline for migrate, *BECAUSE* *history* for group command uses ';'
	   }

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
		for(auto & i : multi_statements_per_line_vec_) {
			boost::algorithm::trim(i);
			if (i.find("edit") != 0) {
				return true;
 			}
		}
	}
 	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////
DefsString::DefsString(const std::string& defs_as_string): empty_(defs_as_string.empty()), index_(0)
{
   if (!empty_) Str::split(defs_as_string,lines_,"\n");
}
bool DefsString::good() const { return index_ < lines_.size();}
void DefsString::getline(std::string& line)
{
   assert(good());
   line = lines_[index_];
   index_++;
}
