//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include "DefsStateParser.hpp"
#include "Defs.hpp"
#include "DefsStructureParser.hpp"

using namespace std;
using namespace boost;


bool DefsStateParser::doParse( const std::string& line, std::vector<std::string >& lineTokens )
{
// cout << "line = " << line << "\n";
   if ( lineTokens.size() < 2 ) throw std::runtime_error( "DefsStateParser::doParse Invalid defs_state " + line );

   if (lineTokens[1] == PrintStyle::to_string(PrintStyle::STATE)) rootParser()->set_file_type( PrintStyle::STATE );
   else if (lineTokens[1] == PrintStyle::to_string(PrintStyle::MIGRATE)) rootParser()->set_file_type( PrintStyle::MIGRATE );
   else throw std::runtime_error( "DefsStateParser::doParse: file type not specified : " + line );

   defsfile()->read_state(line,lineTokens); // this can throw
   return true;
}

bool HistoryParser::doParse( const std::string& line, std::vector<std::string >& lineTokens )
{
// cout << "line = " << line << "\n";
   defsfile()->read_history(line,lineTokens); // this can throw
   return true;
}
