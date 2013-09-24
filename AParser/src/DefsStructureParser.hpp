#ifndef DEFS_STRUCTURE_PARSER_HPP_
#define DEFS_STRUCTURE_PARSER_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #12 $ 
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

#include <string>
#include <stack>
#include <map>
#include <vector>
#include <fstream>

#include "boost/utility.hpp"
#include "boost/scoped_ptr.hpp"

#include "File_r.hpp"
#include "PrintStyle.hpp"

class Defs;
class Node;
class Parser;

// This class is used to parse the DEFS file.
class DefsStructureParser : private boost::noncopyable {
public:

	DefsStructureParser(Defs* defsfile, const std::string& file_name);
	~DefsStructureParser();

	/// Parse the definition file, *AND* check expressions and limits
	/// return true if parse and check are OK, false otherwise
	/// if false is returned, and error message is also returned
	bool doParse(std::string& errorMsg,std::string& warningMsg);

	// The file can be of different styles:
	//    DEFS: This is the structure only (default)
	//    STATE: structure + state
	//    MIGRATE: structure + state (No checking, and no externs )
	void set_file_type(PrintStyle::Type_t t) { file_type_ = t; }
	PrintStyle::Type_t get_file_type() const { return file_type_; }

protected: // allow test code access
   bool do_parse_only(std::string& errorMsg);

private:

 	std::stack< std::pair<Node*,const Parser*> > nodeStack_;  // stack of nodes used in parsing
   std::map<Node*,bool> defStatusMap_;          // check for duplicates

	Defs* defsfile_;
	boost::scoped_ptr<Parser> defsParser_;        // Child parsers will be deleted as well
	friend class Parser;

	ecf::File_r   infile_;
	int           lineNumber_;

	std::vector<std::string> multi_statements_per_line_vec_;
	PrintStyle::Type_t file_type_;

private:
	// read in the next line form the defs file
	void getNextLine(std::string& line);
	bool semiColonInEditVariable();
	friend class TriggerCompleteParser;
};

#endif
