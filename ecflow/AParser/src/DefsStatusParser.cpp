//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #11 $ 
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
#include "DefsStatusParser.hpp"
#include "DState.hpp"
#include "Node.hpp"

using namespace ecf;
using namespace std;
using namespace boost;

bool DefsStatusParser::doParse(
                                const std::string& line,
                                std::vector<std::string >& lineTokens )
{
	if ( lineTokens.size() < 2 ) throw std::runtime_error( "DefsStatusParser::doParse: Invalid defstatus :" + line );

	if ( ! DState::isValid(lineTokens[1])  ) {
		throw std::runtime_error( "DefsStatusParser::doParse: Invalid defstatus state :" + line );
	}

	if ( !nodeStack().empty() ) {
		Node* node = nodeStack_top();

		// Check default status not already defined for this node.
		std::map< Node*, bool >::const_iterator it = defStatusMap().find( node );
		if ( it != defStatusMap().end() ) {
			if ( (*it).second ) {
				std::stringstream ss;
				ss << "DefsStatusParser::doParse: " << node->debugType() << " " << node->name() << " already has a default status\n";
				throw std::runtime_error( ss.str() );
			}
		}
		defStatusMap()[node] = true;
		node->addDefStatus( DState::toState( lineTokens[1] ) );
	}

	return true;
}
