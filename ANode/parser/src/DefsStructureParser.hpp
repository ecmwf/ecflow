#ifndef DEFS_STRUCTURE_PARSER_HPP_
#define DEFS_STRUCTURE_PARSER_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #12 $ 
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

#include <string>
#include <stack>
#include <map>
#include <vector>
#include <fstream>

#include "File_r.hpp"
#include "PrintStyle.hpp"
#include "DefsParser.hpp"
#include "Node.hpp"

class Defs;
class Node;
class Parser;

// This class is used get a line of defs format from a defs string
class DefsString {
public:
   explicit DefsString(const std::string& defs_as_string);
   bool good() const;
   void getline(std::string& line);
   bool empty() const { return empty_; }
private:
  DefsString(const DefsString&) = delete;
  const DefsString& operator=(const DefsString&) = delete;
private:
   std::vector<std::string> lines_;
   size_t line_pos_{0};
   bool empty_;
};


// This class is used to parse the DEFS file.
// The file can be of different styles:
//    DEFS: This is the structure only (default)
//    STATE: structure + state
//    MIGRATE: structure + state (No checking, and no externs and fault tolerant)
class DefsStructureParser {
private:
  DefsStructureParser(const DefsStructureParser&) = delete;
  const DefsStructureParser& operator=(const DefsStructureParser&) = delete;
public:
   DefsStructureParser(Defs* defsfile, const std::string& file_name);
   DefsStructureParser(Defs* defsfile, const std::string& def_str, bool);
   explicit DefsStructureParser(const std::string& defs_node_string);
   ~DefsStructureParser();

   /// Parse the definition file, *AND* check expressions and limits
   /// return true if parse and check are OK, false otherwise
   /// if false is returned, and error message is also returned
   bool doParse(std::string& errorMsg,std::string& warningMsg);

   /// The string passed to the DefsStructureParser is node string, and always in MIGRATE mode
   /// Only used when constructor is DefsStructureParser(const std::string& defs_node_string);
   node_ptr the_node_ptr() const { return the_node_ptr_;}

   // return the file/string type read in.
   PrintStyle::Type_t get_file_type() const { return file_type_; }

   // warn about tokens not understood.
   std::string& faults() { return faults_;}

protected: // allow test code access
   bool do_parse_file(std::string& errorMsg);
   bool do_parse_string(std::string& errorMsg);

private:
   bool               parsing_node_string_;
   ecf::File_r        infile_;
   Defs*              defsfile_;
   DefsParser         defsParser_;        // Child parsers will be deleted as well
   int                lineNumber_;
   PrintStyle::Type_t file_type_;
   DefsString         defs_as_string_;
   node_ptr           the_node_ptr_;

   std::stack< std::pair<Node*,const Parser*> > nodeStack_;  // stack of nodes used in parsing
   std::vector<std::string> multi_statements_per_line_vec_;
   std::string faults_;       // In MIGRATE mode we ignore unrecognised tokens, store here for later reporting
   std::string error_;
   std::unordered_map<Node*,bool> defStatusMap_;                       // check for duplicates
   friend class Parser;

private:
   // read in the next line form the defs file
   void getNextLine(std::string& line);
   void getNextStringLine(std::string& line);
   bool do_parse_line(const std::string& line,std::vector<std::string>& lineTokens,std::string& errorMsg);
   bool semiColonInEditVariable();

   void set_node_ptr(node_ptr node) { the_node_ptr_ = node;}
   bool parsing_node_string() const { return parsing_node_string_;}

   // store file type read in
   void set_file_type(PrintStyle::Type_t t) { file_type_ = t; }

   friend class FamilyParser; // access set_node_ptr(), parsing_node_string()
   friend class SuiteParser;  // access set_node_ptr(), parsing_node_string()
   friend class TaskParser;   // access set_node_ptr(), parsing_node_string()
   friend class AliasParser;  // access set_node_ptr(), parsing_node_string()
   friend class TriggerCompleteParser;
   friend class DefsStateParser;
};

#endif
