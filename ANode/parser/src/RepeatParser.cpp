//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #31 $ 
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

#include "RepeatParser.hpp"
#include "Extract.hpp"
#include "Str.hpp"
#include "Node.hpp"
#include "DefsStructureParser.hpp"

using namespace ecf;
using namespace std;
using namespace boost;


bool RepeatParser::doParse( const std::string& line,
                            std::vector<std::string >& lineTokens )
{
   size_t line_token_size = lineTokens.size();
	if ( line_token_size < 3 ) throw std::runtime_error( "RepeatParser::doParse: Invalid repeat " + line );
	if ( nodeStack().empty() )   throw std::runtime_error("RepeatParser::doParse: Could not add repeat as node stack is empty at line: " + line );

	string errorMsg = "Invalid repeat : ";
	errorMsg += line;

	if ( lineTokens[1] == "date" ) {
		// repeat date VARIABLE yyyymmdd yyyymmdd [delta]
		if ( line_token_size < 5 ) throw std::runtime_error( errorMsg );

		string name = lineTokens[2];
		int startYMD = Extract::ymd( lineTokens[3], errorMsg );
		int endYMD = Extract::ymd( lineTokens[4], errorMsg );
		int delta = Extract::optionalInt( lineTokens, 5, 1, errorMsg );
		RepeatDate rep( name, startYMD, endYMD, delta );
		int value = 0;
		if (get_value(lineTokens,value)) rep.set_value(value);

		nodeStack_top()->addRepeat( Repeat( rep ) ) ;
	}
   else if ( lineTokens[1] == "datelist" ) {

      if ( line_token_size < 4 ) throw std::runtime_error( errorMsg );

      // repeat datelist VARIABLE "YYYYMMDD" "YYYYMMDD" "YYYYMMDD" # comment
      string name = lineTokens[2];
      std::vector<int> theEnums; theEnums.reserve(line_token_size);
      for(size_t i = 3; i < line_token_size; i++) {
         std::string theEnum = lineTokens[i];
         if (theEnum[0] == '#') break;
         Str::removeSingleQuotes(theEnum);// remove quotes, they get added back when we persist
         Str::removeQuotes(theEnum);      // remove quotes, they get added back when we persist

         int date = 0;
         try { date = boost::lexical_cast< int >(theEnum );}
         catch ( boost::bad_lexical_cast& ) {
            throw std::runtime_error( "RepeatParser::doParse: repeat datelist " + name + ", invalid date : " + theEnum  );
         }
         theEnums.push_back(date);
      }
      if ( theEnums.empty() ) throw std::runtime_error( errorMsg );

      RepeatDateList rep( name, theEnums) ;
      int index = 0; // This is *assumed to be the index* and not the value
      if (get_value(lineTokens,index)) rep.set_value(index);

      nodeStack_top()->addRepeat( Repeat( rep ) ) ;
   }
	else if ( lineTokens[1] == "enumerated" ) {

		if ( line_token_size < 4 ) throw std::runtime_error( errorMsg );

		// repeat enumerated VARIABLE "first" "second" "last" # comment
		string name = lineTokens[2];
		std::vector<std::string> theEnums; theEnums.reserve(line_token_size);
		for(size_t i = 3; i < line_token_size; i++) {
			std::string theEnum = lineTokens[i];
			if (theEnum[0] == '#') break;
		   Str::removeSingleQuotes(theEnum);// remove quotes, they get added back when we persist
			Str::removeQuotes(theEnum);      // remove quotes, they get added back when we persist
			theEnums.push_back(theEnum);
		}
		if ( theEnums.empty() ) throw std::runtime_error( errorMsg );

		RepeatEnumerated rep( name, theEnums) ;
      int index = 0; // This is *assumed to be the index* and not the value
      if (get_value(lineTokens,index)) rep.set_value(index);

 		nodeStack_top()->addRepeat( Repeat( rep ) ) ;
 	}
	else if ( lineTokens[1] == "integer" ) {
		// repeat integer VARIABLE start end [step]
		if ( line_token_size < 5 ) throw std::runtime_error( errorMsg );

		string name = lineTokens[2];
 		int start = Extract::theInt(lineTokens[3],errorMsg);
		int end =   Extract::theInt(lineTokens[4],errorMsg);
		int step =  Extract::optionalInt(lineTokens,5, 1,errorMsg );
		RepeatInteger rep( name, start, end, step );
		int value = 0;
		if (get_value(lineTokens,value)) rep.set_value(value);

 		nodeStack_top()->addRepeat( Repeat( rep ) ) ;
	}
	else if ( lineTokens[1] == "day" ) {
		// repeat day step [ yyyymmdd ]  # the step can be positive or negative
		// *** See RepeatAttr.h ***
		// *** We will not support end date until there is a clear requirement for this
		int step = Extract::theInt(lineTokens[2],"Invalid repeat day:");

		// report end date as a parser error
		if ( line_token_size >= 4 && lineTokens[3][0] != '#') {
			throw std::runtime_error( "RepeatParser::doParse: repeat day, <end-date> not supported: " + line );
  	 	}
		// day has no state
		nodeStack_top()->addRepeat( Repeat( RepeatDay( step ) ) ) ;
	}
	else if ( lineTokens[1] == "string" ) {

		if ( line_token_size < 4 ) throw std::runtime_error( errorMsg );

		string name = lineTokens[2];
		std::vector<std::string> theEnums; theEnums.reserve(line_token_size);
		for(size_t i = 3; i < line_token_size; i++) {
			std::string theEnum = lineTokens[i];
			if (theEnum[0] == '#') break;
         Str::removeSingleQuotes(theEnum);// remove quotes, they get added back when we persist
			Str::removeQuotes(theEnum);      // remove quotes, they get added back when we persist
			theEnums.push_back(theEnum);
		}
		if ( theEnums.empty() ) throw std::runtime_error( errorMsg );

		RepeatString rep( name, theEnums) ;
      int index = 0;
      if (get_value(lineTokens,index)) rep.set_value(index);

 		nodeStack_top()->addRepeat( Repeat( rep ) ) ;
	}
	else if ( lineTokens[1] == "month" ) {
		// repeat month step [ yyyymmdd ]  # the step can be positive or negative
		throw std::runtime_error( "RepeatParser::doParse: repeat month not supported: " + line );

//		int endDate = 0;
//		int step = 0;
//		extractDayMonthYear( lineTokens, step, endDate );
//		nodeStack_top()->addRepeat( Repeat( RepeatMonth( step, endDate ) ) );
	}
	else if ( lineTokens[1] == "year" ) {
		// repeat year step [ yyyymmdd ]  # the step can be positive or negative
		throw std::runtime_error( "RepeatParser::doParse: repeat year not supported: " + line );
//		int endDate = 0;
//		int step = 0;
//		extractDayMonthYear( lineTokens, step, endDate );
//		nodeStack_top()->addRepeat( Repeat( RepeatYear( step, endDate ) ) );
	}
	else if ( lineTokens[1] == "file" ) {
		// NOT SUPPORTED
 	}
	else {
		throw std::runtime_error( "RepeatParser::doParse: Invalid repeat " + line );
	}
	return true;
}

//void RepeatParser::extractDayMonthYear( const std::vector< std::string >& lineTokens,
//										int& x,
//										int& endDate )
//{
//	x = Extract::theInt( lineTokens[2], "invalid repeat" );
//	if ( lineTokens.size() >= 4 && lineTokens[3][0] != '#') {
//		std::string errorMsg = "Invalid repeat";
//		endDate = Extract::ymd( lineTokens[3], errorMsg );
// 	}
//	if ( ! nodeStack_top()->isSuite() ) {
//		throw std::runtime_error( "RepeatParser::doParse: Invalid repeat day/month/year only valid for suite " );
//	}
//}

bool RepeatParser::get_value(const std::vector< std::string >& lineTokens, int& value) const
{
   // state
   if (rootParser()->get_file_type() != PrintStyle::DEFS) {
      // search back for comment
      // repeat integer VARIABLE start end [step] # value
      std::string token_after_comment;
      for(size_t i = lineTokens.size()-1; i > 3; i--) {
         if (lineTokens[i] == "#") {
            // token after comment is the value
            value = Extract::theInt(token_after_comment,"RepeatParser::doParse, could not extract repeat value");
            return true;
         }
         else token_after_comment = lineTokens[i];
      }
   }
   return false;
}
