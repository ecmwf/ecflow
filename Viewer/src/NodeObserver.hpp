//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef NODEOBSERVER_HPP_
#define NODEOBSERVER_HPP_

#include "Aspect.hpp"
#include "Node.hpp"

class VNode;
class VNodeChange;

class NodeObserver
{
public:
	NodeObserver() {};
	virtual ~NodeObserver() {};
	virtual void notifyNodeChanged(const VNode* vn, const std::vector<ecf::Aspect::Type>& a,const VNodeChange&)=0;
};


#endif
