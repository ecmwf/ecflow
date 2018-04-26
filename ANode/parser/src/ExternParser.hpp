#ifndef EXTERNPARSER_HPP_
#define EXTERNPARSER_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $ 
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
//
// Externs: will typically have a absolute path, however sometimes when
//          suite are generated incrementally relative paths can be added
// extern a           path =  a
// extern /a/b/c      path = /a/b/c
// extern a/b/c       path = a/b/c
// extern /a/b/c:YMD  path = /a/b/c   variable:YMD (i.e event, meter, variable, repeat, generated variable)
//
// Externs are not persisted, why ?:
//   o Externs are un-resolved references to node paths in trigger expressions and inlimits
//     These references could be dynamically generated.
//   o Saves on network bandwidth and checkpoint file size.
// Hence externs are *ONLY* used on the client side.
//
class ExternParser : public Parser {
public:
   explicit ExternParser(DefsStructureParser* p) : Parser(p) {}
	virtual bool doParse(const std::string& line,std::vector<std::string>& lineTokens);
	virtual const char* keyword() const { return "extern"; }
};

#endif
