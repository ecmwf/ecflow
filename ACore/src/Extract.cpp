//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $ 
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

#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>  // requires boost date and time lib
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "Extract.hpp"
#include "Stl.hpp"
#include "Str.hpp"

#include <sstream>
#include <fstream>

//#define DEBUG_PARSER 1

using namespace ecf;
using namespace std;
using namespace boost;
using namespace boost::gregorian;

template<class T>
ostream& operator<<(ostream& os, const vector<T>& v) {
    copy(v.begin(), v.end(), ostream_iterator<T>(cout, ","));
    return os;
}

bool Extract::pathAndName(const std::string& token, std::string& path, std::string& name)
{
//	cout << "Extract::pathAndName  token = " << token << "\n";
	// can have:
	//            /suite/family:obj      path = /suite/family   name = obj
	//            /suite/family          path = /suite/family   name =
	//            obj                    path =                 name = obj
	if (token.empty()) return false;

	size_t colonPos = token.find_first_of(':');
	if (colonPos == string::npos) {
		if (token[0] == '/') {
			path = token;       // token of the form /a/b/c, ie no name
		}
		else {
			name = token;
		}
	}
	else {
		path = token.substr(0,colonPos);
		name = token.substr(colonPos+1);
	}

//	cout << "Extract::pathAndName token=" << token << " path= '" << path << "' name= '" << name << "'\n";
    return true;
}

bool Extract::split_get_second(const std::string& str, std::string& ret,char separator)
{
   // HH:MM
   // return MM;
   size_t colonPos = str.find_first_of(separator);
   if (colonPos == string::npos)  return false;
   ret = str.substr(colonPos+1);
   return true;
}

/// extract integer or throw an std::runtime exception on failure
int Extract::theInt( const std::string& token, const std::string& errorMsg )
{
	int the_int = -1;
	try {
		the_int = boost::lexical_cast< int >( token );
	}
	catch ( boost::bad_lexical_cast& e ) {
		throw std::runtime_error(errorMsg );
	}
	return the_int;
}

/// extract YMD, integer of the form yyyymmdd
int Extract::ymd(const std::string& ymdToken,  std::string& errorMsg)
{
	if (ymdToken.size() != 8) throw std::runtime_error( errorMsg + " YMD must be 8 characters i.e yyyymmdd");

	// Use date lib to check YMD
	try { boost::gregorian::date(from_undelimited_string(ymdToken)); }
	catch (std::exception& e) {
		errorMsg += "\n";
		errorMsg += e.what();
		throw std::runtime_error( errorMsg + " YMD is not a valid date" );
	}

	return theInt(ymdToken,errorMsg);
}

int Extract::optionalInt(      const std::vector<std::string>& lineTokens,
                               int pos,
                               int defaultValue,
                               const std::string& errorMsg )
{
	// token0 token1 token2 token3   size = 4
	// pos0   pos1   pos2   pos3
	//
	// could have line of the form
	// repeat integer variable 1 2 #a comment
	// repeat integer variable 3 4 # a comment
	// hence we must check the first character, and not the complete token

	int the_int = defaultValue;
	if (static_cast<int>(lineTokens.size()) >= pos+1 && lineTokens[pos][0] != '#' ) {

		the_int = theInt(lineTokens[pos],errorMsg);
	}
	return the_int;
}
