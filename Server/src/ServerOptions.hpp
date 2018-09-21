#ifndef SERVER_OPTIONS_HPP_
#define SERVER_OPTIONS_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : ServerOptions
// Author      : Avi
// Revision    : $Revision: #7 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//
// This class will parse the server arguments
// It will update the ServerEnvironment
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/program_options.hpp>
class ServerEnvironment;

class ServerOptions {
private:
  ServerOptions(const ServerOptions&) = delete;
  const ServerOptions& operator=(const ServerOptions&) = delete;
public:
	ServerOptions(int argc, char* argv[], ServerEnvironment*);

	/// return true if help selected, else false
   bool help_option() const;
   bool version_option() const;

private:
 	boost::program_options::variables_map vm_;
};
#endif
