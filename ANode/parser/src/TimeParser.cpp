//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #17 $ 
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

#include "TimeParser.hpp"
#include "TimeSeries.hpp"
#include "Node.hpp"
#include "DefsStructureParser.hpp"


using namespace ecf;
using namespace std;

bool TimeParser::doParse( const std::string& line, std::vector<std::string >& lineTokens )
{
	if ( lineTokens.size() < 2 )
		throw std::runtime_error( "TimeParser::doParse: Invalid time :" + line );

   bool parse_state = false;
   bool isFree = false;
   if (rootParser()->get_file_type() != PrintStyle::DEFS) {
      parse_state = true;
      bool comment_fnd =  false;
      for(size_t i = 2; i < lineTokens.size(); i++) {
         if (comment_fnd && lineTokens[i] == "free") isFree = true;
         if (lineTokens[i] == "#") comment_fnd = true;
      }
   }

   size_t index = 1;
   TimeAttr attr( TimeSeries::create(index,lineTokens,parse_state) );
   if (isFree) attr.setFree();

   nodeStack_top()->addTime( attr );
   return true;
}

