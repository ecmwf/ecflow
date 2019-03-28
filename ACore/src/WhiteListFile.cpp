//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Parser for white list file
//============================================================================
#include <vector>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "WhiteListFile.hpp"
#include "File.hpp"
#include "Str.hpp"
#include "Log.hpp"
#include "User.hpp"

using namespace ecf;
using namespace std;
using namespace boost;

WhiteListFile::WhiteListFile()= default;

WhiteListFile::~WhiteListFile() = default;

bool WhiteListFile::verify_read_access(const std::string& user) const
{
   // Please note that the presence of * , even with paths means all users, have access
   // Command like --news has no paths, hence allow access to all if * specified
   if (all_users_have_read_access_) return true;
   if (users_with_read_access_.empty() && users_with_write_access_.empty() ) return true;

   if (users_with_read_access_.find(user) != users_with_read_access_.end()) return true;
   if (users_with_read_access_.find("*") !=  users_with_read_access_.end()) return true;

   // user with write access also have read access
   if (users_with_write_access_.find(user) != users_with_write_access_.end()) return true;
   if (users_with_write_access_.find("*") !=  users_with_write_access_.end()) return true;
   return false;
}

bool WhiteListFile::verify_write_access(const std::string& user) const
{
   if (all_users_have_write_access_) return true;
   if (users_with_read_access_.empty() && users_with_write_access_.empty() ) return true;

   if (verify_path_access(user,Str::EMPTY(),users_with_write_access_)) return true;
   if (verify_path_access("*", Str::EMPTY(),users_with_write_access_)) return true;
   return false;
}

bool WhiteListFile::verify_read_access(const std::string& user, const std::string& path) const
{
   if (all_users_have_read_access_) return true;
   if (users_with_read_access_.empty() && users_with_write_access_.empty() ) return true;
   if (verify_path_access(user,path,users_with_read_access_)) return true;
   if (verify_path_access("*",path,users_with_read_access_)) return true;

   // user with write access also have read access
   if (verify_path_access(user,path,users_with_write_access_)) return true;
   if (verify_path_access("*",path,users_with_write_access_)) return true;

   return false;
}

bool WhiteListFile::verify_write_access(const std::string& user,const std::string& path) const
{
   if (all_users_have_write_access_) return true;
   if (users_with_read_access_.empty() && users_with_write_access_.empty() ) return true;

   if (verify_path_access(user,path,users_with_write_access_)) return true;
   if (verify_path_access("*",path,users_with_write_access_)) return true;

   return false;
}

bool WhiteListFile::verify_read_access(const std::string& user, const std::vector<std::string>& paths) const
{
//   std::cout << "WhiteListFile::verify_read_access user " << user << " paths( ";
//   for(size_t i = 0; i < paths.size(); i++) { std::cout << paths[i] << " "; }
//   std::cout << ") \n";
//   std::cout << "  all_users_have_read_access_: " << all_users_have_read_access_ << "\n";
//   std::cout << "  users_with_read_access_.size(): " << users_with_read_access_.size() << "\n";
//   std::cout << "  users_with_write_access_.size(): " << users_with_write_access_.size() << "\n";
//   std::cout << dump_valid_users() << "\n\n";

   if (all_users_have_read_access_) return true;
   if (users_with_read_access_.empty() && users_with_write_access_.empty() ) return true;
   if (verify_path_access(user,paths,users_with_read_access_)) return true;
   if (verify_path_access("*",paths,users_with_read_access_)) return true;

   // user with write access also have read access
   if (verify_path_access(user,paths,users_with_write_access_)) return true;
   if (verify_path_access("*",paths,users_with_write_access_)) return true;

   return false;
}

bool WhiteListFile::verify_write_access(const std::string& user, const std::vector<std::string>& paths) const
{
   if (all_users_have_write_access_) return true;
   if (users_with_read_access_.empty() && users_with_write_access_.empty() ) return true;
   if (verify_path_access(user,paths,users_with_write_access_)) return true;
   if (verify_path_access("*",paths,users_with_write_access_)) return true;
   return false;
}

static bool path_access(const std::vector<std::string>& paths,const std::vector<std::string>& allowed_paths)
{
   if (allowed_paths.empty()) return true; // no paths is specified in PASSWORD file allow access

   // Paths specified in PASSWORD file.
   if ( paths.empty())  return false; // INPUT user path empty deny access

   // when we have a set of paths, supplied the the GUI. These can be random path selections
   // Hence to allow access *ALL* paths need to pass access
   //
   // need to correctly check for subsets
   //    allowed_paths              paths               valid
   //    /ecflow                    /ecflow_1182        FALSE
   //    /ecflow/fred               /ecflow/freddy      FALSE
   //    /ecflow/fred               /ecflow/fred/me     TRUE

   size_t allowed_paths_size = allowed_paths.size();
   size_t paths_size = paths.size();
   for(size_t i = 0; i < paths_size; i++) {
      const std::string& path = paths[i];
      bool found_path_in_allowed_paths = false;

      for(size_t ap = 0; ap < allowed_paths_size; ap++) {
         const std::string& allowed_path = allowed_paths[ap];
//         cout << "path " << path << "\n";
//         cout << "allowed_path " <<  allowed_path << "\n";
//         if (path.size() > allowed_paths[ap].size()) {
//            cout << "path[allowed_path.size()] " << path[ allowed_path.size()] << "\n";
//         }
         if (path.find(allowed_path) == 0) {
            if (path.size() > allowed_path.size() && path[allowed_path.size()] != '/') {
               // FALSE case above
               continue;
            }

            found_path_in_allowed_paths = true;
            continue; // all paths must match, or fail
         }
      }
      if (!found_path_in_allowed_paths) return false;
   }
   return true;
}

static bool path_access(const std::string& path,const std::vector<std::string>& allowed_paths)
{
   if (allowed_paths.empty()) return true;
   if (path.empty())  return false;

   size_t allowed_paths_size = allowed_paths.size();
   for(size_t ap = 0; ap < allowed_paths_size; ap++) {
      const std::string& allowed_path = allowed_paths[ap];

      if (path.find(allowed_path)  == 0) {
         // found path in allowed paths, check correct SUB_PATH
         if (path.size() > allowed_path.size() && path[allowed_path.size()] != '/') {
            // FALSE case above
            continue;
         }
         return true;
      }
   }
   return false;
}

bool WhiteListFile::verify_path_access(const std::string& user,const std::vector<std::string>& paths,const mymap& user_path_map) const
{
   auto it = user_path_map.find(user);
   if (it != user_path_map.end()) {
      return path_access(paths,it->second);
   }
   return false;
}

bool WhiteListFile::verify_path_access(const std::string& user,const std::string& path,const mymap& user_path_map) const
{
   auto it = user_path_map.find(user);
   if (it != user_path_map.end()) {
      return path_access(path,it->second);
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
 		size_t lines_size = lines.size();
 		for(size_t i = 0; i < lines_size; ++i) {

 			if (lines[i].empty())   continue;

 			// ignore/remove all comments
 			if (lines[i][0] == '#') continue;
 			std::string theLine = lines[i];
 			string::size_type comment_pos = theLine.find("#");
 			if (comment_pos != std::string::npos ) {
 			   theLine.erase(comment_pos);
 			}

 			boost::algorithm::trim(theLine);  // remove leading and trailing spaces
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
 			  if (!add_user(lineTokens,errorMsg)) {
 			     return false;
 			  }
 			}
 		}

 		// remove duplicates
 		mymap::iterator i;
 		for(i=users_with_read_access_.begin(); i!= users_with_read_access_.end(); ++i) {
 		   std::vector<std::string>& paths = (*i).second;
 		   std::sort(paths.begin(), paths.end());
 		   paths.erase(std::unique(paths.begin(), paths.end()), paths.end());
 		}
 		for(i=users_with_write_access_.begin(); i!= users_with_write_access_.end(); ++i) {
 		   std::vector<std::string>& paths = (*i).second;
 		   std::sort(paths.begin(), paths.end());
 		   paths.erase(std::unique(paths.begin(), paths.end()), paths.end());
 		}

 		if (debug)  std::cout << dump_valid_users() << "\n";

 		return true;
 	}

 	errorMsg += "Could not open file specified by ECF_LISTS ";
 	errorMsg += white_list_file_;
   errorMsg += " (";
   errorMsg += strerror(errno);
   errorMsg += ")";
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

   mymap::const_iterator i;
   for(i=users_with_read_access_.begin(); i!= users_with_read_access_.end(); ++i) {
      ss << " User: " << (*i).first << " ";
      const std::vector<std::string>& paths = (*i).second;
      for(const auto & path : paths)  ss << path << ",";
      ss << " has read access\n";
   }
   for(i=users_with_write_access_.begin(); i!= users_with_write_access_.end(); ++i) {
      ss << " User: " << (*i).first << " ";
      const std::vector<std::string>& paths = (*i).second;
      for(const auto & path : paths)  ss << path << ",";
      ss << " has read/write access\n";
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
			auto major = boost::lexical_cast< int >( versionNumberTokens[0] );
			auto minor = boost::lexical_cast< int >( versionNumberTokens[1] );
			auto part = boost::lexical_cast< int >( versionNumberTokens[2] );
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


bool WhiteListFile::add_user(std::vector<std::string>& tokens, std::string& error_msg)
{
   //# please note if the same user appear multiple times, then the access right are additive
   //# Hence if the user has been given write access, then other change to access rights for
   //# same user are ignored
   //
   //# These user have read and write access to the server
   //uid1  # a comment
   //uid2  # a comment
   //cog   # a comment
   //
   //# Read only users
   //-*     # *all* users have read access to *all* suites
   //-fred
   //
   //*   # *all* user have read/write access, to *all* suites
   //
   //# access to suites
   //-fred /suiteX  # user fred is only allowed read access to suiteX only
   //bill    /suiteX,/suiteB # user bill is allowed read/write access to suiteX and suiteB only
   //-*      /open   # all user are allowed access to suite /open
   //joe    /        # has read/write access to everything, this same as 'joe' by itself
   //*      /        # all users have read/write access
   //*      /x,/y    # all user have read/write access to suite /x, /y

   // Use of * by itself means *all* user have read/write access to *all* suites
   if ( all_users_have_write_access_)  return true;

   if (tokens.size() == 1) {
      std::string user = tokens[0];
      if (user[0] == '-') {

         // Read access
         user.erase(user.begin());

         // if we see -* all users have read access
         if (user == "*") {
            all_users_have_read_access_ = true;
            users_with_read_access_.clear();
         }
         else {
            if (!all_users_have_read_access_)
               users_with_read_access_.insert(std::make_pair(user,std::vector<std::string>()));
         }
         return true;
      }

      // write access , this also IMPLIES read access
      // if we see * all users have read and write access
      if (user == "*") {
         all_users_have_write_access_ = true;
         all_users_have_read_access_ = true;
         users_with_write_access_.clear();
         users_with_read_access_.clear();
      }
      else {
         if (!all_users_have_read_access_)
            users_with_read_access_.insert(std::make_pair(user,std::vector<std::string>()));
         if (!all_users_have_write_access_)
            users_with_write_access_.insert(std::make_pair(user,std::vector<std::string>()));
      }
      return true;
   }

   bool clear_paths = false;
   bool read_only = false;
   std::string user;
   std::vector<std::string> paths;
   for(auto tok : tokens) {
      if (tok.empty()) continue;
      if (tok[0] == '-') {
         // read only user for a given set of paths
         read_only = true;
         tok.erase(tok.begin());
         if (!tok.empty()) {
            if (!user.empty()) { error_msg = "Can only have one user per line, first user:";error_msg+=user; error_msg+=" second user:"; error_msg+=tok;return false;}
            user = tok ; // user =  <name> || *
         }
      }
      else if (tok[0] == '*') {
         // all user have read/write access for a given set of suites ?
         if (!user.empty()) { error_msg = "Can only have one user per line, first user:";error_msg+=user; error_msg+=" second user:"; error_msg+=tok;return false;}
         user = "*";
      }
      else if (tok[0] == '/') {
         // path or set of paths
         std::vector<std::string> local_paths;
         Str::split(tok,local_paths,",");
         std::copy(local_paths.begin(),local_paths.end(),std::back_inserter(paths));

         // root path '/' means apply to all suites, in which case the paths may as well be empty.
         for(const auto & path : paths) {
            if (path == "/") {
               clear_paths = true;
               break;
            }
         }
      }
      else {
         // read/write user
         if (!user.empty()) { error_msg = "Can only have one user per line, first user:";error_msg+=user; error_msg+=" second user:"; error_msg+=tok;return false;}
         user = tok;
      }
   }

   if (clear_paths) {
      paths.clear(); // user has used root path '/'

      // * user means all users
      if (user == "*") {
         if (read_only) {
            // -* /
            all_users_have_read_access_ = true;
            users_with_read_access_.clear();
         }
         else {
            // * /
            all_users_have_write_access_ = true;
            users_with_write_access_.clear();
         }
      }
   }

   // check all paths start with '/'
   for(auto & path : paths) {
      if (!path.empty() && path[0] != '/') {
         error_msg = "Paths must start with '/'";
         return false;
      }
   }

   if (read_only) {
      if (!all_users_have_read_access_) {
         auto it = users_with_read_access_.find(user);
         if (it == users_with_read_access_.end()) users_with_read_access_.insert(std::make_pair(user,paths));
         else  std::copy(paths.begin(),paths.end(),std::back_inserter(it->second));
      }
   }
   else {
      if ( !all_users_have_write_access_) {
         auto it = users_with_write_access_.find(user);
         if (it == users_with_write_access_.end()) users_with_write_access_.insert(std::make_pair(user,paths));
         else  std::copy(paths.begin(),paths.end(),std::back_inserter(it->second));
      }
   }
   return true;
}

bool WhiteListFile::createWithReadAccess( const std::string& pathToFile,std::string& errorMsg)
{
	std::vector<std::string> lines; lines.reserve( 2 );

	lines.push_back("4.4.14");
 	lines.push_back("-" + User::login_name());

	return File::create(pathToFile,lines,errorMsg);
}

bool WhiteListFile::createWithWriteAccess( const std::string& pathToFile , std::string& errorMsg)
{
	std::vector<std::string> lines; lines.reserve( 2 );

	lines.push_back("4.4.14");
	lines.push_back(User::login_name()); // equivalent to the login name

	return File::create(pathToFile,lines,errorMsg);
}

bool WhiteListFile::createWithNoAccess( const std::string& pathToFile,std::string& errorMsg)
{
   std::vector<std::string> lines; lines.reserve(5);

   lines.push_back("4.4.14");
   lines.push_back("userXX");
   lines.push_back("userYY");
   lines.push_back("-userZZ");
   lines.push_back("-userZY");

   return File::create(pathToFile,lines,errorMsg);
}

bool WhiteListFile::createEmpty( const std::string& pathToFile,std::string& errorMsg)
{
   std::vector<std::string> lines; lines.reserve( 1 );
   lines.push_back("4.4.14");

   return File::create(pathToFile,lines,errorMsg);
}

void WhiteListFile::allow_write_access_for_server_user()
{
   std::string user = User::login_name();
   if (verify_write_access(user)) return;

   // add write access
   auto it = users_with_write_access_.find(user);
   if (it == users_with_write_access_.end()) {
      users_with_write_access_.insert(std::make_pair(user,std::vector<std::string>() ));
   }
}

