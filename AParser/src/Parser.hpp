#ifndef PARSER_HPP_
#define PARSER_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #23 $ 
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
#include <vector>
#include <stack>
#include <map>

class DefsStructureParser;
class Node;
class Defs;

//#define SHOW_PARSER_STATS 1

class Parser {
public:
	Parser(DefsStructureParser* p);
	virtual ~Parser();

	// if child does not recognise token try the parent
	virtual bool doParse(const std::string& line, std::vector<std::string>& lineTokens);
	Parser* parent() const { return parent_;}
	void parent(Parser* p)   { parent_ = p;}

	virtual const char* keyword() const = 0;

	DefsStructureParser* rootParser() const { return rootParser_;}

	// convenience function that access DefsStructureParser
   std::stack< std::pair<Node*,const   Parser*> >& nodeStack() const;
   Node* nodeStack_top() const;
	std::map<Node*,bool >& defStatusMap() const;

#ifdef SHOW_PARSER_STATS
	// The following function used in parser stats only
	void printStats();
#endif

protected:

	Defs* defsfile() const;
	void dumpStackTop(const std::string& msg, const std::string& msg2 = "") const;
	void addParser(Parser* p);
	void popNode() const;
	void popToContainerNode();
	void dump(const std::vector<std::string>& lineTokens) const;
	void reserve_vec(int res) { expectedParsers_.reserve(res); }

private:

	bool hasChildren() const    { return !expectedParsers_.empty();}

	Parser* parent_;
	DefsStructureParser* rootParser_;
	std::vector<Parser*> expectedParsers_;

#ifdef SHOW_PARSER_STATS
	// The following function used in parser stats only
	void incrementParserCount() { parserCount_++;}
	int parserCount() const     { return parserCount_;}
	int parserCount_;
#endif
};

#endif
