#ifndef ECF_PORT_LOCK_HPP_
#define ECF_PORT_LOCK_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
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
// Description : This class is used in TEST only
//               It allows functionality to create a lock file file, so that different process
//               can avoid creating server with same port number
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/noncopyable.hpp>
#include "boost/filesystem.hpp"
#include "boost/filesystem/operations.hpp"
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <iostream>
#include "File.hpp"

namespace ecf {

class EcfPortLock : private boost::noncopyable {
public:

   static bool is_free(int port)
   {
      std::string the_port = boost::lexical_cast<std::string>(port);
      if (boost::filesystem::exists(port_file(the_port))) {
   //      std::cout << "EcfPortLock::is_free returning FALSE\n ";
         return false;
      }
   //   std::cout << "EcfPortLock::is_free returning TRUE\n ";
      return true;
   }

   static void create(const std::string& the_port)
   {
      std::string the_file = port_file( the_port );
   //   std::cout << "EcfPortLock::create " << the_file << "\n";
      std::string errorMsg;
      if (!ecf::File::create(the_file,"",errorMsg)) {
         std::stringstream sb;
         sb << "EcfPortLock::create_free_port_file : could not create file " << the_file;
         throw std::runtime_error(sb.str());
      }
   }

   static void remove(const std::string& the_port)
   {
      std::string the_file = port_file(the_port);
   //   std::cout << "EcfPortLock::remove " << the_file << "\n";
      boost::filesystem::remove(the_file);
   }

private:
   EcfPortLock();

   static std::string port_file(const std::string& the_port)
   {
      // We need the *SAME* location so that different process find the same file.
      // Get to the workspace directory
      boost::filesystem::path current_path = boost::filesystem::current_path();
      //std::cout << "\ncurrent_path = " << current_path << " ------------------------------------------------------\n";
      while(current_path.stem() != "ecflow") {
         //std::cout << "current_path = " << current_path << " **********************************************\n";
         current_path = current_path.parent_path();
      }

      std::string path = current_path.string();
      path += "/ECF_PORT_used_";
      path += the_port;
      path += ".lock";
      return path;
   }
};
}
#endif
