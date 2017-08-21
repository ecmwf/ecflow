#ifndef DEFS_STRUCTURE_PARSER_HPP_
#define DEFS_STRUCTURE_PARSER_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #12 $ 
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

#include <string>
#include <stack>
#include <map>
#include <vector>
#include <fstream>

#include "File_r.hpp"
#include "PrintStyle.hpp"
#include "DefsParser.hpp"

class Defs;
class Node;
class Parser;

// This class is used get a line of defs format from a defs string
class DefsString : private boost::noncopyable {
public:
   DefsString(const std::string& defs_as_string);
   bool good() const;
   void getline(std::string& line);
   bool empty() const { return empty_; }
private:
   bool empty_;
   size_t index_;
   std::vector<std::string> lines_;
};


// This class is used to parse the DEFS file.
// The file can be of different styles:
//    DEFS: This is the structure only (default)
//    STATE: structure + state
//    MIGRATE: structure + state (No checking, and no externs and fault tolerant)
class DefsStructureParser : private boost::noncopyable {
public:
   DefsStructureParser(Defs* defsfile, const std::string& file_name);
   DefsStructureParser(Defs* defsfile, const std::string& str, bool);
   ~DefsStructureParser();

   /// Parse the definition file, *AND* check expressions and limits
   /// return true if parse and check are OK, false otherwise
   /// if false is returned, and error message is also returned
   bool doParse(std::string& errorMsg,std::string& warningMsg);

   // store file type read in
   void set_file_type(PrintStyle::Type_t t) { file_type_ = t; }
   PrintStyle::Type_t get_file_type() const { return file_type_; }

   std::string& faults() { return faults_;}

protected: // allow test code access
   bool do_parse_file(std::string& errorMsg);
   bool do_parse_string(std::string& errorMsg);

private:
   ecf::File_r        infile_;
   Defs*              defsfile_;
   DefsParser         defsParser_;        // Child parsers will be deleted as well
   int                lineNumber_;
   PrintStyle::Type_t file_type_;
   DefsString         defs_as_string_;

   std::stack< std::pair<Node*,const Parser*> > nodeStack_;  // stack of nodes used in parsing
   std::vector<std::string> multi_statements_per_line_vec_;
   std::string faults_;       // In MIGRATE mode we ignore unrecognised tokens, store here for later reporting
   std::string error_;
   std::map<Node*,bool> defStatusMap_;                       // check for duplicates
   friend class Parser;

private:
   // read in the next line form the defs file
   void getNextLine(std::string& line);
   void getNextStringLine(std::string& line);
   bool do_parse_line(const std::string& line,std::vector<std::string>& lineTokens,std::string& errorMsg);
   bool semiColonInEditVariable();
   friend class TriggerCompleteParser;
};

#endif
