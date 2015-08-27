//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "InfoPanelHandler.hpp"


#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

#include "NodeExpression.hpp"
#include "UserMessage.hpp"

InfoPanelHandler* InfoPanelHandler::instance_=0;


InfoPanelDef::InfoPanelDef(const std::string& name) :
	name_(name),
	visibleCondition_(0),
	enabledCondition_(0)
{

}

InfoPanelHandler::InfoPanelHandler()
{

}

InfoPanelHandler* InfoPanelHandler::instance()
{
	if(!instance_)
		instance_=new InfoPanelHandler();

	return instance_;
}

void InfoPanelHandler::init(const std::string &configFile)
{
	// parse the response using the boost JSON property tree parser

	using boost::property_tree::ptree;
	ptree pt;

	try
	{
		read_json(configFile, pt);
	}
	catch (const boost::property_tree::json_parser::json_parser_error& e)
	{
        std::string errorMessage = e.what();
        UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse JSON menu file : " + errorMessage));
		return;
    }


    // iterate over the top level of the tree
	for (ptree::const_iterator itTopLevel = pt.begin(); itTopLevel != pt.end(); ++itTopLevel)
    {
        if (itTopLevel->first == "info_panel")
        {
            UserMessage::message(UserMessage::DBG, false, std::string("Panels:"));

            ptree const &panelsPt = itTopLevel->second;

            // iterate through all the panels
		    for (ptree::const_iterator itPanel = panelsPt.begin(); itPanel != panelsPt.end(); ++itPanel)
            {
                ptree const &panelPt = itPanel->second;

                std::string cname = panelPt.get("name", "");

                UserMessage::message(UserMessage::DBG, false, std::string("  ") + cname);

                InfoPanelDef* def= new InfoPanelDef(cname);

                def->setLabel(panelPt.get("label",""));
                def->setIcon(panelPt.get("icon",""));
                def->setDockIcon(panelPt.get("dock_icon",""));
                def->setShow(panelPt.get("show",""));

                std::string enabled  = panelPt.get("enabled_for", "");
                std::string visible  = panelPt.get("visible_for", "");

                BaseNodeCondition *enabledCond = NodeExpressionParser::parseWholeExpression(enabled);
                if (enabledCond == NULL)
                {
                	UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse enabled condition: " + enabled));
                    enabledCond = new FalseNodeCondition();
                }
                def->setEnabledCondition(enabledCond);


                BaseNodeCondition *visibleCond = NodeExpressionParser::parseWholeExpression(visible);
                if (visibleCond == NULL)
                {
                	UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse visible condition: " + visible));
                    visibleCond = new FalseNodeCondition();
                }
                def->setVisibleCondition(visibleCond);

                panels_.push_back(def);

            }
        }
    }
}

void InfoPanelHandler::visible(VInfo_ptr info,std::vector<InfoPanelDef*>& lst)
{
	for(std::vector<InfoPanelDef*>::const_iterator it=panels_.begin(); it != panels_.end(); ++it)
	{
          if((*it)->visibleCondition()->execute(info))
        	 lst.push_back((*it));
    }
}

