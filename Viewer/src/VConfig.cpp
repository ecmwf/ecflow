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

#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

VConfig* VConfig::instance_=0;

VConfig::VConfig()
{

}

VConfig::~VConfig()
{
    for(std::vector<VProperty*>::iterator it=groups_.begin(); it != groups_.end(); it++)
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
        for(fs::directory_iterator it(parDir) ; 
            it != fs::directory_iterator() ; ++it)
        {
            if(fs::is_regular_file(it->status()) )
            {
                std::string name=it->path().filename().string();
                if(name.find("_conf.json") != std::string::npos)
                {    
                    loadInit(it->path().string());
                }    
            }
        }
    } 

   //Read gui definition for the editable properties
   std::string guiFile=DirectoryHandler::concatenate(parDir.string(),"ecflowview_gui.json");
   loadInit(guiFile);

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

        UserMessage::message(UserMessage::DBG,false,std::string("Read config group: ") + groupName);

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

    	//Default value
    	if(name == "default")
    	{
    		std::string val=ptProp.get_value<std::string>();
    		prop->setDefaultValue(val);
    	}
    	else if(name == "line")
    	{
    		VProperty *chProp=new VProperty(name);
    		prop->addChild(chProp);

    		std::string val=ptProp.get_value<std::string>();

    		QString prefix=prop->param("prefix");
    		if(!prefix.isEmpty())
    			val=prefix.toStdString() + "." + val;

    		if(VProperty* lineEditProp=find(val))
    		{
    			chProp->setLink(lineEditProp);
    		}
    	}
        //Here we only load the properties with
        //children (i.e. key/value pairs (like "label" etc above)
        //are ignored.
    	else if(!ptProp.empty())
        {
            VProperty *chProp=new VProperty(name);
            prop->addChild(chProp);
            loadProperty(ptProp,chProp);
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

	for(std::vector<VProperty*>::const_iterator it=groups_.begin();it != groups_.end(); it++)
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
	for(std::vector<VProperty*>::const_iterator it=groups_.begin();it != groups_.end(); it++)
	{
		if((*it)->strName() == name)
			return *it;
	}

	return 0;
}

VProperty* VConfig::cloneServerGui(VProperty *linkTarget)
{
	VProperty* gr=find("gui.server");

	assert(gr);

	VProperty* cGr=gr->clone(true,false);

	std::vector<VProperty*> chVec;
	cGr->collectChildren(chVec);
	for(std::vector<VProperty*>::iterator it=chVec.begin(); it != chVec.end(); it++)
	{
		VProperty *p=*it;
		if(p->link())
		{
			p->setLink(linkTarget->find(p->link()->path()));
		}
	}


	return cGr;
}

void VConfig::saveSettings()
{
	SessionItem* cs=SessionHandler::instance()->current();
	std::string fName=cs->settingsFile();

	VProperty *guiProp=group("gui");

	saveSettings(fName,guiProp,NULL);
}


void VConfig::saveSettings(const std::string& parFile,VProperty* guiProp,VSettings* vs)
{
	using boost::property_tree::ptree;
	ptree pt;

	//Get editable properties
	std::vector<VProperty*> linkVec;
	guiProp->collectLinks(linkVec);

	for(std::vector<VProperty*>::const_iterator it=linkVec.begin(); it != linkVec.end(); it++)
	{
		if((*it)->changed())
			pt.put((*it)->path(),(*it)->valueAsString());
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

void VConfig::loadSettings()
{
	SessionItem* cs=SessionHandler::instance()->current();
	std::string parFile=cs->settingsFile();

	VProperty *guiProp=group("gui");

	loadSettings(parFile,guiProp);
}



void VConfig::loadSettings(const std::string& parFile,VProperty* guiProp)
{
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
		if(!DirectoryHandler::isFirstStartUp())
		{
			std::string errorMessage = e.what();
			UserMessage::message(UserMessage::ERROR, true,
	                std::string("Error! VConfig::loadSettings() unable to parse settings file: " + parFile + " Message: " +errorMessage));
		}
	    return;
    }

	for(std::vector<VProperty*>::const_iterator it=linkVec.begin(); it != linkVec.end(); it++)
	{
		if(pt.get_child_optional((*it)->path()) != boost::none)
		{
			std::string val=pt.get<std::string>((*it)->path());
			(*it)->setValue(val);
		}
	}
}
