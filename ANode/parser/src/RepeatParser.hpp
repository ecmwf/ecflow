#ifndef REPEATPARSER_HPP_
#define REPEATPARSER_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #8 $ 
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

class RepeatParser : public Parser {
public:
   explicit RepeatParser(DefsStructureParser* p) : Parser(p) {}

	const char* keyword() const override { return "repeat"; }
	bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;

private:
//	void extractDayMonthYear(const std::vector<std::string>& lineTokens,int& x, int& endDate);
	bool get_value(const std::vector< std::string >& lineTokens, int& value) const;
};

#endif
