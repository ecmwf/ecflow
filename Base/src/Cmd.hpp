#ifndef CMD_HPP_
#define CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Cmd
// Author      : Avi
// Revision    : $Revision: #84 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <memory>

class ClientToServerCmd;
class ServerToClientCmd;
class ServerReply;

typedef std::shared_ptr<ClientToServerCmd> Cmd_ptr;
typedef std::shared_ptr<ServerToClientCmd> STC_Cmd_ptr;

#endif
