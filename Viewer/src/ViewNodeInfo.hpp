//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWNODEINFO_HPP_
#define VIEWNODEINFO_HPP_

#include <cstddef>
#include <boost/shared_ptr.hpp>

class Node;

class ServerHandler;

typedef bool (*HasProc)(Node*);

class ViewNodeInfo
{
public:
		ViewNodeInfo();
		ViewNodeInfo(Node*,ServerHandler* server=NULL);
		ViewNodeInfo(ServerHandler*);

		bool isNode() const;
		bool isServer() const;
		bool isEmpty() const;
		Node* node() const;
		ServerHandler* server() const;

private:
		Node* node_;
		mutable ServerHandler* server_;
};

typedef boost::shared_ptr<ViewNodeInfo>   ViewNodeInfo_ptr;

#endif

