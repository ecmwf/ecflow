//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VState.hpp"

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

std::vector<VState*> VState::items_;
std::map<NState::State,VState*> stateMap_;
std::map<VParam::Type,VState*> typeMap_;

static VState unSt("unknown",VParam::UnknownState,NState::UNKNOWN);
static VState compSt("complete",VParam::CompleteState,NState::COMPLETE);
static VState queuedSt("queued",VParam::QueuedState,NState::QUEUED);
static VState abortedSt("aborted",VParam::AbortedState,NState::ABORTED);
static VState submittedSt("submitted",VParam::SubmittedState,NState::SUBMITTED);
static VState activeSt("active",VParam::ActiveState,NState::ACTIVE);
static VState suspendedSt("suspended",VParam::SuspendedState);


VState::VState(QString name,VParam::Type type,NState::State nstate) :
		VParam(name,type)

{
	stateMap_[nstate]=this;
	typeMap_[type]=this;
	items_.push_back(this);
}

VState::VState(QString name,VParam::Type type) :
		VParam(name,type)
{
	//if(type == VParam::SuspendedState)

	typeMap_[type]=this;
	items_.push_back(this);
}

QColor VState::colour() const
{
	return VParam::colour("colour");
}

//===============================================================
//
// Static methods
//
//===============================================================

std::vector<VParam*> VState::filterItems()
{
	std::vector<VParam*> v;
	for(std::vector<VState*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
			if((*it)->text("server") != "1")
				v.push_back(*it);
	}

	return v;
}

VState* VState::find(Node *n)
{
	VParam::Type type;
	if(n->isSuspended())
			return typeMap_[VParam::SuspendedState];
	else
	{
		std::map<NState::State,VState*>::const_iterator it=stateMap_.find(n->state());
		if(it != stateMap_.end())
			return it->second;
	}

	return NULL;
}

VState* VState::find(VParam::Type type)
{
	std::map<VParam::Type,VState*>::const_iterator it=typeMap_.find(type);
	return (it != typeMap_.end())?(it->second):NULL;
}

VState* VState::find(const std::string& name)
{
	for(std::vector<VState*>::const_iterator it=items_.begin(); it != items_.end(); it++)
	{
		if((*it)->stdName() == name)
			return *it;
	}

	return NULL;
}

//
//Has to be very quick!!
//

QColor VState::toColour(Node *n)
{
	VState *obj=VState::find(n);
	return (obj)?(obj->colour()):QColor();
}

VParam::Type VState::toType(Node *n)
{
	VState *obj=VState::find(n);
	return (obj)?(obj->type()):VParam::UnknownState;
}

QString VState::toName(Node *n)
{
	VState *obj=VState::find(n);
	return (obj)?(obj->name()):QString();
}

void VState::init()
{
	std::string parFile("/home/graphics/cgr/ecflowview_state.json");

	//Parse param file using the boost JSON property tree parser
	using boost::property_tree::ptree;
	ptree pt;

	try
	{
		read_json(parFile,pt);
	}
	catch (const boost::property_tree::json_parser::json_parser_error& e)
	{
		 std::string errorMessage = e.what();
		 UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse JSON icons file : " + errorMessage));
		 return;
	}

	//Check if the category to read in is labelled as "icon"
	ptree::const_iterator itTop = pt.begin();
	if(itTop == pt.end() || itTop->first != "state")
	{
		return;
	}

	//Loop for each parameter
	for(ptree::const_iterator itPar = itTop->second.begin(); itPar != itTop->second.end(); ++itPar)
	{
		QString name=QString::fromStdString(itPar->first);
		ptree const &parPt = itPar->second;

		std::map<QString,QString> vals;
		for(ptree::const_iterator itV = parPt.begin(); itV != parPt.end(); ++itV)
		{
			vals[QString::fromStdString(itV->first)]=QString::fromStdString(itV->second.get_value<std::string>());
		}

		//Assign the information we read to an existing VState object
		bool objFound=false;
		for(std::vector<VState*>::const_iterator itObj=VState::items_.begin(); itObj != VState::items_.end(); itObj++)
		{
			if(name == (*itObj)->name())
			{
				objFound=true;
				(*itObj)->addAttributes(vals);
				break;
			}
		}

		//We are in trouble: the icon defined in the JSON file does not correspond to any of the VState objects!!!
		if(!objFound)
		{
			 //UserMessage::message(UserMessage::ERROR, true, std::string("Error, state defined in JSON file does not belong to any state objects : " + name.toStdString()));
		}
	}
}
