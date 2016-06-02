//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #6 $ 
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

#include "ZombieAttrParser.hpp"
#include "Node.hpp"

using namespace ecf;
using namespace std;

bool ZombieAttrParser::doParse( const std::string& line, std::vector<std::string >& lineTokens )
{
	// expect:  tokenizer
	//    zombie <zombie_type>: action : child_cmds :  lifetime
   //    zombie_type = [ user | ecf | path ]                                 # can only have one
   //    action      = [ fob | fail | block | remove | adopt ]               # can only have one
	//    child_cmd   = [ init, event, meter, label, wait, abort, complete ]  # can have mutiple
	//    zombie ecf:fob::                                                    # fob all child commands
	//    zombie ecf:fail:event,meter:200                                     # fail child command  event,meter and block other children
 	if ( lineTokens.size() < 2 ) throw std::runtime_error( "ZombieAttrParser::doParse: Invalid zombie :" + line );
	if (nodeStack().empty() )  throw std::runtime_error("Add zombie failed empty node stack");

	//cout << "ZombieAttrParser::doParse: " << lineTokens[1] << "\n";

  	nodeStack_top()->addZombie( ZombieAttr::create(lineTokens[1]) ) ;

	return true;
}
