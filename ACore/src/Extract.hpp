#ifndef EXTRACT_HPP_
#define EXTRACT_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $ 
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

#include <string>
#include <vector>
#include <boost/noncopyable.hpp>

class Extract : private boost::noncopyable {
public:

	// token if of the form:
	//    /path/to/home:obj        OR    obj
	//    path = /path/to/home           path = ""
	//    name = obj                     name = obj
	// return true for ok or false for error
	static bool pathAndName(const std::string& token, std::string& path, std::string& name);

   // Given str = HH:MM
   // return MM;
	static bool split_get_second(const std::string& str, std::string& ret,char separator = ':');

	/// extract integer or throw an std::runtime_error on failure,
	/// the error message passed in is used to configure the returned exception
	static int theInt(const std::string& token,const std::string& errorMsg) ;

	/// extract YMD, integer of the form yyyymmdd, will throw std::runtime_error on failure
	/// the error message passed in is used to configure the returned exception
	static int ymd(const std::string& ymdToken, std::string& errorMsg);

	/// extract optional int, else return -1
	/// the error message passed in is used to configure the returned exception
	// could have line of the form
	// repeat integer variable 1 2 #a comment
	// repeat integer variable 3 4 # a comment
	// hence we must check the first character, and not the complete token
	static int optionalInt(const std::vector<std::string>& lineTokens,
	                       int pos,int defaultValue,const std::string& errorMsg);
private:
	Extract() = delete;
};

#endif
