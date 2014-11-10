//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #17 $ 
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

#include <stdexcept>
#include <sstream>

#include "TriggerParser.hpp"
#include "Str.hpp"
#include "DefsStructureParser.hpp"
#include "Node.hpp"

using namespace ecf;
using namespace std;
using namespace boost;


static bool hasExtension( const std::string& line, const std::vector< std::string >& lineTokens )
{
   //    cout << "hasExtension = ";
   if ( line[line.size() - 1] == '\\' ) {
      //       cout << "true\n";
      return true;
   }
   const std::string& lastToken = lineTokens.back();
   if ( lastToken == "\\" || lastToken[lastToken.size() - 1] == '\\' ) {
      //       cout << "true\n";
      return true;
   }
   //    cout << "false\n";
   return false;
}

// ===============================================================================

void TriggerCompleteParser::getExpression(
         const std::string& line,
         std::vector< std::string >& theLineTokens,
         std::string& expression,
         bool& andExp,
         bool& orExp,
         bool& isFree) const
{
	assert( *theLineTokens.begin() == keyword() );
	if ( theLineTokens.size() < 2 ) throw std::runtime_error( "Invalid " + std::string(keyword()) + " " + line );

	// trigger -a n == complete
	// complete -o n == complete
	if (theLineTokens[1] == "-a") {
		andExp = true;
		theLineTokens.erase(theLineTokens.begin() + 1);
	}
	else if (theLineTokens[1] == "-o") {
		orExp = true;
		theLineTokens.erase(theLineTokens.begin() + 1);
	}


	// Handle continuations, by removing them and adding to expression
	if ( hasExtension( line, theLineTokens ) ) {
		/*
		     trigger a == complete and  /
			         b == complete
		 */
		std::vector< std::string > accumalatedTokens = theLineTokens;
		while ( true ) {

			std::string line2;
			rootParser()->getNextLine( line2 );

			std::vector< std::string > lineTokens2;
			Str::split( line2, lineTokens2 );

			std::copy( lineTokens2.begin(), lineTokens2.end(),
						std::back_inserter( accumalatedTokens ) );

			if ( !hasExtension( line2, lineTokens2 ) ) {
				break;
			}

			if ( accumalatedTokens.back() == "\\" ) { // remove continuation
				accumalatedTokens.pop_back();
			}
		}

      size_t accumalated_tokens_size = accumalatedTokens.size();
		for (size_t i = 1; i < accumalated_tokens_size; i++) {
			std::string token = accumalatedTokens[i];
			if ( token[token.size() - 1] == '\\' ) {
				token.erase( token.begin() + token.size() - 1 );
			}
			if ( token.empty() )
				continue;
			if ( token.at( 0 ) == '#' )
				break;
			if ( i != 1 )
				expression += " ";
			expression += token;
		}
	}
	else {

		// lineTokens[0] == "trigger";/ "complete"
	   size_t line_token_size = theLineTokens.size();
	   expression.reserve(line.size());
		for (size_t i = 1; i < line_token_size; i++) {
			if ( theLineTokens[i].at( 0 ) == '#' ) break;
			if ( i != 1 ) expression += " ";
			expression += theLineTokens[i];
		}

	   // state
	   if (rootParser()->get_file_type() != PrintStyle::DEFS) {
	      bool comment_fnd =  false;
	      for(size_t i = 3; i < line_token_size; i++) {
	         if (comment_fnd) {
	            if (theLineTokens[i] == "free") {
	               isFree = true;
	               break;
	            }
	         }
	         if (theLineTokens[i] == "#") comment_fnd = true;
	      }
	   }
	}

//	cout << "expression = '" << expression << "'\n";
	if ( expression.empty() ) throw std::runtime_error( "Invalid trigger " + line );
}


bool TriggerParser::doParse( const std::string& line, std::vector<std::string >& lineTokens )
{
	bool andExp = false;
	bool orExp = false;
	bool isFree = false;
	std::string expression;
	getExpression( line, lineTokens, expression, andExp , orExp, isFree);

	if ( !nodeStack().empty() ) {
		Node* node = nodeStack_top();
		if (!andExp && !orExp) node->add_part_trigger( PartExpression( expression )) ;
		else if (andExp)       node->add_part_trigger( PartExpression( expression, true)) ;
		else if (orExp)        node->add_part_trigger( PartExpression( expression, false)) ;
		else throw std::runtime_error( "Invalid trigger " + line );
		if (isFree) node->freeTrigger();
	}

	return true;
}

bool CompleteParser::doParse( const std::string& line, std::vector<std::string >& lineTokens )
{
	bool andExp = false;
	bool orExp = false;
	bool isFree = false;
	std::string expression;
	getExpression( line, lineTokens, expression, andExp , orExp, isFree);

	if ( !nodeStack().empty() ) {
		Node* node = nodeStack_top();
      if (!andExp && !orExp) node->add_part_complete( PartExpression( expression )) ;
      else if (andExp)       node->add_part_complete( PartExpression( expression, true)) ;
      else if (orExp)        node->add_part_complete( PartExpression( expression, false)) ;
      else throw std::runtime_error( "Invalid complete trigger " + line );
		if (isFree) node->freeComplete();
	}
	return true;
}
