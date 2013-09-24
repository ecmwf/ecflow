#ifndef WHITELISTFILE_CPP_
#define WHITELISTFILE_CPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Parser for while list file
//============================================================================

#include <boost/noncopyable.hpp>
#include <string>
#include <map>


class WhiteListFile : private boost::noncopyable {
public:
	WhiteListFile(const std::string& file) : smsWhiteListFile_(file) {}
	~WhiteListFile() {}

	// Parse the file if any errors found raise a warning
	// The parser expects version number  4.4.5
	// first user name, second true is write access, false read access
	bool parse( std::map<std::string,bool>& validUsers, std::string& errorMsg );

	// Function used in test:
	// Will overwrite the existing file
	static bool createWithReadAccess( const std::string& pathToFile, std::string& errorMsg);
	static bool createWithWriteAccess( const std::string& pathToFile, std::string& errorMsg);

private:

	bool validateVersionNumber(const std::string& line, std::string& errorMsg) const;
	void addUser(const std::string& token, std::map<std::string,bool>& validUsers) const;

	std::string smsWhiteListFile_;
};

#endif

