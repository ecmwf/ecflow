//#ifndef PASSWDFILE_CPP_
//#define PASSWDFILE_CPP_
////============================================================================
//// Name        :
//// Author      : map
//// Revision    : $Revision: #5 $
////
//// Copyright 2009-2016 ECMWF.
//// This software is licensed under the terms of the Apache Licence version 2.0
//// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
//// In applying this licence, ECMWF does not waive the privileges and immunities
//// granted to it by virtue of its status as an intergovernmental organisation
//// nor does it submit to any jurisdiction.
////
//// Description : Parser for passwd file
////============================================================================
///* TODO
//
//   crypt
//   add user+crypted-phrase
//   send-check
// */
//
//#include <boost/noncopyable.hpp>
//#include <string>
//#include <map>
//#include "Str.hpp"
//namespace ecf {
//
//class PasswdFile : private boost::noncopyable {
//public:
//  PasswdFile(const std::string& node, const std::string& port)
//    : node_(node)
//    , port_(port)
//    , cname_ (getenv("ECF_PASSWD"))
//    , default_("")
//    , fname_ (cname_ ? cname_ : "")
//
//  { /* http://www.gnu.org/software/libc/manual/html_node/crypt.html */
//    seed[0] = time(NULL);
//    seed[1] = getpid() ^ (seed[0] >> 14 & 0x30000);
//    strcpy(salt, "$1$........");
//    const char * seedchars = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
//    for (int i = 0; i < 8; i++)
//      salt[3+i] = seedchars[(seed[i/5] >> (i%5)*6) & 0x3f];
//
//  }
//
//  ~PasswdFile() {}
//
//  // Parse the file if any errors found raise a warning
//  // The parser expects version number  4.4.5
//  // first user name, second true is write access, false read access
//  bool parse();
//
//  bool is_valid(std::string& path, std::string& phrase);
//  const std::string& get_phrase(std::string& path);
//
//private:
//  bool validateVersionNumber(const std::string& line,
//			     std::string& errorMsg) const;
//  bool addLine(const std::string& line);
//  // , std::map<std::string,std::string>& validLines) const;
//
//  const std::string node_;
//  const std::string port_;
//  const char* cname_;
//  const std::string default_;
//  std::map<std::string,std::string> record_;
//  unsigned long seed[2];
//  // char salt[] = "$1$........";
//  char salt[11]; // = "$1$........";
//
//public:
//  const std::string fname_; // filename from ECF_PASSWD public
//};
//};
//
//#endif
