//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include "VInfo.hpp"

//Class to interpret generic commands and send them
// 1. to ServerHandler if they are "ecflow_client" commands
// 2. to ShellCommand if they are shell commands

class CommandHandler
{
public:
    static void run(std::vector<VInfo_ptr>,const std::string&);
    static void run(VInfo_ptr,const std::vector<std::string>&);
    static void run(VInfo_ptr,const std::string&);

protected:
    static std::string commandToString(const std::vector<std::string>& cmd);
    static void substituteVariables(std::string& cmd,const std::vector<VInfo_ptr>& info);
};

#endif // COMMANDHANDLER_HPP
