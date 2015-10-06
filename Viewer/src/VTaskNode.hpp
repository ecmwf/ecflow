//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================
#ifndef VIEWER_SRC_VTASKNODE_HPP_
#define VIEWER_SRC_VTASKNODE_HPP_

#include "VNode.hpp"
#include "Node.hpp"

class ServerHandler;
class VServer;
class VServerSettings;

class VTaskNode : public VNode
{
	friend class ServerHandler;

public:
	explicit VTaskNode(VNode* parent,node_ptr);
	~VTaskNode();

	bool isEmpty() const { return true;}
	bool isTopLevel() const {return false;}
	bool isServer() const {return false;}
	void internalState(VNodeInternalState&);

protected:
	void check(VServerSettings* conf,const VNodeInternalState&);
	void check(VServerSettings* conf,bool);

private:
	void updatePrev(int,bool,bool,bool);
	bool isZombie() const;
	bool isLate() const;
	bool prevAborted() const;
	bool prevZombie() const;
	bool prevLate() const;

	enum FlagPos {AbortedPos=0x01,ZombiePos=0x02,LatePos=0x04};
	unsigned char prevTryNo_;
	unsigned char prevFlag_;
};


#endif /* VIEWER_SRC_VTASKNODE_HPP_ */
