#ifndef CTS_CMD_REGISTRY_HPP_
#define CTS_CMD_REGISTRY_HPP_
//============================================================================
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
// Description : Registration of all the client to server commands
//               This allows us to parse the arg associated with
//               commands in this category. The idea being to keep
//               new commands functionality in one place.
//               Any new client to server commands that are created
//               must be added to this class.
//============================================================================

#include <boost/noncopyable.hpp>
#include <boost/program_options.hpp>
#include <vector>
#include "Cmd.hpp"

class AbstractClientEnv;

class CtsCmdRegistry : private boost::noncopyable  {
public:
	CtsCmdRegistry(bool addGroupCmd = true );

	/// These option describe the arguments for each of the commands
	/// They also can be presented to the user via --help option.
	void addAllOptions(boost::program_options::options_description& desc) const;
	void addCmdOptions(boost::program_options::options_description& desc) const;

	/// Parse arguments given in 'vm' and use that to create a command
	/// that will be sent to the server. Will throw std::runtime_error for errors
	/// Returns true if command line argument specified via 'vm', matches one of the
	/// registered command.
	/// *** This allows us to distinguish between where we match with a registered
	/// *** command, but do *NOT* set Cmd_ptr, ie since its a client specific command
	/// *** i.e there is no need to send it to the server
	bool parse( Cmd_ptr& cmd,
				   boost::program_options::variables_map& vm,
				   AbstractClientEnv* clientEnv ) const;

private:
	std::vector<Cmd_ptr > vec_;

	void addHelpOption(boost::program_options::options_description& desc) const;
};

#endif
