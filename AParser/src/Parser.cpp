//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #34 $ 
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
#include <sstream>
#include <fstream>
#include <boost/foreach.hpp>

#include "Parser.hpp"
#include "DefsStructureParser.hpp"
#include "Stl.hpp"
#include "Str.hpp"
#include "Indentor.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
using namespace boost::gregorian;

//#define DEBUG_PARSER 1

template<class T>
ostream& operator<<(ostream& os, const vector<T>& v) {
    copy(v.begin(), v.end(), ostream_iterator<T>(cout, ","));
    return os;
}



// ===============================================================================

Parser::Parser( DefsStructureParser* p )
: parent_(NULL),
  rootParser_( p )
#ifdef SHOW_PARSER_STATS
  ,parserCount_(0)
#endif
{}

Parser::~Parser() {
	DeletePtrs( expectedParsers_ );
}

bool Parser::doParse(const std::string& line, std::vector<std::string>& lineTokens)
{
   const char *first_token = lineTokens[0].c_str();
	size_t theSize = expectedParsers_.size();
	for(size_t i = 0; i < theSize; ++i) {
		Parser* p = expectedParsers_[i];

		if (Str::local_strcmp(first_token,p->keyword()) == 0) {

#ifdef SHOW_PARSER_STATS
			p->incrementParserCount();  // used for stats
#endif

			return p->doParse(line,lineTokens);
 		}
	}

#ifdef DEBUG_PARSER
	cerr << "Parser::" << keyword() << " token = '" << *lineTokens.begin() << "' did not match parsers(";
	BOOST_FOREACH(Parser* p, expectedParsers_) { cerr << " " <<  p->keyword() ; }
	cerr << ") Trying parent ";
	if (parent()) cout << "Parser::" << parent()->keyword();
	cerr << "\n";
#endif

	// Parent should handle "endfamily", "family" and "endsuite" for hierarchical families
	if (parent() && ((Str::local_strcmp(first_token,"endfamily") == 0)  ||
	                 (Str::local_strcmp(first_token,"family") == 0)  ||
	                 (Str::local_strcmp(first_token,"endsuite") == 0))) {
		return parent()->doParse(line,lineTokens);
	}

   // Check if first token is '#' comment character
   // very first non space character is # comment, hence ignore this line
   if (*first_token == '#') {
      // std::cout << "Ignoring line with leading comment : " << line << "\n";
      return true;
   }

   // Does not match any parser, or leading comment
	std::string errorMsg = "Unexpected keyword ";
	errorMsg += *lineTokens.begin();
 	errorMsg += " found whilst parsing ";
	errorMsg += keyword();
	if ( !nodeStack().empty() ) {
		errorMsg += " ";
		errorMsg += nodeStack_top()->name();
	}
	throw std::runtime_error( errorMsg );
	return false;
}


Defs* Parser::defsfile() const { return rootParser_->defsfile_ ;}
std::stack< std::pair<Node*,const Parser*> >& Parser::nodeStack() const { return rootParser_->nodeStack_;}
Node* Parser::nodeStack_top() const { return rootParser_->nodeStack_.top().first;}
std::map<Node*,bool>& Parser::defStatusMap() const { return rootParser_->defStatusMap_;  }

void Parser::dumpStackTop(const std::string& msg, const std::string& msg2) const
{
#ifdef DEBUG_NODE_STACK
    	std::cout << msg << "  '" << msg2 << "' ++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    	if (rootParser_->nodeStack_.empty()) std::cout << "nodeStack_ is EMPTY\n";
    	else  std::cout << "TOP = " <<  rootParser_->nodeStack_.top()->debugType()
    	                << " '" << rootParser_->nodeStack_.top()->name() << "'\n";
#endif
}

void Parser::addParser(Parser* p)
{
	p->parent(this);
	expectedParsers_.push_back(p);
}

void Parser::popNode() const { nodeStack().pop();}


void Parser::popToContainerNode() const
{
	while ( !nodeStack().empty() &&  !nodeStack_top()->isNodeContainer() ) {
 	     nodeStack().pop(); // keep poping till we get to family or suite
 	}
}

void Parser::dump(const std::vector<std::string>& lineTokens)
{
   cout << "tokens:";
   for(unsigned i=0; i < lineTokens.size(); i++) {
      cout << " '" << lineTokens[i] << "' ";
   }
   cout << "\n";
}


#ifdef SHOW_PARSER_STATS
void Parser::printStats()
{
 	Indentor::indent( std::cout ) << "Parser::" << keyword() << "\n";
	BOOST_FOREACH(Parser* p, expectedParsers_) {
		Indentor::indent( std::cout ) << p->keyword() << " " << p->parserCount() << "\n";
	}

	Indentor in;
	BOOST_FOREACH(Parser* p, expectedParsers_) {
 		if (p->hasChildren()) {
			p->printStats();
		}
	}
}
#endif

