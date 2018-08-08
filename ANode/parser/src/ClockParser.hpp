#ifndef CLOCKPARSER_HPP_
#define CLOCKPARSER_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #6 $ 
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

#include "Parser.hpp"
class ClockAttr;

class ClockParser : public Parser {
public:
   explicit ClockParser(DefsStructureParser* p) : Parser(p) {}
	const char* keyword() const override { return "clock"; }
	bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
};

class EndClockParser : public Parser {
public:
   explicit EndClockParser(DefsStructureParser* p) : Parser(p) {}
   const char* keyword() const override { return "endclock"; }
   bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
};

#endif
