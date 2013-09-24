#ifndef VERIFYPARSER_HPP_
#define VERIFYPARSER_HPP_
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
// Description : A verify attribute parser. Note verify are only used for verification
//               and do not constitute to the structure of a definition file
//
//============================================================================

#include "Parser.hpp"

class VerifyParser : public Parser {
public:
	VerifyParser(DefsStructureParser* p) : Parser(p) {}
	virtual const char* keyword() const { return "verify"; }
	virtual bool doParse( const std::string& line, std::vector<std::string >& lineTokens );
};

#endif
