//============================================================================
// Copyright 2014 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
//============================================================================


#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

#include <iostream>

#include <QMessageBox>
#include <QMenu>
#include <QDebug>

#include "MenuHandler.hpp"
#include "ServerHandler.hpp"
#include "UserMessage.hpp"


std::vector<Menu> MenuHandler::menus_;


MenuHandler::MenuHandler()
{
    menus_.clear();
}


// ---------------------------------------------------------
// MenuHandler::readMenuConfigFile
// Read the given config file and store the resulting menus
// internally.
// ---------------------------------------------------------

bool MenuHandler::readMenuConfigFile(const std::string &configFile)
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
		return false;
    }



    // iterate over the top level of the tree

	for (ptree::const_iterator itTopLevel = pt.begin(); itTopLevel != pt.end(); ++itTopLevel)
    {
        // parse the menu definitions?

        if (itTopLevel->first == "menus")
        {
            UserMessage::message(UserMessage::DBG, false, std::string("Menus:"));

            ptree const &menusDef = itTopLevel->second;

            // iterate through all the menus

		    for (ptree::const_iterator itMenus = menusDef.begin(); itMenus != menusDef.end(); ++itMenus)
            {
                ptree const &menuDef = itMenus->second;

                std::string cname = menuDef.get("name", "NoName");
                UserMessage::message(UserMessage::DBG, false, std::string("  ") + cname);
                Menu menu(cname);

                //ptree const &menuModesDef = menuDef.get_child("modes");

		        //for (ptree::const_iterator itMenuModes = menuModesDef.begin(); itMenuModes != menuModesDef.end(); ++itMenuModes)
                //{
                //    std::cout << "   +" << itMenuModes->second.data() << std::endl;
                //}

                addMenu(menu);  // add to our list of available menus

            }
        }

        // parse the menu items?

        else if (itTopLevel->first == "menu_items")
        {
            UserMessage::message(UserMessage::DBG, false, std::string("Menu items:"));

            ptree const &itemsDef = itTopLevel->second;

            // iterate through all the items

		    for (ptree::const_iterator itItems = itemsDef.begin(); itItems != itemsDef.end(); ++itItems)
            {
                ptree const &ItemDef = itItems->second;

                std::string name     = ItemDef.get("name",    "NoName");
                std::string menuName = ItemDef.get("menu",    "NoMenu");
                std::string command  = ItemDef.get("command", "NoCommand");
                //std::cout << "  " << name << " :" << menuName << std::endl;

                MenuItem *item = new MenuItem(name);
                item->setCommand(command);
                addItemToMenu(item, menuName);
                //std::cout << "   added" << std::endl;

                // tell the ServerHandler how to translate from the item name to an actual command
                ServerHandler::addServerCommand(name, command);
            }
        }
    }


    //ptree ptMenus = pt.get_child("menus");




	//for (ptree::const_iterator itTopLevel = pt.begin(); itTopLevel != pt.end(); ++itTopLevel)
    //{
    //    if (itTopLevel->first == "menus")

    //}




    return true;
}



Menu *MenuHandler::findMenu(const std::string &name)
{
    for (std::vector<Menu>::iterator itMenus = menus_.begin(); itMenus != menus_.end(); ++itMenus)
    {
        if ((*itMenus).name() == name)
        {
            return &(*itMenus);
        }
    }

    return NULL; // if we got to here, then the menu was not found
}


MenuItem* MenuHandler::newItem(const std::string &name)
{
}

bool MenuHandler::addItemToMenu(MenuItem *item, const std::string &menuName)
{
    Menu *menu = findMenu(menuName);
    // items_.push_back(item); // add to our global list of menu items
    
    if (menu)
    {
        menu->addItem(item);
    }
    else
    {
        UserMessage::message(UserMessage::ERROR, false, std::string("Could not find menu called " + 
                             menuName + " to add item " + item->name() + " to."));
        return false;
    }
}


QAction *MenuHandler::invokeMenu(const std::string &menuName, std::vector<ViewNodeInfo_ptr> nodes, QPoint pos, QWidget *parent)
{
    QAction *selectedAction = NULL;
    Menu *menu = findMenu(menuName);

    if (menu)
    {
        QMenu *qMenu = menu->generateMenu(nodes, parent);

        selectedAction = qMenu->exec(pos);

        delete qMenu;
    }

    return selectedAction;
}



// --------------------
// Menu class functions
// --------------------

Menu::Menu(const std::string &name) : name_(name)
{
}


Menu::~Menu()
{
    for (std::vector<MenuItem*>::iterator itItems = items_.begin(); itItems != items_.end(); ++itItems)
    {
        if (*itItems)
            delete (*itItems);
    }
}


QMenu *Menu::generateMenu(std::vector<ViewNodeInfo_ptr> nodes, QWidget *parent)
{
    QMenu *qmenu=new QMenu(parent);	


    for (std::vector<MenuItem*>::iterator itItems = items_.begin(); itItems != items_.end(); ++itItems)
    {
        // XXX  is this item valid for the current selection?
        // XXX code to be written later

        if (true)
        {
            QAction *action = (*itItems)->action();
            action->setParent(parent);
            qmenu->addAction(action);
            //qDebug() << "action: " << action->iconText();
        }
    }


    return qmenu;
}


// ------------------------
// MenuItem class functions
// ------------------------

MenuItem::MenuItem(const std::string &name) : name_(name), action_(0)
{
    action_ = new QAction(0);
    action_->setText(QString(name.c_str()));
}


MenuItem::~MenuItem()
{
    if (action_)
        delete action_;
}
