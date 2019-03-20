#ifndef WHITELISTFILE_CPP_
#define WHITELISTFILE_CPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Parser for while list file
//============================================================================

#include <string>
#include <unordered_map>
#include <vector>

// ----------------------------------------------------------------
// The whiteList file, is used to indicate users who are allowed
// read/write access to the server when calling *user* commands

class WhiteListFile {
public:
	WhiteListFile();
	~WhiteListFile();

	// If any user in read/write lists, then it has read access.
	// However when paths are specified teh returns may not me the same. i.e
	//    verify_read_access("userX") != verify_read_access("userX","/f")
	// This is is because different commands, call different functions:
	// Hence if we have:
	//              verify_read_access(userX) verify_read_access(userX,'/f') verify_read_access(userX,'/s1')
	//  userX  /s1               TRUE                  FALSE                        TRUE
	//  -userX /s2               TRUE                  FALSE                        FALSE
	//
	// If we also add * in the mix: All users have read/write acess to suite /a, and read access to /b
	//  *  /a
	//  -* /b
	//  userX  /s1
	//  -userX /s2
	// HENCE:
	//    verify_read_access(userX)   -> TRUE
	//    verify_read_access(yy)      -> TRUE  // yy qualifies as all users
	//   verify_read_access(userX,/a) -> TRUE
	//   verify_read_access(yy,/a)    -> TRUE
   bool verify_read_access(const std::string& user) const;
   bool verify_read_access(const std::string& user, const std::string& path) const;
   bool verify_read_access(const std::string& user, const std::vector<std::string>& paths) const;

   bool verify_write_access(const std::string& user) const;
   bool verify_write_access(const std::string& user,const std::string& path) const;
   bool verify_write_access(const std::string& user,const std::vector<std::string>& paths) const;

   std::string dump_valid_users() const;

	// Parse the file if any errors found return false and errorMsg
	// The parser expects version number  4.4.5
   bool load(const std::string& file,bool debug,  std::string& errorMsg);


	// Function used in test:
	// Will overwrite the existing file
	static bool createWithReadAccess( const std::string& pathToFile, std::string& errorMsg);
   static bool createWithWriteAccess( const std::string& pathToFile, std::string& errorMsg);
   static bool createWithNoAccess( const std::string& pathToFile, std::string& errorMsg);
   static bool createEmpty( const std::string& pathToFile, std::string& errorMsg);

	size_t read_access_size() const { return users_with_read_access_.size(); }
	size_t write_access_size() const { return users_with_write_access_.size(); }

	// return true if current user has write access
	void allow_write_access_for_server_user();

private:
  WhiteListFile(const WhiteListFile&) = delete;
  const WhiteListFile& operator=(const WhiteListFile&) = delete;

private:

	bool validateVersionNumber(const std::string& line, std::string& errorMsg) const;
	bool add_user(std::vector<std::string>& tokens, std::string& error_msg);

	typedef std::unordered_map<std::string,std::vector<std::string> > mymap;
   bool verify_path_access(const std::string& user,const std::vector<std::string>& paths,const mymap&) const;
   bool verify_path_access(const std::string& user,const std::string& path,const mymap&) const;

   bool all_users_have_read_access_{false};
   bool all_users_have_write_access_{false};
	std::string white_list_file_;
	mymap users_with_read_access_;   // user,paths
	mymap users_with_write_access_;  // user,paths
};

#endif
