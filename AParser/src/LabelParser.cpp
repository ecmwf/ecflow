//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include "LabelParser.hpp"
#include "Node.hpp"
#include "DefsStructureParser.hpp"

using namespace std;
using namespace ecf;

bool LabelParser::doParse( const std::string& line,
                           std::vector<std::string >& lineTokens )
{
	if ( nodeStack().empty() ) {
		throw std::runtime_error("LabelParser::doParse: Could not add label as node stack is empty at line: " + line );
	}

	Label label;
	label.parse(line,lineTokens,rootParser()->get_file_type() != PrintStyle::DEFS);
	nodeStack_top()->addLabel( label );

	return true;
}
