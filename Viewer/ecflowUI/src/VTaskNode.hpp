//============================================================================
// Copyright 2009-2019 ECMWF.
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
	~VTaskNode() override;

	bool isEmpty() const { return true;}
    //bool isNode() const {return true;}
    bool isTopLevel() const override {return false;}
    //bool isServer() const {return false;}
    VTaskNode* isTask() const override {return const_cast<VTaskNode*>(this);}
	void internalState(VNodeInternalState&) override;
    const std::string& typeName() const override;

protected:
	void check(VServerSettings* conf,const VNodeInternalState&) override;
	void check(VServerSettings* conf,bool) override;

private:
	void updatePrev(int,bool,bool,bool);
	bool isZombie() const;
	bool isLate() const;
	bool prevAborted() const;
	bool prevZombie() const;
	bool prevLate() const;

	enum FlagMask {AbortedMask=0x01,ZombieMask=0x02,LateMask=0x04};
	unsigned char prevTryNo_;
	unsigned char prevFlag_;
};


#endif /* VIEWER_SRC_VTASKNODE_HPP_ */
