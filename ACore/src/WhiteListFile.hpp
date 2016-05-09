#ifndef WHITELISTFILE_CPP_
#define WHITELISTFILE_CPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $ 
//
// Copyright 2009-2016 ECMWF. 
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
#include <set>

// ----------------------------------------------------------------
// The whitelist file, is used to indicate users who are allowed
// read/write access to the server when calling *user* commands

class WhiteListFile : private boost::noncopyable {
public:
	WhiteListFile();
	~WhiteListFile();

   bool allow_read_access(const std::string& user) const;
   bool allow_write_access(const std::string& user) const;
   std::string dump_valid_users() const;

	// Parse the file if any errors found return false and errorMsg
	// The parser expects version number  4.4.5
   bool load(const std::string& file,bool debug,  std::string& errorMsg);


	// Function used in test:
	// Will overwrite the existing file
	static bool createWithReadAccess( const std::string& pathToFile, std::string& errorMsg);
	static bool createWithWriteAccess( const std::string& pathToFile, std::string& errorMsg);

	size_t read_access_size() const { return users_with_read_access_.size(); }
	size_t write_access_size() const { return users_with_write_access_.size(); }

private:

	bool validateVersionNumber(const std::string& line, std::string& errorMsg) const;
	void add_user(const std::string& token);

   bool all_users_have_read_access_;
   bool all_users_have_write_access_;
	std::string white_list_file_;
   std::set<std::string> users_with_read_access_;
   std::set<std::string> users_with_write_access_;
};

#endif
