//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef LOGSERVER_H
#define LOGSERVER_H

#include "VFile.hpp"
#include "VDir.hpp"

class LogServer;
typedef boost::shared_ptr<LogServer> LogServer_ptr;

class LogServer
{
public:
    LogServer(std::string host,std::string port);
    ~LogServer();

    const std::string host() const {return host_;}
    const std::string port() const {return port_;}

    VFile_ptr getFile(std::string name);
    VDir_ptr  getDir(const char* name);
    bool      ok() const { return soc_ >= 0; }

private:
	LogServer(const LogServer&);
	LogServer& operator=(const LogServer&);
	void connect(std::string,int);

	int  soc_;
	std::string host_;
	std::string port_;
};

#endif
