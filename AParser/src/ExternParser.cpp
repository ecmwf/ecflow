//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $ 
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

#include "ExternParser.hpp"
#include "Extract.hpp"
#include "Defs.hpp"

using namespace std;
using namespace boost;

bool ExternParser::doParse( const std::string& line,
                            std::vector<std::string >& lineTokens )
{
//	cout << "line = " << line << "\n";
	if ( lineTokens.size() < 2 )
		throw std::runtime_error( "ExternParser::doParse Invalid extern " + line );

	// Guard against
	// extern   # empty extern with a comment
	// extern   #empty extern with a comment
	if (lineTokens[1][0] == '#') {
		throw std::runtime_error( "ExternParser::doParse Invalid extern paths." + line );
	}

	// Expecting:
	//   extern <path>
	//   extern <path>:<attr>
	// where attr is the name of [ event, meter, repeat, variable, generated variable ]
	//
	// We will not split it up:

//	cout << "add extern  = '" << lineTokens[1] << "'\n";
	defsfile()->add_extern( lineTokens[1]);

	return true;
}
