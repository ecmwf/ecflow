//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ViewNodeInfo.hpp"

#include "ServerHandler.hpp"

ViewNodeInfo::ViewNodeInfo() : node_(NULL),server_(NULL)
{
}

ViewNodeInfo::ViewNodeInfo(Node *node,ServerHandler* server) : node_(node), server_(server)
{
}

ViewNodeInfo::ViewNodeInfo(ServerHandler* server) : node_(NULL),server_(server)
{

}

bool ViewNodeInfo::isNode() const
{
	return (node_ != NULL);
}

bool ViewNodeInfo::isServer() const
{
	return (node_ == NULL && server_ != NULL);
}

bool ViewNodeInfo::isEmpty() const
{
	return (node_ == NULL && server_ == NULL);
}

Node* ViewNodeInfo::node() const
{
	return node_;
}

ServerHandler* ViewNodeInfo::server() const
{
	if(server_ == NULL && node_)
	{
		server_=ServerHandler::find(node_);
	}
	return server_;
}
