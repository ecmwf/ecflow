//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VTaskNode.hpp"

#include "ServerDefsAccess.hpp"
#include "ServerHandler.hpp"
#include "VNState.hpp"

#include <boost/algorithm/string.hpp>
#include "ChangeNotify.hpp"

VTaskNode::VTaskNode(VNode* parent,node_ptr node) :
	VNode(parent,node),
	prevTryNo_(0),
	prevFlag_(0)
{
	unsigned int tn=tryNo();
	NState::State new_status = node_->state();

	bool aborted=(new_status == NState::ABORTED);
	bool zombie=node_->flag().is_set(ecf::Flag::ZOMBIE);
	bool late=node_->flag().is_set(ecf::Flag::LATE);

	updatePrev(tn,aborted,zombie,late);
}

VTaskNode::~VTaskNode()
= default;

void VTaskNode::internalState(VNodeInternalState& st)
{
	st.tryNo_=prevTryNo_;
	st.flag_=prevFlag_;
}

void VTaskNode::updatePrev(int tn,bool aborted,bool zombie,bool late)
{
	prevTryNo_=tn;

	if(aborted)
		prevFlag_ |= AbortedMask; //(0x01 << AbortedPos);
	else
		prevFlag_ &= ~AbortedMask; //0x01 << AbortedPos);

	if(zombie)
		prevFlag_ |= ZombieMask;
	else
		prevFlag_ &= ~ZombieMask;

	if(late)
		prevFlag_ |= LateMask;
	else
		prevFlag_ &= ~LateMask;
}

bool VTaskNode::isZombie() const
{
	if(node_)
		return node_->flag().is_set(ecf::Flag::ZOMBIE);
	return false;
}

bool VTaskNode::isLate() const
{
	if(node_)
		return node_->flag().is_set(ecf::Flag::LATE);
	return false;
}

bool VTaskNode::prevAborted() const
{
	return prevFlag_ & AbortedMask;
}

bool VTaskNode::prevZombie() const
{
	return prevFlag_ & ZombieMask;
}

bool VTaskNode::prevLate() const
{
	return prevFlag_ & LateMask;
}

void VTaskNode::check(VServerSettings* conf,const VNodeInternalState& st)
{
	//if(st)
	//{
		prevTryNo_=st.tryNo_;
		prevFlag_=st.flag_;

		check(conf,true);
	//}
}

void VTaskNode::check(VServerSettings* conf,bool stateChange)
{
	NState::State new_status = node_->state();

	bool new_aborted=(new_status == NState::ABORTED);
	unsigned int new_tryNo=tryNo();
	bool new_zombie=isZombie();
	bool new_late=isLate();

	//Aborted
	if(stateChange)
	{
		if(conf->boolValue(VServerSettings::NotifyAbortedEnabled))
		{
			bool prev_aborted=prevAborted();

			//Check for aborted
			if(new_aborted && !prev_aborted)
			{
				bool popup=conf->boolValue(VServerSettings::NotifyAbortedPopup);
				bool sound=conf->boolValue(VServerSettings::NotifyAbortedSound);
				ChangeNotify::add("aborted",this,popup,sound);
			}
			else if(!new_aborted && prev_aborted)
			{
				ChangeNotify::remove("aborted",this);
			}
		}
	}

	//Restarted
	if(conf->boolValue(VServerSettings::NotifyRestartedEnabled))
	{
		if(new_status == NState::SUBMITTED || new_status == NState::ACTIVE)
		{
			if(new_tryNo > 1 && new_tryNo != prevTryNo_)
			{
				bool popup=conf->boolValue(VServerSettings::NotifyRestartedPopup);
				bool sound=conf->boolValue(VServerSettings::NotifyRestartedSound);
				ChangeNotify::add("restarted",this,popup,sound);
			}
		}
	}

	//Zombie
	if(conf->boolValue(VServerSettings::NotifyZombieEnabled))
	{
		if(node_)
		{
			if(new_zombie && !prevZombie())
			{
				bool popup=conf->boolValue(VServerSettings::NotifyZombiePopup);
				bool sound=conf->boolValue(VServerSettings::NotifyZombieSound);
				ChangeNotify::add("zombie",this,popup,sound);
			}
		}
	}

	//Late
	if(conf->boolValue(VServerSettings::NotifyLateEnabled))
	{
		if(node_)
		{
			if(new_late && !prevLate())
			{
				bool popup=conf->boolValue(VServerSettings::NotifyLatePopup);
				bool sound=conf->boolValue(VServerSettings::NotifyLateSound);
				ChangeNotify::add("late",this,popup,sound);
			}
		}
	}

	updatePrev(new_tryNo,new_aborted,new_zombie,new_late);
}

const std::string& VTaskNode::typeName() const
{
   static std::string t("task");
   return t;
}
