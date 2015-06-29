//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VServerSettings.hpp"

#include <assert.h>

#include "ServerHandler.hpp"
#include "SessionHandler.hpp"
#include "SuiteFilter.hpp"
#include "UserMessage.hpp"
#include "VConfig.hpp"
#include "VConfigLoader.hpp"
#include "VProperty.hpp"
#include "VSettings.hpp"

#include <boost/property_tree/json_parser.hpp>

std::map<VServerSettings::Param,std::string> VServerSettings::parNames_;
VProperty* VServerSettings::globalProp_=0;

VServerSettings::VServerSettings(ServerHandler* server) :
	server_(server),
	prop_(NULL),
	guiProp_(NULL)
{
	if(parNames_.empty())
	{
		parNames_[UpdateRate]="server.update.updateRateInSec";
		parNames_[AdaptiveUpdate]="server.update.adaptiveUpdate";
		parNames_[MaxAdaptiveUpdateRate]="server.update.maxAdaptiveUpdateRateInMin";

		parNames_[MaxJobFileLines]="server.files.maxJobFileLines";
		parNames_[ReadFromDisk]="server.files.readFilesFromDisk";

		parNames_[AbortedPopup]="server.popup.aborted";
	}

	assert(globalProp_);

	prop_=globalProp_->clone(false,true);

	for(std::map<Param,std::string>::const_iterator it=parNames_.begin(); it != parNames_.end(); it++)
	{
		if(VProperty* p=prop_->find(it->second))
		{
			parToProp_[it->first]=p;
			propToPar_[p]=it->first;
			p->addObserver(this);
		}
		else
		{
			UserMessage::message(UserMessage::DBG, false, std::string("VServerSettings - could not find property: ") + it->second);
			assert(0);
		}
	}

	guiProp_=VConfig::instance()->cloneServerGui(prop_);
	assert(guiProp_);
}

VServerSettings::~VServerSettings()
{
	delete prop_;

	//for(std::map<Param,VProperty*>::iterator it=parToProp_.begin(); it != parToProp_.end(); it++)
	//{
	//	it->second->removeObserver(this);
	//}
}


int  VServerSettings::intValue(Param par) const
{
	return property(par)->value().toInt();
}

bool  VServerSettings::boolValue(Param par) const
{
	return property(par)->value().toBool();
}

VProperty* VServerSettings::property(Param par) const
{
	std::map<Param,VProperty*>::const_iterator it=parToProp_.find(par);
	if(it != parToProp_.end())
	{
		return it->second;
	}
	else
	{
		assert(0);
	}

	return NULL;
}

void VServerSettings::notifyChange(VProperty* p)
{
	std::map<VProperty*,Param>::iterator it=propToPar_.find(p);
	if(it != propToPar_.end())
	{
		server_->confChanged(it->second,it->first);

	}
	else
	{
		assert(0);
	}
}

void VServerSettings::loadSettings()
{
	SessionItem* cs=SessionHandler::instance()->current();
	std::string fName=cs->serverFile(server_->name());

	//Load settings stored in VProperty
	VConfig::instance()->loadSettings(fName,guiProp_);

	//Some  settings are read through VSettings
	VSettings vs(fName);
	vs.beginGroup("server_filter");
	server_->suiteFilter()->readSettings(&vs);
	vs.endGroup();
}

void VServerSettings::saveSettings()
{
	SessionItem* cs=SessionHandler::instance()->current();
	std::string fName=cs->serverFile(server_->name());

	//We save the sitefilter through VSettings
	VSettings vs("");
	vs.beginGroup("server_filter");
	server_->suiteFilter()->writeSettings(&vs);
	vs.endGroup();

	VConfig::instance()->saveSettings(fName,guiProp_,&vs);
}

//Called from VConfigLoader
void VServerSettings::load(VProperty* p)
{
	globalProp_=p;
}

static SimpleLoader<VServerSettings> loader("server");
