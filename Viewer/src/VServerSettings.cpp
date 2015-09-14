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

#include "DirectoryHandler.hpp"
#include "ServerHandler.hpp"
#include "ServerItem.hpp"
#include "ServerList.hpp"
#include "SessionHandler.hpp"
#include "SuiteFilter.hpp"
#include "UserMessage.hpp"
#include "VConfig.hpp"
#include "VConfigLoader.hpp"
#include "VProperty.hpp"
#include "VSettings.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>

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

		parNames_[AbortedEnabled]="server.notification.aborted.enabled";
		parNames_[AbortedPopup]="server.notification.aborted.popup";
		parNames_[AbortedSound]="server.notification.aborted.sound";

		parNames_[RestartedEnabled]="server.notification.restarted.enabled";
		parNames_[RestartedPopup]="server.notification.restarted.popup";
		parNames_[RestartedSound]="server.notification.restarted.sound";

		parNames_[LateEnabled]="server.notification.late.enabled";
		parNames_[LatePopup]="server.notification.late.popup";
		parNames_[LateSound]="server.notification.late.sound";

		parNames_[ZombieEnabled]="server.notification.zombie.enabled";
		parNames_[ZombiePopup]="server.notification.zombie.popup";
		parNames_[ZombieSound]="server.notification.zombie.sound";

		parNames_[AliasEnabled]="server.notification.alias.enabled";
		parNames_[AliasPopup]="server.notification.alias.popup";
		parNames_[AliasSound]="server.notification.alias.sound";
	}

	assert(globalProp_);

	prop_=globalProp_->clone(false,true);

	for(std::map<Param,std::string>::const_iterator it=parNames_.begin(); it != parNames_.end(); ++it)
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
	if(boost::filesystem::exists(fName))
	{
		VSettings vs(fName);
		vs.read();
		vs.beginGroup("suite_filter");
		server_->suiteFilter()->readSettings(&vs);
		vs.endGroup();
	}
}

void VServerSettings::saveSettings()
{
	SessionItem* cs=SessionHandler::instance()->current();
	std::string fName=cs->serverFile(server_->name());

	//We save the suite filter through VSettings
	VSettings vs("");
	vs.beginGroup("suite_filter");
	server_->suiteFilter()->writeSettings(&vs);
	vs.endGroup();

	VConfig::instance()->saveSettings(fName,guiProp_,&vs);
}

void VServerSettings::importRcFiles()
{
	SessionItem* cs=SessionHandler::instance()->current();

	for(int i=0; i < ServerList::instance()->count(); i++)
	{
		std::string name=ServerList::instance()->itemAt(i)->name();

		std::string rcFile(DirectoryHandler::concatenate(DirectoryHandler::rcDir(),name + ".options"));

		std::ifstream in(rcFile.c_str());

		if(!in.good())
			continue;

		using boost::property_tree::ptree;
		ptree pt;

		bool hasValue=false;

		std::string line;
		while(getline(in,line))
		{
			std::string buf;
			std::stringstream ssdata(line);
			std::vector<std::string> vec;

			while(ssdata >> buf)
			{
				vec.push_back(buf);
			}

			if(vec.size() >= 1)
			{
				std::vector<std::string> par;
				boost::split(par,vec[0],boost::is_any_of(":"));

				if(par.size()==2)
				{
					//Update
					if(par[0] == "timeout")
					{
						pt.put("server.update.updateRateInSec",par[1]);
						hasValue=true;
					}
					else if(par[0] == "poll")
					{
						pt.put("server.update.update",par[1]);
						hasValue=true;
					}

					else if(par[0] == "drift")
					{
						pt.put("server.update.adaptiveUpdate",par[1]);
						hasValue=true;
					}
					else if(par[0] == "maximum")
					{
						pt.put("server.update.maxAdaptiveUpdateRateInMin",par[1]);
						hasValue=true;
					}

					//Files
					else if(par[0] == "direct_read")
					{
						pt.put("server.files.readFilesFromDisk",par[1]);
						hasValue=true;
					}
					else if(par[0] == "jobfile_length")
					{
						pt.put("server.files.maxJobFileLines",par[1]);
						hasValue=true;
					}

					//Popup
					else if(par[0] == "aborted")
					{
						pt.put("server.notification.aborted.enabled",par[1]);
						pt.put("server.notification.aborted.popup",par[1]);
						hasValue=true;
					}
					else if(par[0] == "restarted")
					{
						pt.put("server.notification.restarted.enabled",par[1]);
						pt.put("server.notification.restarted.popup",par[1]);
						hasValue=true;
					}
					else if(par[0] == "late")
					{
						pt.put("server.notification.late.enabled",par[1]);
						pt.put("server.notification.late.popup",par[1]);
						hasValue=true;
					}
					else if(par[0] == "zombies")
					{
						pt.put("server.notification.zombie.enabled",par[1]);
						pt.put("server.notification.zombie.popup",par[1]);
						hasValue=true;
					}
					else if(par[0] == "aliases")
					{
						pt.put("server.notification.alias.enabled",par[1]);
						pt.put("server.notification.alias.popup",par[1]);
						hasValue=true;
					}
					//Suites
					else if(par[0] == "new_suites")
					{
						pt.put("suite_filter.autoAddNew",par[1]);
						hasValue=true;

					}
					else if(par[0] == "suites")
					{
						boost::property_tree::ptree suites;
						suites.push_back(std::make_pair("",par[1]));

						for(unsigned int j=1; j < vec.size(); j++)
						{
							suites.push_back(std::make_pair("",vec.at(j)));
						}

						pt.put_child("suite_filter.suites",suites);

						pt.put("suite_filter.enabled","true");

						hasValue=true;

					}
				}
			}

		} //while(getline)

		in.close();

		if(hasValue)
		{
			std::string jsonName=cs->serverFile(name);
			write_json(jsonName,pt);
		}

	}
}


//Called from VConfigLoader
void VServerSettings::load(VProperty* p)
{
	globalProp_=p;
}

static SimpleLoader<VServerSettings> loader("server");
