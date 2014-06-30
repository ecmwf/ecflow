#ifndef MENUHANDLER_HPP_
#define MENUHANDLER_HPP_

//============================================================================
// Copyright 2014 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//============================================================================

#include <QString>
#include <QAction>


#include "DState.hpp"

#include "ViewNodeInfo.hpp"


class QMenu;
class Node;


// -------------------------
// MenuItem
// A single item in a menu
// -------------------------

class MenuItem
{
public:
    MenuItem(const std::string &name);
    ~MenuItem();
    

    enum NodeType {SERVER, SUITE, FAMILY, TASK, ALIAS};


    void setCommand(const std::string &command) {command_ = command;};
    bool compatibleWithNode(ViewNodeInfo_ptr nodeInfo);
    void addValidType(std::string type);
    void addValidState(std::string type);
    void setAsSubMenu() {isSubMenu_ = true;};
    bool isSubMenu()    {return isSubMenu_;};
    std::string &name()   {return name_;};
    QAction     *action() {return action_;};



private:
    std::string name_;
    std::string tooltip_;
    std::string command_;
    std::string question_;
    std::string defaultAnswer_;

    std::vector<NodeType>      validNodeTypes_;
    std::vector<DState::State> validNodeStates_;
    
    bool isSubMenu_;

    QAction *action_;
};



// -------------------------------------------------------------
// Menu
// Contains all the possible items for a given menu. These will
// be filtered at run-time according to the state of
// the given item which has been clicked.
// -------------------------------------------------------------

class Menu
{
public:
    Menu(const std::string &name);
    ~Menu();
    QString exec(std::vector<Node *> nodes);
    std::string &name()       {return name_;};
    void addItem(MenuItem *item) {items_.push_back(item);};
    QMenu *generateMenu(std::vector<ViewNodeInfo_ptr> nodes, QWidget *parent);


private:
    std::string             name_;
    std::vector<MenuItem *> items_;

};


// --------------------------------------------------------------
// MenuHandler
// Responsible for creating menus (read from configuration files)
// and generating 'actual' (i.e. context-dependent filtered) 
// menus at run-time.
// --------------------------------------------------------------

class MenuHandler
{
public:
    MenuHandler();

    //Menu *createMenu(QString &name);
    static bool readMenuConfigFile(const std::string &configFile);
    static QAction *invokeMenu(const std::string &menuName, std::vector<ViewNodeInfo_ptr> nodes, QPoint pos, QWidget *parent);
    static bool addItemToMenu(MenuItem *item, const std::string &menuName);
    static Menu *findMenu(const std::string &name);
    static MenuItem* newItem(const std::string &name);

private:
    static std::vector<Menu> menus_;
    //static std::vector<MenuItem> items_;
    static void addMenu(Menu &menu) {menus_.push_back(menu);};

};


#endif 
