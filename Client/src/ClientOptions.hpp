#ifndef CLIENTOPTIONS_HPP_
#define CLIENTOPTIONS_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : ClientOptions
// Author      : Avi
// Revision    : $Revision$ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Will parse the client argument line, and construct a command
//               that will be sent to the server.
//
// The environment must be read in before the program options. The program options
// will construct the commands, some of which require the environment
// We could have just done this as last part of constructor. However we need a
// separation between reading the environment and reading the option for:
// a/ testing purposes. i.e as this allows us to inject/override the task path
//    read in from the environment.
// b/ override host and port number.
// will throw std::runtime_error for invalid arguments
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/noncopyable.hpp>
#include "CtsCmdRegistry.hpp"
class ClientEnvironment;

class ClientOptions : private boost::noncopyable {
public:
   /// Will create command register, & ask each cmd to describe their arguments
   ClientOptions();
   ~ClientOptions();

	/// parse the arguments and create the client request that is to be sent
   /// to the server. Will throw std::runtime_error if invalid arguments specified
	Cmd_ptr parse(int argc, char* argv[], ClientEnvironment*) const;

private:

	void show_help(const std::string& help_cmd) const;
   void show_all_commands(const char* title) const;
   void show_cmd_summary(const char* title, const std::string& user_or_child = "") const;

  	CtsCmdRegistry cmdRegistry_;
  	boost::program_options::options_description* desc_;
};
#endif
