/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
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
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "GenericParser.hpp"
#include "Node.hpp"
#include "Extract.hpp"

using namespace ecf;
using namespace std;

bool GenericParser::doParse( const std::string& line, std::vector<std::string >& lineTokens )
{
   // expect:
   //    generic <key> <value1> <value2> .... # actual  i.e
   //    generic fred va1 val2 #  sdsdsd
   //    generic fred # sdsdsd
   if ( lineTokens.size() < 2 ) throw std::runtime_error( "GenericParser::doParse: Invalid generic :" + line );

   if ( !nodeStack().empty() ) {
      Node* node = nodeStack_top();

      std::string key = lineTokens[1];
      std::vector<std::string> values;

      for (size_t i = 2; i < lineTokens.size(); i++) {
         if (lineTokens[i][0] == '#') break;
         values.push_back(lineTokens[i]);
      }

      node->add_generic( GenericAttr(key,values) ) ;
   }
   return true;
}
