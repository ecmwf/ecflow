//============================================================================
// Copyright 2014 ECMWF.
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

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <map>

#include "Node.hpp"
#include "Submittable.hpp"
#include "UserMessage.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

std::map<std::string,VNState*> VNState::items_;
static std::map<NState::State,VNState*> stateMap_;

static VNState unSt("unknown",NState::UNKNOWN);
static VNState compSt("complete",NState::COMPLETE);
static VNState queuedSt("queued",NState::QUEUED);
static VNState abortedSt("aborted",NState::ABORTED);
static VNState submittedSt("submitted",NState::SUBMITTED);
static VNState activeSt("active",NState::ACTIVE);
static VNState suspendedSt("suspended");

VNState::VNState(const std::string& name,NState::State nstate) :
		VParam(name)

{
	items_[name]=this;
	stateMap_[nstate]=this;
}

VNState::VNState(const std::string& name) :
		VParam(name)
{
	items_[name]=this;
}

QColor VNState::colour() const
{
	return VParam::colour("colour");
}

//===============================================================
//
// Static methods
//
//===============================================================

std::vector<VParam*> VNState::filterItems()
{
	std::vector<VParam*> v;
	for(std::map<std::string,VNState*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
			v.push_back(it->second);
	}

	return v;
}

VNState* VNState::toState(Node *n)
{
	//VParam::Type type;
	if(n->isSuspended())
			return items_["suspended"];
	else
	{
		std::map<NState::State,VNState*>::const_iterator it=stateMap_.find(n->state());
		if(it != stateMap_.end())
			return it->second;
	}

	return NULL;
}

VNState* VNState::find(const std::string& name)
{
	std::map<std::string,VNState*>::const_iterator it=items_.find(name);
	if(it != items_.end())
				return it->second;

	return NULL;
}

//
//Has to be very quick!!
//

QColor VNState::toColour(Node *n)
{
	VNState *obj=VNState::toState(n);
	return (obj)?(obj->colour()):QColor();
}

QString VNState::toName(Node *n)
{
	VNState *obj=VNState::toState(n);
	return (obj)?(obj->qName()):QString();
}

void VNState::init(const std::string& parFile)
{
	//std::string parFile("/home/graphics/cgr/ecflowview_state.json");
	std::map<std::string,std::map<std::string,std::string> > vals;

	VParam::init(parFile,"nstate",vals);

	for(std::map<std::string,std::map<std::string,std::string> >::const_iterator it=vals.begin(); it != vals.end(); it++)
	{
		std::string name=it->first;
		//Assign the information we read to an existing VAttribute object
		if(VNState* obj=VNState::find(name))
				obj->addAttributes(it->second);

		//We are in trouble: the icon defined in the JSON file does not correspond to any of the VIcon objects!!!
		else
		{
			UserMessage::message(UserMessage::ERROR, true,
					std::string("Error, state defined in JSON file does not belong to any attribute objects : " + name));
		}
	}
}
