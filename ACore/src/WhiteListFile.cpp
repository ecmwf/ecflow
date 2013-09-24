//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Parser for white list file
//============================================================================
#include <pwd.h>       /* getpwuid */
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "WhiteListFile.hpp"
#include "File.hpp"
#include "Str.hpp"
#include "Log.hpp"

using namespace ecf;
using namespace std;
using namespace boost;

bool WhiteListFile::parse( std::map<std::string,bool>& validUsers, std::string& errorMsg )
{
	std::vector<std::string> lines;
 	if (File::splitFileIntoLines(smsWhiteListFile_,lines,true /* ignore empty lines */)) {

 		bool foundVersionNumber = false; // can read from version 4.4.5 onwards
 		for(size_t i = 0; i < lines.size(); ++i) {

 			if (lines[i].empty())   continue;
 			if (lines[i][0] == '#') continue; // ignore comments

 			std::string theLine = lines[i];
 			boost::algorithm::trim(theLine); // don't know why we get leading/trailing spaces

 			std::vector< std::string > lineTokens;
 			Str::split( theLine, lineTokens );
 			if ( lineTokens.empty() ) continue;


 			// version should be at the start
 			if (!foundVersionNumber) {

 				if (!validateVersionNumber(lineTokens[0], errorMsg )) {
 					std::stringstream ss;
 					ss << " " << i + 1 << ": " << lines[i] << "\n";
 					ss << "for ECF_LISTS file " << smsWhiteListFile_ << "\n";
 					errorMsg += ss.str();
 					return false;
 				}
 				foundVersionNumber = true;
 				continue;
 			}
 			else  {

 				addUser(lineTokens[0], validUsers);
 			}
 		}

 		return true;
 	}

 	errorMsg += "Could not open file specified by ECF_LISTS ";
 	errorMsg += smsWhiteListFile_;
 	errorMsg += "\n";
 	return false;
}


bool WhiteListFile::validateVersionNumber(const std::string& line, std::string& errorMsg) const
{
	// Expect 4.4.14, Current syntax in force after 4.4.5
	// If first character is NUMERIC and we have dots
	bool firstCharIsNumeric = Str::NUMERIC().find( line[0], 0 ) != string::npos;
	if ( firstCharIsNumeric && line.find( "." ) != string::npos) {

		std::vector< std::string > versionNumberTokens;
		Str::split( line, versionNumberTokens, "." );
		if ( versionNumberTokens.size() != 3 ) {
			std::stringstream ss;
			ss << "Expected version of the form <int>.<int>.<int> i.e 4.4.14. but found invalid version number\n";
			errorMsg += ss.str();
			return false;
		}

		try {
			int major = boost::lexical_cast< int >( versionNumberTokens[0] );
			int minor = boost::lexical_cast< int >( versionNumberTokens[1] );
			int part = boost::lexical_cast< int >( versionNumberTokens[2] );
			if ( major < 4  ) {
 				errorMsg += "Only white list files with a version >= 4.4.5 is supported\n";
				return false;
			}
			if ( major == 4 && minor < 4 ) {
 				errorMsg += "Only white list files with a version >= 4.4.5 is supported\n";
				return false;
			}
			if ( major == 4 && minor == 4  && part < 5) {
 				errorMsg += "Only white list files with a version >= 4.4.5 is supported\n";
				return false;
			}
		}
		catch ( boost::bad_lexical_cast& ) {
 			errorMsg += "Invalid version number \n";
			return false;
		}

		return true;
	}

	errorMsg +="The version number not found. The version number must appear before the users.\n";
	return false;
}


void WhiteListFile::addUser(const std::string& line, std::map<std::string,bool>& validUsers) const
{
	LOG_ASSERT(!line.empty(), "");
	if (line[0] == '-') {
		std::string user = line;
		user.erase(user.begin());
		validUsers.insert(std::make_pair(user,false));  // Read access
		return;
	}

	validUsers.insert(std::make_pair(line,true)); // write access
}

bool WhiteListFile::createWithReadAccess( const std::string& pathToFile,std::string& errorMsg)
{
	std::vector<std::string> lines; lines.reserve( 2 );

	lines.push_back("4.4.14");

	string user = "-";
	struct passwd * thePassWord = getpwuid ( getuid() );
	user += string( thePassWord->pw_name ) ;  // equivalent to the login name

 	lines.push_back(user);

	return File::create(pathToFile,lines,errorMsg);
}

bool WhiteListFile::createWithWriteAccess( const std::string& pathToFile , std::string& errorMsg)
{
	std::vector<std::string> lines; lines.reserve( 2 );

	lines.push_back("4.4.14");

 	struct passwd * thePassWord = getpwuid ( getuid() );
	string user( thePassWord->pw_name ) ;  // equivalent to the login name

 	lines.push_back(user);

	return File::create(pathToFile,lines,errorMsg);
}
