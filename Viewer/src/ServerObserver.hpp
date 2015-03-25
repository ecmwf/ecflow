//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SERVEROBSERVER_HPP_
#define SERVEROBSERVER_HPP_

#include "Aspect.hpp"

class ServerHandler;

class ServerObserver
{
public:
	ServerObserver() {};
	virtual ~ServerObserver() {};
	virtual void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a)=0;
};


#endif
