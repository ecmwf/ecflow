//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VConfig.hpp"
#include "VConfigLoader.hpp"
#include "VProperty.hpp"
#include "VSettings.hpp"

#include "DirectoryHandler.hpp"
#include "SessionHandler.hpp"
#include "UserMessage.hpp"

#include "Version.hpp"

#include <QDebug>

#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

VConfig* VConfig::instance_=0;

VConfig::VConfig()
{
	appName_="ecFlowUI";
	appLongName_=appName_ + " (" + ecf::Version::raw() + ")";
}

VConfig::~VConfig()
{
    for(std::vector<VProperty*>::iterator it=groups_.begin(); it != groups_.end(); ++it)
    {
        delete *it;
    }
    groups_.clear();
}


VConfig* VConfig::instance() 
{
    if(!instance_)
        instance_=new VConfig();
    
    return instance_;
}    

void VConfig::init(const std::string& parDirPath)
{
   namespace fs=boost::filesystem;
   
   fs::path parDir(parDirPath);
    
   if(fs::exists(parDir) && fs::is_directory(parDir))
   {
       //fs::directory_iterator it(parDir);
       
       //The conf files have to be loaded in alphabetical order!! At least NotifyChange require it!
       //So we read the paths into a vector and sort it.
       std::vector<fs::path> vec; 
       copy(fs::directory_iterator(parDir), fs::directory_iterator(), back_inserter(vec));
       std::sort(vec.begin(), vec.end()); 
       
       //The paths are now in alphabetical order
       for(std::vector<fs::path>::const_iterator it=vec.begin(); it != vec.end(); ++it) 
       {
            if(fs::is_regular_file(*it))
            {
                std::string name=it->filename().string();
                if(name.find("_conf.json") != std::string::npos)
                {    
                    loadInit(it->string());
                }    
            }
        }
    } 

   //Read gui definition for the editable properties
   std::string guiFile=DirectoryHandler::concatenate(parDir.string(),"ecflowview_gui.json");
   loadInit(guiFile);

   //Read gui definition for the editable properties tahat can be cutomised per server
   std::string guiServerFile=DirectoryHandler::concatenate(parDir.string(),"ecflowview_gui_server.json");
   loadInit(guiServerFile);

   //Load existing user settings for the editable properties
   loadSettings();

}

void VConfig::loadInit(const std::string& parFile)
{
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
         UserMessage::message(UserMessage::ERROR, true,
                 std::string("Error! VConfig::load() unable to parse definition file: " + parFile + " Message: " +errorMessage));
         return;
    }
    
    //Loop over the groups
    for(ptree::const_iterator itGr = pt.begin(); itGr != pt.end(); ++itGr)
    {
        ptree ptGr=itGr->second;

        //Get the group name and create it
        std::string groupName=itGr->first;
        VProperty *grProp=new VProperty(groupName);
        groups_.push_back(grProp);

        UserMessage::message(UserMessage::DBG,false,std::string("VConfig::loadInit() read config group: ") + groupName);

        //Load the property parameters. It will recursively add all the
        //children properties.
        loadProperty(ptGr,grProp);

        //Add the group we created to the registered configloader
        VConfigLoader::process(groupName,grProp);
     }
 }

void VConfig::loadProperty(const boost::property_tree::ptree& pt,VProperty *prop)
{
    using boost::property_tree::ptree;

    ptree::const_assoc_iterator itProp;

    //Loop over the possible properties
    for(ptree::const_iterator it = pt.begin(); it != pt.end(); ++it)
    {
    	std::string name=it->first;
    	ptree ptProp=it->second;

    	UserMessage::message(UserMessage::DBG,false,"   VConfig::loadProperty() read item: " + name);

    	//Default value
    	if(name == "default")
    	{
    		std::string val=ptProp.get_value<std::string>();
    		prop->setDefaultValue(val);
    	}

    	//If it is just a key/value pair "line"
    	else if(name == "line" && ptProp.empty())
    	{
    		VProperty *chProp=new VProperty(name);
    		prop->addChild(chProp);
    		std::string val=ptProp.get_value<std::string>();

    		QString prefix=prop->param("prefix");
    		if(!prefix.isEmpty())
    			val=prefix.toStdString() + "." + val;

    		UserMessage::message(UserMessage::DBG,false,"   VConfig::loadProperty() line: " + val);
            
    		if(VProperty* lineEditProp=find(val))
    		{
    			UserMessage::message(UserMessage::DBG,false,"     --> link found");
                chProp->setLink(lineEditProp);
    		}
    		else
    		{
    			UserMessage::message(UserMessage::DBG,false,"     --> link NOT found");
    		}
    	}
    	//If the property is a "line" (i.e. a line with additional parameters)
    	else if(prop->name() == "line" && name ==  "link")
    	{
    		std::string val=ptProp.get_value<std::string>();

    		UserMessage::message(UserMessage::DBG,false,"   VConfig::loadProperty() line link: " + val);

    		if(VProperty* lineEditProp=find(val))
    		{
    			UserMessage::message(UserMessage::DBG,false,"     --> link found");
    			   prop->setLink(lineEditProp);
    		}
    		else
    		{
    			   UserMessage::message(UserMessage::DBG,false,"     --> link NOT found");
    		}
    	}


        //Here we only load the properties with
        //children (i.e. key/value pairs (like "line" etc above)
        //are ignored.
    	else if(!ptProp.empty())
        {
            VProperty *chProp=new VProperty(name);
            prop->addChild(chProp);
            loadProperty(ptProp,chProp);
            chProp->adjustAfterLoad();
        }
        else
        {
        	QString val=QString::fromStdString(ptProp.get_value<std::string>());
        	prop->setParam(QString::fromStdString(name),val);
        }
    }
}

VProperty* VConfig::find(const std::string& path)
{
	VProperty* res=0;

	for(std::vector<VProperty*>::const_iterator it=groups_.begin();it != groups_.end(); ++it)
	{
	    VProperty *vGroup=*it;
	    res=vGroup->find(path);
	    if(res)
	    {
	    	return res;
	    }
	}

	return res;
}

VProperty* VConfig::group(const std::string& name)
{
	for(std::vector<VProperty*>::const_iterator it=groups_.begin();it != groups_.end(); ++it)
	{
		if((*it)->strName() == name)
			return *it;
	}

	return 0;
}

VProperty* VConfig::cloneServerGui(VProperty *linkTarget)
{
	VProperty* gr=find("gui_server.server");

	assert(gr);

	VProperty* cGr=gr->clone(true,false);

	std::vector<VProperty*> chVec;
	cGr->collectChildren(chVec);
	for(std::vector<VProperty*>::iterator it=chVec.begin(); it != chVec.end(); ++it)
	{
		VProperty *p=*it;
		if(p->link())
		{
			p->setLink(linkTarget->find(p->link()->path()));
		}
	}

	return cGr;
}

//Saves the global settings that can be edited through the gui
void VConfig::saveSettings()
{
	SessionItem* cs=SessionHandler::instance()->current();
	std::string fName=cs->settingsFile();

	VProperty *guiProp=group("gui");

	saveSettings(fName,guiProp,NULL,true);
}

//Saves the settings per server that can be edited through the servers option gui
void VConfig::saveSettings(const std::string& parFile,VProperty* guiProp,VSettings* vs,bool global)
{
	using boost::property_tree::ptree;
	ptree pt;

	//Get editable properties. We will operate on the links.
	std::vector<VProperty*> linkVec;
	guiProp->collectLinks(linkVec);

	for(std::vector<VProperty*>::const_iterator it=linkVec.begin(); it != linkVec.end(); ++it)
	{
		if(global)
		{
			if((*it)->changed())
			{
				pt.put((*it)->path(),(*it)->valueAsString());
			}
		}

		else
		{
			if(!(*it)->useMaster())
			{
				pt.put((*it)->path(),(*it)->valueAsString());
			}
		}
	}

	//Add settings stored in VSettings
	if(vs)
	{
		//Loop over the possible properties
		for(ptree::const_iterator it = vs->propertyTree().begin(); it != vs->propertyTree().end(); ++it)
		{
			pt.add_child(it->first,it->second);
		}
	}

	write_json(parFile,pt);
}

//Loads the global settings that can be edited through the gui
void VConfig::loadSettings()
{
	SessionItem* cs=SessionHandler::instance()->current();
	std::string parFile=cs->settingsFile();

	VProperty *guiProp=group("gui");

	loadSettings(parFile,guiProp,true);
}

//Loads the settings per server that can be edited through the servers option gui
void VConfig::loadSettings(const std::string& parFile,VProperty* guiProp,bool global)
{
	//We will operate on the links
	std::vector<VProperty*> linkVec;
	guiProp->collectLinks(linkVec);

	//Parse file using the boost JSON property tree parser
	using boost::property_tree::ptree;
	ptree pt;

	try
	{
	    read_json(parFile,pt);
	}
	catch (const boost::property_tree::json_parser::json_parser_error& e)
	{
		if(boost::filesystem::exists(parFile))
		{
			std::string errorMessage = e.what();
			UserMessage::message(UserMessage::ERROR, true,
	                std::string("Error! VConfig::loadSettings() unable to parse settings file: " + parFile + " Message: " +errorMessage));
		}
	    return;
    }

	for(std::vector<VProperty*>::const_iterator it=linkVec.begin(); it != linkVec.end(); ++it)
	{
		if(pt.get_child_optional((*it)->path()) != boost::none)
		{
			std::string val=pt.get<std::string>((*it)->path());

			if(!global)
			{
				(*it)->setUseMaster(false);
			}

			(*it)->setValue(val);
		}
	}
}

void VConfig::loadImportedSettings(const boost::property_tree::ptree& pt,VProperty* guiProp)
{
	std::vector<VProperty*> linkVec;
	guiProp->collectLinks(linkVec);

	for(std::vector<VProperty*>::const_iterator it=linkVec.begin(); it != linkVec.end(); ++it)
	{
		if(pt.get_child_optional((*it)->path()) != boost::none)
		{
			std::string val=pt.get<std::string>((*it)->path());
			(*it)->setValue(val);
		}
		else if((*it)->master())
		{
			(*it)->setUseMaster(true);
		}
	}
}

void VConfig::importSettings()
{
	boost::property_tree::ptree pt;

	std::string globalRcFile(DirectoryHandler::concatenate(DirectoryHandler::rcDir(),"user.default.options"));
	if(readRcFile(globalRcFile,pt))
	{
		VProperty* gr=VConfig::find("gui");
		loadImportedSettings(pt,gr);
		VConfig::saveSettings();
	}
}

bool VConfig::readRcFile(const std::string& rcFile,boost::property_tree::ptree& pt)
{
	std::ifstream in(rcFile.c_str());

	if(!in.good())
		return false;;

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

	return hasValue;

}
/*
void VConfig::decodeShowMask()
{

option<int> show::status32_ (globals::instance(), "show_mask32", 0);

option<int> show::status_ (globals::instance(), "show_mask",
			     (1<<show::unknown)
			     |(1<<show::suspended)
			     |(1<<show::complete)
			     |(1<<show::queued)
			     |(1<<show::submitted)
			     |(1<<show::active)
			     |(1<<show::aborted)
			     |(1<<show::time_dependant)
			     |(1<<show::late_nodes)
			   //			     |(1<<show::migrated_nodes)
			     |(1<<show::rerun_tasks)
			     |(1<<show::nodes_with_messages)
			     |(1<<show::label)
			     |(1<<show::meter)
			     |(1<<show::event)
			     |(1<<show::repeat)
			     |(1<<show::time)
			     |(1<<show::date)
			     |(1<<show::late)
			     |(1<<show::inlimit)
			     |(1<<show::limit)
			     |(1<<show::trigger)
			     //	& (~(1<<show::variable))
			     //	& (~(1<<show::genvar))
			     |(1<<show::time_icon)
			     |(1<<show::date_icon)
			     |(1<<show::late_icon)
			     |(1<<show::waiting_icon)
			     |(1<<show::rerun_icon)
			   // |(1<<show::migrated_icon)
			     |(1<<show::message_icon)
 // & (~(1<<show::defstatus_icon)) & (~(1<<show::zombie_icon))
			     );

show::show(int f) : flag_(f) {
  status_ = status_ & (~(1<<show::variable));
  status_ = status_ & (~(1<<show::genvar));
}

show::~show() {}

void show::on()
{
  if (flag_ > 31) {
    status32_ = int(status32_) | (1<<(flag_-32));
  } else {
    status_   = int(status_  ) | (1<<(flag_));
  }
}

void show::off()
{
  if (flag_ == show::all) {
    status_ = 0xFFFF ; status32_ = 0xFFFF;
    status32_ = (int) (status32_) & (~(1<<(show::none-32)));
    status32_ = (int) (status32_) & (~(1<<(show::all -32)));
  } else if (flag_ == show::none) {
    status_ = 0; status32_ = 0;
  } else if (flag_ > 31) {
    status32_ = int(status32_) & (~(1<<(flag_-32)));
  } else {
    status_   = int(status_)   & (~(1<<flag_));
  }
}

bool show::wanted()
{
  if (flag_ > 31) {
    return (int(status32_) & (1<<(flag_-32))) != 0;
  } else {
    return (int(status_  ) & (1<<flag_))    != 0;
  }

*/









