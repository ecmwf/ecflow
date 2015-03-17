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
                    loadFile(it->path().string()); 
                }    
            }
        }
    } 
    
}
void VConfig::loadFile(const std::string& parFile)
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

    //Check if is editable. If not it will not
    //appear in the editor.
    if((itProp=pt.find("editable")) != pt.not_found())
    {
        prop->setEditable((itProp->second.get_value<std::string>() == "false")?false:true);
    }

    //Label
    if((itProp=pt.find("label")) != pt.not_found())
    {
        prop->setLabelText(itProp->second.get_value<std::string>());
    }

    //Tooltip
    if((itProp=pt.find("tooltip")) != pt.not_found())
    {
        prop->setToolTip(itProp->second.get_value<std::string>());
    }

    //Default
    if((itProp=pt.find("default")) != pt.not_found())
    {
        prop->setDefaultValue(itProp->second.get_value<std::string>());
    }

    //Loop over the possible properties
    for(ptree::const_iterator it = pt.begin(); it != pt.end(); ++it)
    {
        ptree ptProp=it->second;

        //Here we only load the properties with
        //children (i.e. key/value pairs (like "label" etc above)
        //are ignored.
        if(!ptProp.empty())
        {
            VProperty *chProp=new VProperty(it->first);
            prop->addChild(chProp);
            loadProperty(ptProp,chProp);
        }
    }
}
