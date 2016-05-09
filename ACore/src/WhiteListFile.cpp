//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $ 
//
// Copyright 2009-2016 ECMWF. 
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
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "WhiteListFile.hpp"
#include "File.hpp"
#include "Str.hpp"
#include "Log.hpp"

using namespace ecf;
using namespace std;
using namespace boost;

WhiteListFile::WhiteListFile()
:  all_users_have_read_access_(false),
   all_users_have_write_access_(false)
{
}

WhiteListFile::~WhiteListFile() {}


bool WhiteListFile::allow_read_access(const std::string& user) const
{
   if (all_users_have_read_access_) return true;
   if (users_with_read_access_.empty() && users_with_write_access_.empty() ) return true;

   if (users_with_read_access_.find(user) != users_with_read_access_.end()) {
      return true;
   }
   return false;
}

bool WhiteListFile::allow_write_access(const std::string& user) const
{
   if (all_users_have_write_access_) return true;
   if (users_with_read_access_.empty() && users_with_write_access_.empty() ) return true;

   if (users_with_write_access_.find(user) != users_with_write_access_.end()) {
      return true;
   }
   return false;
}

bool WhiteListFile::load(const std::string& file, bool debug, std::string& errorMsg )
{
   if (debug) {
      std::cout << "  White list file " << file << " opening...\n";
   }

   white_list_file_ = file;
   all_users_have_read_access_ = false;
   all_users_have_write_access_ = false;
   users_with_read_access_.clear();
   users_with_write_access_.clear();


	std::vector<std::string> lines;
 	if (File::splitFileIntoLines(white_list_file_,lines,true /* ignore empty lines */)) {

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
 					ss << "for ECF_LISTS file " << white_list_file_ << "\n";
 					errorMsg += ss.str();
 					return false;
 				}
 				foundVersionNumber = true;
 				continue;
 			}
 			else  {
 			  add_user(lineTokens[0]);
 			}
 		}

      if (debug)  std::cout << dump_valid_users() << "\n";

 		return true;
 	}

 	errorMsg += "Could not open file specified by ECF_LISTS ";
 	errorMsg += white_list_file_;
 	errorMsg += "\n";
   if (debug)  std::cout << dump_valid_users() << "\n";
 	return false;
}

std::string WhiteListFile::dump_valid_users() const
{
   std::stringstream ss;
   ss << "ECF_LISTS = '" << white_list_file_ << "'\n";
   if (users_with_read_access_.empty() && users_with_write_access_.empty()) ss << " No users specified. Everyone has read/write access\n";
   if (all_users_have_read_access_)  ss << " All users have read access\n";
   if (all_users_have_write_access_) ss << " All users have write access\n";

   std::set<std::string>::const_iterator i;
   for(i=users_with_read_access_.begin(); i!= users_with_read_access_.end(); ++i) {
      ss << " User: " << (*i) << " has read access\n";
   }
   for(i=users_with_write_access_.begin(); i!= users_with_write_access_.end(); ++i) {
      ss << " User: " << (*i) << " has read/write access\n";
   }

   return ss.str();
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


void WhiteListFile::add_user(const std::string& line )
{
	LOG_ASSERT(!line.empty(), "");
	if (line[0] == '-') {

	   // Read access
		std::string user = line;
		user.erase(user.begin());

		// if we see -* all users have read access
		if (user == "*") {
		   all_users_have_read_access_ = true;
		   users_with_read_access_.clear();
		}
		else {
		   if (!all_users_have_read_access_)
		      users_with_read_access_.insert(user);
		}
		return;
	}

	// write access , this also IMPLIES read access
   // if we see * all users have read and write access
   if (line == "*") {
      all_users_have_write_access_ = true;
      all_users_have_read_access_ = true;
      users_with_write_access_.clear();
      users_with_read_access_.clear();
   }
   else {
      if (!all_users_have_read_access_)
         users_with_read_access_.insert(line);
      if (!all_users_have_write_access_)
         users_with_write_access_.insert(line);
   }
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
