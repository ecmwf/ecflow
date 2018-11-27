//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VNState.hpp"

#include <QDebug>
#include <QImage>
#include <QImageReader>

#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <map>

#include "VNode.hpp"
#include "ServerHandler.hpp"
#include "Submittable.hpp"
#include "UserMessage.hpp"
#include "VConfigLoader.hpp"
#include "VProperty.hpp"

std::map<std::string,VNState*> VNState::items_;
static std::map<NState::State,VNState*> stateMap_;
static std::map<unsigned char,VNState*> idMap_;

static VNState unSt("unknown",NState::UNKNOWN);
static VNState compSt("complete",NState::COMPLETE);
static VNState queuedSt("queued",NState::QUEUED);
static VNState abortedSt("aborted",NState::ABORTED);
static VNState submittedSt("submitted",NState::SUBMITTED);
static VNState activeSt("active",NState::ACTIVE);
static VNState suspendedSt("suspended");

static unsigned char ucIdCnt=0;

VNState::VNState(const std::string& name,NState::State nstate) :
    VParam(name),
    ucId_(ucIdCnt++)
{
	items_[name]=this;
	stateMap_[nstate]=this;
    idMap_[ucId_]=this;
}

VNState::VNState(const std::string& name) :
	VParam(name)
{
	items_[name]=this;
}

//===============================================================
//
// Static methods
//
//===============================================================

std::vector<VParam*> VNState::filterItems()
{
	std::vector<VParam*> v;
	for(std::map<std::string,VNState*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
	{
			v.push_back(it->second);
	}

	return v;
}

VNState* VNState::toState(const VNode *n)
{
	if(!n || !n->node().get())
		return nullptr;

	node_ptr node=n->node();

	if(node->isSuspended())
			return items_["suspended"];
	else
	{
		std::map<NState::State,VNState*>::const_iterator it=stateMap_.find(node->state());
		if(it != stateMap_.end())
			return it->second;
	}

	return nullptr;
}

VNState* VNState::toRealState(const VNode *n)
{
	if(!n || !n->node().get())
		return nullptr;

	node_ptr node=n->node();

	std::map<NState::State,VNState*>::const_iterator it=stateMap_.find(node->state());
	if(it != stateMap_.end())
		return it->second;

	return nullptr;
}

VNState* VNState::toDefaultState(const VNode *n)
{
    if(!n || !n->node())
		return nullptr;

	node_ptr node=n->node();

    const char *dStateName=DState::toString(node->defStatus());
    assert(dStateName);
    std::string dsn(dStateName);
    return find(dsn);
}

VNState* VNState::find(const std::string& name)
{
	std::map<std::string,VNState*>::const_iterator it=items_.find(name);
	if(it != items_.end())
				return it->second;

	return nullptr;
}

VNState* VNState::find(unsigned char ucId)
{
    std::map<unsigned char,VNState*>::const_iterator it=idMap_.find(ucId);
    if(it != idMap_.end())
        return it->second;

    return NULL;
}

//
//Has to be very quick!!
//

QColor VNState::toColour(const VNode *n)
{
	VNState *obj=VNState::toState(n);
	return (obj)?(obj->colour()):QColor();
}

QColor VNState::toRealColour(const VNode *n)
{
	VNState *obj=VNState::toRealState(n);
	return (obj)?(obj->colour()):QColor();
}

QColor VNState::toFontColour(const VNode *n)
{
	VNState *obj=VNState::toState(n);
	return (obj)?(obj->fontColour()):QColor();
}

QColor VNState::toTypeColour(const VNode *n)
{
    VNState *obj=VNState::toState(n);
    return (obj)?(obj->typeColour()):QColor();
}

QString VNState::toName(const VNode *n)
{
	VNState *obj=VNState::toState(n);
	return (obj)?(obj->name()):QString();
}

QString VNState::toDefaultStateName(const VNode *n)
{
	VNState *obj=VNState::toDefaultState(n);
	return (obj)?(obj->name()):QString();
}

QString VNState::toRealStateName(const VNode *n)
{
    VNState *obj=VNState::toRealState(n);
    return (obj)?(obj->name()):QString();
}

bool VNState::isActive(unsigned char ucId)
{
    VNState *obj=VNState::find(ucId);
    return (obj)?(obj->name() == "active"):false;
}

bool VNState::isComplete(unsigned char ucId)
{
    VNState *obj=VNState::find(ucId);
    return (obj)?(obj->name() == "complete"):false;
}

bool VNState::isSubmitted(unsigned char ucId)
{
    VNState *obj=VNState::find(ucId);
    return (obj)?(obj->name() == "submitted"):false;
}

//==================================================
// Server state
//==================================================

VNState* VNState::toState(ServerHandler *s)
{
	if(!s)
		return nullptr;

	bool susp=false;
	NState::State ns=s->state(susp);

	if(susp)
			return items_["suspended"];
	else
	{
		std::map<NState::State,VNState*>::const_iterator it=stateMap_.find(ns);
		if(it != stateMap_.end())
			return it->second;
	}

	return nullptr;
}

QString VNState::toName(ServerHandler *s)
{
	VNState *obj=VNState::toState(s);
	return (obj)?(obj->name()):QString();
}

QColor VNState::toColour(ServerHandler *s)
{
	VNState *obj=VNState::toState(s);
	return (obj)?(obj->colour()):QColor();
}

QColor VNState::toFontColour(ServerHandler *s)
{
	VNState *obj=VNState::toState(s);
	return (obj)?(obj->fontColour()):QColor();
}

void VNState::load(VProperty* group)
{
    Q_FOREACH(VProperty* p,group->children())
    {
         if(VNState* obj=VNState::find(p->strName())) 
         {
            obj->setProperty(p);
         }   
    }    
}
        
static SimpleLoader<VNState> loader("nstate");
