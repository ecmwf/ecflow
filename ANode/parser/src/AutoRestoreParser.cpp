//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #11 $
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

#include "../../ANode/parser/src/AutoRestoreParser.hpp"

#include "AutoRestoreAttr.hpp"
#include "TimeSeries.hpp"
#include "Extract.hpp"
#include "Node.hpp"

using namespace ecf;
using namespace std;

bool AutoRestoreParser::doParse( const std::string& line, std::vector<std::string >& lineTokens )
{
   // autorestore /s1/f1
   // autorestore ../f1

   if ( lineTokens.size() < 2 )  throw std::runtime_error( "AutoRestoreParser::doParse: Invalid autorestore :" + line );
   if ( nodeStack().empty() ) throw std::runtime_error("AutoRestoreParser::doParse: Could not add autorestore as node stack is empty at line: " + line );

   std::vector<std::string> nodes_to_restore;
   for (size_t i=1; i < lineTokens.size(); i++) {
      if (lineTokens[i][0] == '#') break;
      nodes_to_restore.push_back(lineTokens[i]);
   }

   if (nodes_to_restore.empty())  throw std::runtime_error("AutoRestoreParser::doParse: no paths specified " + line );

   nodeStack_top()->add_autorestore( ecf::AutoRestoreAttr(nodes_to_restore) ) ;

   return true;
}
