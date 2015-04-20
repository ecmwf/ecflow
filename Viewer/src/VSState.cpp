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
#include "VConfigLoader.hpp"
#include "VProperty.hpp"

std::map<std::string,VSState*> VSState::items_;
static std::map<SState::State,VSState*> stateMap_;

static VSState haltedSt("halted",SState::HALTED);
static VSState runningSt("running",SState::RUNNING);
static VSState shutSt("shutdown",SState::SHUTDOWN);
static VSState disconnectedSt("disconnected");

VSState::VSState(const std::string& name,SState::State Sstate) :
		VParam(name),
		prop_(0)
{
	items_[name]=this;
	stateMap_[Sstate]=this;
}

VSState::VSState(const std::string& name) :
		VParam(name),
		prop_(0)
{
	items_[name]=this;
}

void VSState::setProperty(VProperty* prop)
{
    prop_=prop; 
    
    //get colour
    if(VProperty *p=prop_->findChild("fill_colour"))
    {
        colour_=p->value().value<QColor>();
        
        qDebug() << qName_ << colour_;
    }    
}

//===============================================================
//
// Static methods
//
//===============================================================

bool VSState::isRunningState(ServerHandler* s)
{
	return toState(s)==items_["running"];
}


VSState* VSState::toState(ServerHandler* s)
{
	if(!s)
		return NULL;

	if(!s->connected())
		return items_["disconnected"];

	std::map<SState::State,VSState*>::const_iterator it=stateMap_.find(s->serverState());
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
	return (obj)?(obj->name()):QString();
}

void VSState::load(VProperty* group)
{
    Q_FOREACH(VProperty* p,group->children())
    {
         if(VSState* obj=VSState::find(p->strName())) 
         {
            obj->setProperty(p);
         }   
    }    
}
        
static SimpleLoader<VSState> loader("sstate");

