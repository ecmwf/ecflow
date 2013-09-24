#ifndef CLOCKPARSER_HPP_
#define CLOCKPARSER_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #6 $ 
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

#include "Parser.hpp"
class ClockAttr;

class ClockParser : public Parser {
public:
	ClockParser(DefsStructureParser* p) : Parser(p) {}
	virtual const char* keyword() const { return "clock"; }
	virtual bool doParse(const std::string& line, std::vector<std::string>& lineTokens);

private:
	void extractTheGain(const std::string& theGainToken, ClockAttr& clockAttr);
};

#endif
