//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #13 $ 
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
#include "CronParser.hpp"
#include "Node.hpp"
#include "DefsStructureParser.hpp"

using namespace ecf;
using namespace std;
using namespace boost;

//#define DEBUG_CRON 1

bool CronParser::doParse( const std::string& line, std::vector<std::string >& lineTokens )
{
	// cron 23:00                 # run every day at 23:00
	// cron 10:00 20:00 01:00     # run every hour between 10am and 8pm
	// cron -w 0,1 10:00          # run every sunday and monday at 10am
	// cron -d 10,11,12 12:00     # run 10th, 11th and 12th of each month at noon
	// cron -m 1,2,3 12:00        # run on Jan,Feb and March every day at noon.
	// cron -w 0 -m 5,6,7,8 10:00 20:00 01:00 # run every sunday, between May-Aug, every hour between 10am and 8pm
	if ( lineTokens.size() < 2 ) throw std::runtime_error( "CronParser::doParse: Invalid cron: " + line );

#ifdef DEBUG_CRON
	cerr << "CronParser::doParse " << line << "\n";
#endif

   bool parse_state = (rootParser()->get_file_type() != PrintStyle::DEFS);

	size_t index = 1; // to get over the cron
	CronAttr cronAttr;
	CronAttr::parse( cronAttr,lineTokens, index, parse_state);

	nodeStack_top()->addCron( cronAttr  );
	return true;
}
