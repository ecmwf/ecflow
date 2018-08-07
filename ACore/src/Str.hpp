#ifndef STR_HPP_
#define STR_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #50 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This class is used as a helper class
//============================================================================
#include <cstring> //for strcmp
#include <string>
#include <vector>
#include <limits>
#include <boost/noncopyable.hpp>

#include <algorithm>
#include <boost/algorithm/string.hpp>

namespace ecf {

class Str : private boost::noncopyable {
public:
	static int reserve_4()  { return 4; }
	static int reserve_8()  { return 8; }
	static int reserve_16() { return 17; }
	static int reserve_32() { return 32; }
	static int reserve_64() { return 64; }

	// remove any quotes on the string, else does nothing
	// "fred" -> fred
	//  fred  -> fred
	static void removeQuotes(std::string&);

	// remove any single quotes on the string, else does nothing
	// 'fred' -> fred
	//  fred  -> fred
	static void removeSingleQuotes(std::string&);

	/// Find 'stringToFind' in 'jobLine' and replace with string 'stringToRplace'
	/// return true if replace ok else returns false;
   static bool replace(std::string& subject, const std::string& stringToFind, const std::string& stringToRplace);
   static bool replace_all(std::string& subject, const std::string& stringToFind, const std::string& stringToRplace);
   static void replaceall(std::string& subject, const std::string& stringToFind, const std::string& stringToRplace);

   // extract data member value, ie given a string of the form:
   //   str=cmd a b fred:value
   //   data_member_name=fred:
   //   extract value
   static bool extract_data_member_value(const std::string& str, const std::string& data_member_name, std::string& data_member_value);

	/// split string using default delimiters of space and tab as a separator;
	/// The split is based on *ANY* of the characters in the delimiters.
	/// **** Hence a delimiter of "==" will still split "a = complete"
	/// **** sequential delimiter character are ignored ****
	static void split(const std::string& line,
	                  std::vector< std::string >& tokens,
	                  const std::string& delimiters = " \t");

	// Uses boost::make_split_iterator will remove
	// consecutive delimiters in the middle of the string
	// ** However preserves leading and trailing empty tokens *IF* delimiters at start/end
	//
	// Usage:
	//    boost::split_iterator<std::string::const_iterator> tokens = Str::make_split_iterator(str);
   //    for(; !tokens.eof(); ++tokens ) {
   //       boost::iterator_range<string::const_iterator> range = *tokens;
   //       std::string the_string(range.begin(), range.end()) ;
   //    }
	//
	//    std::vector<std::string> vec;
	//    typedef boost::split_iterator<std::string::const_iterator> split_iter_t;
	//    for(split_iter_t i = Str::split(s,delim); i != split_iter_t(); i++) {
	//       vec.push_back(boost::copy_range<std::string>(*i));
	//    }
	static boost::split_iterator<std::string::const_iterator> make_split_iterator(const std::string& str,const std::string& delimiters = " \t");

	/// case in sensitive string comparison
	static bool caseInsCompare( const std::string&, const std::string&);

	/// case insenstive less
   static bool caseInsLess( const std::string&, const std::string&);

   /// case insenstive Greater
   static bool caseInsGreater( const std::string&, const std::string&);

	/// Used for checking node names
   static bool valid_name(const std::string& name, std::string &msg);
   static bool valid_name(const std::string& name);

	/// Use this function when you are not sure if string is convertible to an integer.
 	/// This function will check if string has a numeric first & hence faster than
	/// using boost::lexical_cast< int > alone. Will trap any exception
	/// will return numeric_limits<int>::max() for invalid conversions
	static int to_int( const std::string&, int error_return = std::numeric_limits<int>::max() );

	/// Truncate the input string at the start/end if exceeds max_lines_ newlines
	/// returns true if truncated false otherwise
   static bool truncate_at_start(  std::string& fileContents, size_t max_lines);
   static bool truncate_at_end(  std::string& fileContents, size_t max_lines);

	/// Only use strcmp if the first characters are the same
	static int local_strcmp(const char* s, const char* t)
	{
	   return (*s != *t ? *s - *t : strcmp(s, t));
	}

	// returns a static string of alpha numerics and underscore
   static const std::string& ALPHANUMERIC_UNDERSCORE();

   // returns a static string of numerics chars
   static const std::string& NUMERIC();

   static const char* CHILD_CMD();
   static const char* USER_CMD();
   static const char* SVR_CMD(); // Only for automatic check_pt

	// Allows string to be returned by reference
	static const std::string& EMPTY() ;
	static const std::string& ROOT_PATH();      // "/"
	static const std::string& PATH_SEPERATOR() ; // "/"
	static const std::string& COLON() ;          // ":"

	static const std::string& STATE_CHANGE();

	static const std::string& TASK();
	static const std::string& FAMILY();
   static const std::string& SUITE();
   static const std::string& ALIAS();

   static const std::string& DEFAULT_PORT_NUMBER(); // "3141"
	static const std::string& LOCALHOST();

   static const std::string& ECF_PORT();
   static const std::string& ECF_RID();
   static const std::string& ECF_TRYNO();
   static const std::string& ECF_TRIES();
   static const std::string& ECF_NAME();
   static const std::string& ECF_HOST();
   static const std::string& ECF_PASS();
   static const std::string& ECF_JOB();
   static const std::string& ECF_JOBOUT();
   static const std::string& ECF_SCRIPT();
   static const std::string& ECF_DUMMY_TASK();
   static const std::string& ECF_NO_SCRIPT();
   static const std::string& ECF_MICRO();
   static const std::string& ECF_FILES();
   static const std::string& ECF_FETCH();
   static const std::string& ECF_KILL_CMD();
   static const std::string& ECF_STATUS_CMD();

   static const std::string& ECF_HOME();
   static const std::string& ECF_INCLUDE();
   static const std::string& ECF_JOB_CMD();
   static const std::string& ECF_OUT();
   static const std::string& ECF_EXTN();
   static const std::string& ECF_LOG();

   static const std::string& WHITE_LIST_FILE();
   static const std::string& ECF_PASSWD();
private:
	Str(){}
};

}

#endif
