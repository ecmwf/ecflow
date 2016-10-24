#ifndef PASSWDFILE_CPP_
#define PASSWDFILE_CPP_
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
#include <map>
#include <vector>

// ----------------------------------------------------------------
//
class Passwd {
public:
   Passwd(const std::string& user,const std::string& host,const std::string& port, const std::string& passwd )
   : user_(user),host_(host),port_(port),passwd_(passwd) {}
   ~Passwd() {}

   bool operator==(const Passwd& rhs) const { return rhs.user_ == user_ && rhs.host_ == host_ && rhs.port_ == port_ && rhs.passwd_ == passwd_;}
   const std::string& user() const { return user_;}
   const std::string& host() const { return host_;}
   const std::string& port()  const { return port_;}
   const std::string& passwd() const { return passwd_;}

private:
   std::string user_;
   std::string host_;
   std::string port_;
   std::string passwd_;
};

// This class is used to authenticate, user commands, i.e like ping,alter, etc
class PasswdFile : private boost::noncopyable {
public:
   PasswdFile();
   ~PasswdFile();

   // Parse the file if any errors found return false and errorMsg
   // The parser expects version number  4.5.0
   bool load(const std::string& file,bool debug,  std::string& errorMsg);

   std::string get_passwd(const std::string& user, const std::string& host, const std::string& port);

   bool authenticate(const std::string& user, const std::string& passwd) const ;

   const std::vector<Passwd>& passwds() const { return vec_;}

   std::string dump() const;

   // Function used in test:
   // Will overwrite the existing file
   static bool createWithAccess(
         const std::string& pathToFile,
         const std::string& host,
         const std::string& port,
         const std::string& passwd,
         std::string& errorMsg);

private:
   bool add_user(std::vector<std::string>& tokens, std::string& error_msg);
   bool validateVersionNumber(const std::string& line, std::string& errorMsg) const;

private:

   std::string passwd_file_;
   std::vector<Passwd> vec_;
};

#endif
