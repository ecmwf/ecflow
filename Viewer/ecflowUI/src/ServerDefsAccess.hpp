//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SERVERDEFSACCESS_HPP_
#define SERVERDEFSACCESS_HPP_

#include "Defs.hpp"

class ServerHandler;

// -------------------------------------------------------------------------
// ServerDefsAccess - a class to manage access to the server definition tree
// - required for multi-threaded access
// -------------------------------------------------------------------------

class ServerDefsAccess
{
public:
	explicit ServerDefsAccess(ServerHandler *server);
	~ServerDefsAccess();

	defs_ptr defs();

private:
	ServerHandler *server_;
};

#endif
