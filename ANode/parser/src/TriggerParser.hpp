#ifndef TRIGGERPARSER_HPP_
#define TRIGGERPARSER_HPP_

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

class TriggerCompleteParser : public Parser {
protected:
   explicit TriggerCompleteParser(DefsStructureParser* p) : Parser(p) {}
	void getExpression(const std::string& line,
	                   std::vector<std::string>& lineTokens,
	                   std::string& expression,
	                   bool& andExr,
	                   bool& orExpr,
	                   bool& isFree) const;
};

class TriggerParser : public TriggerCompleteParser {
public:
   explicit TriggerParser(DefsStructureParser* p) : TriggerCompleteParser(p) {}
	bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
	const char* keyword() const override { return "trigger"; }
};

class CompleteParser : public TriggerCompleteParser {
public:
   explicit CompleteParser(DefsStructureParser* p) : TriggerCompleteParser(p) {}
	bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override ;
	const char* keyword() const override { return "complete"; }
};

#endif
