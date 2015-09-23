//============================================================================
// Name        :
// Author      : map
// Revision    : $Revision: #1 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Parser for passwd file
//============================================================================
#include <stdio.h>
#include <time.h>
// #define _XOPEN_SOURCE
#include <unistd.h>
#include <crypt.h>
#include <pwd.h>       /* getpwuid */
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "PasswdFile.hpp"
#include "File.hpp"
#include "Str.hpp"
#include "Log.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace fs = boost::filesystem;

//const char * PasswdFile::seedchars; // = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

bool PasswdFile::parse()                        
{
  std::string errorMsg;
  LOG(Log::MSG, "loadPasswdFile:(" 
      << fname_ << ") CWD(" 
      << fs::current_path().string() << ")\n");

  if (fname_.empty()) {
    errorMsg += "The ECF_PASSWD file ";
    errorMsg += fname_;
    errorMsg += " has not been specified.";
    return false;
  }
  if (!fs::exists(fname_)) {
    errorMsg += "The ECF_PASSWD file ";
    errorMsg += fname_;
    errorMsg += " does not exist. Server CWD : " + fs::current_path().string();
    return false;
  }

  // std::map<std::string,std::string> validLines;
  std::vector<std::string> lines;

  if (!cname_) {
    std::cerr << "#MSG: you may use ECF_PASSWD to define path to phrase file\n";
    return false;
  } else if (access(cname_, F_OK)) {
    std::cerr << "#WAR: cannot open " << fname_ << " not set\n";    
    return false;
  } else if (File::splitFileIntoLines(fname_,lines,true)) {
    /* ignore empty lines */
    bool foundVersionNumber = false; 
    // can read from version 4.4.5 onwards
    for(size_t i = 0; i < lines.size(); ++i) {
	      
      if (lines[i].empty())   continue;
      if (lines[i][0] == '#') continue; // ignore comments
      
      std::string theLine = lines[i];
      boost::algorithm::trim(theLine); 
      // don't know why we get leading/trailing spaces
      
      std::vector< std::string > lineTokens;
      Str::split( theLine, lineTokens );
      if ( lineTokens.empty() ) continue;
      	      
      // version should be at the start
      if (!foundVersionNumber) {	
	if (!validateVersionNumber(lineTokens[0], errorMsg )) {
	  std::stringstream ss;
	  ss << " " << i + 1 << ": " << lines[i] << "\n";
	  ss << "for ECF_PASSWD file " << fname_ << "\n";
	  errorMsg += ss.str();
	  return false;
	}
	foundVersionNumber = true;
	continue;
      }
      else  {	
	addLine(lineTokens[0]); // , validLines);
      }
    }
    
    return true;
  }
  
  errorMsg += "Could not open file specified by ECF_PASSWD ";
  errorMsg += fname_;
  errorMsg += "\n";
  return false;
}


bool PasswdFile::validateVersionNumber(const std::string& line, 
			   std::string& errorMsg) const
{
  // Expect 4.4.14, Current syntax in force after 4.4.5
  // from WhiteListFile, thanks Avi
  bool firstCharIsNumeric = Str::NUMERIC().find( line[0], 0 ) != string::npos;
  if ( firstCharIsNumeric && line.find( "." ) != string::npos) {
    std::vector< std::string > versionNumberTokens;
    Str::split( line, versionNumberTokens, "." );
    if ( versionNumberTokens.size() != 3 ) {
      std::stringstream ss;
      ss << "Expected version of the form <int>.<int>.<int> i.e 4.4.5\n "
	 << "but found invalid version number\n";
      errorMsg += ss.str();
      std::cerr << errorMsg;
      return false;
    }
    
    try {
      int major = boost::lexical_cast< int >( versionNumberTokens[0] );
      int minor = boost::lexical_cast< int >( versionNumberTokens[1] );
      int part = boost::lexical_cast< int >( versionNumberTokens[2] );
      if ( major < 0  ) {
	errorMsg += "Only white list files with a version >= 0.0.1 is supported\n";
	return false;
      }
      if ( minor < 0 ) {
	errorMsg += "Only white list files with a version >= 0.0.1 is supported\n";
	return false;
      }
      if ( major == 0 && minor == 0  && part < 1) {
	errorMsg += "Only white list files with a version >= 0.0.1 is supported\n";
	return false;
      }
    }
    catch ( boost::bad_lexical_cast& ) {
      errorMsg += "Invalid version number\n";
      return false;
    }

    return true;
  }
  errorMsg +="The version number not found on top of passwd phrase file.\n";
  return false;
}


bool PasswdFile::addLine(const std::string& line) 
// std::map<std::string,std::string>& validLines) const
{
  if(line.empty()) return true;
  if (line[0] == '#') { return true; } // comment
  // FIXME trim chop line ???
  std::size_t found_sp  = line.find_first_of(" ");
  if (found_sp == std::string::npos) {
    std::cerr << "\n#WAR: passwdfile, no phrase found\n" << line ;
    return false; // syntax error
  }

  std::size_t found_sl  = line.find_first_of("/");
  if (found_sl == std::string::npos) {
    std::cerr << "\n#WAR: passwdfile, '/' not found\n" << line ;
    return false; // syntax error
  }

  if (line[0] == '/') { // /path/to/node "phrase"
    // nothing more to do here
  } else {
    std::size_t found_at  = line.find_first_of("@");
    if (found_at == std::string::npos) {
      std::cerr << "\n#WAR: passwdfile, '@' not found\n" << line ;
      return false; // syntax error
    }
    if (line.substr(0, found_at) != node_) return false;// ignore, host no match
    
    const std::string port(line.substr(found_at+1, found_sl));
    if (port != port_) return false; // ignore, port no match     
  }
  std::string path   = line.substr(found_sl, found_sp);
  std::string phrase = line.substr(found_sp);
  // validLines.insert(std::make_pair(path, phrase));

#if 0 /* test */
  record_.insert(std::make_pair(path, 
				crypt(phrase.c_str(), salt)
				// phrase
				));
#endif
  return true;
}

bool create( const std::string& pathToFile,std::string& errorMsg)
{
        std::vector<std::string> lines; lines.reserve( 2 );
        lines.push_back("# 4.4.14");
        // ostringstream convert; convert << port_;
        // const std::string line = node_ + "@" + convert.str() + "/";
        // lines.push_back(line);
        return File::create(pathToFile,lines,errorMsg);
}

bool PasswdFile::is_valid(std::string& path, std::string& phrase)
{
  return false;
}

const std::string& PasswdFile::get_phrase(std::string& path)
{
  return default_;
}
/* 
python -c "import crypt, getpass, pwd; \
           print crypt.crypt('password', '\$6\$saltsalt\$')"

echo "password" | sha512sum

man openssl

*/

const char* passwdTest = "0.0.1\n"
"#node@port/path/to/node phare to protect\n"
"# export ECF_PASSWD=$HOME/.ecf_passwd # before starting server/client\n"
"vsms2@43333/e_41r2    phare to protect\n"
"vsms2@43333/emc_41r2  phare to protect\n"
"vsms3@31415/o         phare to protect\n"
"/sapp test\n"
"# error, no phrase:\n"
"/xxx  \n"
"# error, no phrase:\n"
"server@123/xxx  \n"
"# error, no port:\n"
		   "server@abc/xxx  \n";
		   // R"( ... )"; // c++-0x raw string litteral
