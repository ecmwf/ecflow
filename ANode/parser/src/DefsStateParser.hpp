#ifndef DEFS_STATE_PARSER_HPP_
#define DEFS_STATE_PARSER_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $
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

#include "Parser.hpp"

class DefsStateParser : public Parser {
public:
   explicit DefsStateParser(DefsStructureParser* p) : Parser(p){}
   virtual bool doParse(const std::string& line,std::vector<std::string>& lineTokens);
   virtual const char* keyword() const { return "defs_state"; }
};

class HistoryParser : public Parser {
public:
   explicit HistoryParser(DefsStructureParser* p) : Parser(p){}
   virtual bool doParse(const std::string& line,std::vector<std::string>& lineTokens);
   virtual const char* keyword() const { return "history"; }
};

#endif
