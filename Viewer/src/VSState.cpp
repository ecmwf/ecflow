//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VSState.hpp"

#include <QDebug>
#include <QImage>
#include <QImageReader>

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <map>

#include "ServerHandler.hpp"
#include "Submittable.hpp"
#include "UserMessage.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

std::map<std::string,VSState*> VSState::items_;
static std::map<SState::State,VSState*> stateMap_;


static VSState halterSt("halted",SState::HALTED);
static VSState runningSt("running",SState::RUNNING);
static VSState shutSt("shutdown",SState::SHUTDOWN);


VSState::VSState(const std::string& name,SState::State Sstate) :
		VParam(name)
{
	items_[name]=this;
	stateMap_[Sstate]=this;
}


QColor VSState::colour() const
{
	return VParam::colour("colour");
}

//===============================================================
//
// Static methods
//
//===============================================================

VSState* VSState::toState(ServerHandler* s)
{
	if(!s)
		return NULL;

	std::map<SState::State,VSState*>::const_iterator it=stateMap_.find(s->state());
	if(it != stateMap_.end())
		return it->second;

	return NULL;
}

VSState* VSState::find(const std::string& name)
{
	std::map<std::string,VSState*>::const_iterator it=items_.find(name);
	if(it != items_.end())
				return it->second;

	return NULL;
}

//
//Has to be very quick!!
//

QColor VSState::toColour(ServerHandler* s)
{
	VSState *obj=VSState::toState(s);
	return (obj)?(obj->colour()):QColor();
}

QString VSState::toName(ServerHandler* s)
{
	VSState *obj=VSState::toState(s);
	return (obj)?(obj->qName()):QString();
}

void VSState::init(const std::string& parFile)
{
	//std::string parFile("/home/graphics/cgr/ecflowview_sstate.json");
	std::map<std::string,std::map<std::string,std::string> > vals;

	VParam::init(parFile,"sstate",vals);

	for(std::map<std::string,std::map<std::string,std::string> >::const_iterator it=vals.begin(); it != vals.end(); it++)
	{
		std::string name=it->first;
		//Assign the information we read to an existing VAttribute object
		if(VSState* obj=VSState::find(name))
				obj->addAttributes(it->second);

		//We are in trouble: the icon defined in the JSON file does not correspond to any of the VIcon objects!!!
		else
		{
			UserMessage::message(UserMessage::ERROR, true,
					std::string("Error, state defined in JSON file does not belong to any attribute objects : " + name));
		}
	}
}
