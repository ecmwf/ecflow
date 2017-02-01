//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//============================================================================

#ifndef MENUHANDLER_HPP_
#define MENUHANDLER_HPP_

#include <vector>
#include <map>
#include <QString>
#include <QIcon>
#include <QList>
#include <QPoint>

#include "VInfo.hpp"
#include "NodeExpression.hpp"

class QMenu;
class QAction;
class QWidget;
class Node;
class BaseNodeCondition;



// -------------------------
// MenuItem
// A single item in a menu
// -------------------------

class MenuItem
{
public:
    explicit MenuItem(const std::string &name);
    ~MenuItem();

    void setCommand(const std::string &command);
    //bool compatibleWithNode(VInfo_ptr nodeInfo);
    //void addValidType(std::string type);
    //void addValidState(std::string type);
    void setHandler(const std::string &handler);
    void setViews(const std::vector<std::string> &views) {views_=views;};
    void setQuestion(const std::string &question) {question_=question;}
    void setIcon(const std::string &icon);
    void setStatustip(const std::string &statustip) {statustip_=statustip;}
    void setHidden(bool b) {hidden_=b;}
    void setAsSubMenu() {isSubMenu_ = true;};
    void setVisibleCondition(BaseNodeCondition *cond)  {visibleCondition_  = cond;};
    void setEnabledCondition(BaseNodeCondition *cond)  {enabledCondition_  = cond;};
    void setQuestionCondition(BaseNodeCondition *cond) {questionCondition_ = cond;};
    void setCustom(bool b) {isCustom_ = b;};
    BaseNodeCondition *visibleCondition()  {return visibleCondition_;};
    BaseNodeCondition *enabledCondition()  {return enabledCondition_;};
    BaseNodeCondition *questionCondition() {return questionCondition_;};
    bool shouldAskQuestion(std::vector<VInfo_ptr> &nodes);
    bool isSubMenu()      {return isSubMenu_;};
    bool isDivider()      {return isDivider_;};
    bool isCustom()       {return isCustom_;};
    std::string &name()   {return name_;};
    const std::string handler() const {return handler_;}
    bool isValidView(const std::string&) const;
    const std::string command() const {return command_;}
    const std::string question() const {return question_;}
    bool hidden() const {return hidden_;}
    int id() const {return id_;}
    QAction* createAction(QWidget* parent);

private:
    //No copy allowed
    MenuItem(const MenuItem&);
    MenuItem& operator=(const MenuItem&);

    //bool isNodeTypeValidForMenuItem(NodeType type);

    std::string name_;
    int id_;
    std::string tooltip_;
    std::string command_;
    std::string statustip_;
    std::string question_;
    std::string defaultAnswer_;
    std::string handler_;
    std::vector<std::string> views_;
    bool hidden_;

    //std::vector<NodeType>      validNodeTypes_;
    //std::vector<DState::State> validNodeStates_;


    BaseNodeCondition *visibleCondition_;
    BaseNodeCondition *enabledCondition_;
    BaseNodeCondition *questionCondition_;

    bool isSubMenu_;
    bool isDivider_;
    bool isCustom_;

    QIcon icon_;

    static int idCnt_;
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
    explicit Menu(const std::string &name);
    ~Menu();
    QString exec(std::vector<Node *> nodes);
    std::string &name()       {return name_;};
    void addItemToFixedList(MenuItem *item) {itemsFixed_.push_back(item);};
    void addItemToCustomList(MenuItem *item) {itemsCustom_.push_back(item);};
    void clearFixedList() {itemsFixed_.clear();}
    QMenu *generateMenu(std::vector<VInfo_ptr> nodes, QWidget *parent,QMenu* parentMenu,const std::string& view,QList<QAction*>&);
    std::vector<MenuItem *>& items() {return itemsCombined_;};

private:
    void buildMenuTitle(std::vector<VInfo_ptr> nodes, QMenu* qmenu);

    std::string             name_;
    std::vector<MenuItem *> itemsFixed_;
    std::vector<MenuItem *> itemsCustom_;
    std::vector<MenuItem *> itemsCombined_;  // items from config file plus custom commands

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
    static MenuItem *invokeMenu(const std::string &menuName, std::vector<VInfo_ptr> nodes, QPoint pos, QWidget *parent,const std::string& view);
    static bool addItemToMenu(MenuItem *item, const std::string &menuName);
    static Menu *findMenu(const std::string &name);    
    static MenuItem* newItem(const std::string &name);
    static void addMenu(Menu *menu) {menus_.push_back(menu);};
    static void interceptCommandsThatNeedConfirmation(MenuItem *item);
    static void refreshCustomMenuCommands();

private:
    typedef std::map<std::string, std::string> ConfirmationMap;
    static MenuItem* findItem(QAction*);
    static ConfirmationMap &getCommandsThatRequireConfirmation();

    static std::vector<Menu *> menus_;
    static ConfirmationMap commandsWhichRequireConfirmation_;
    static TrueNodeCondition trueCond_;
    static FalseNodeCondition falseCond_;
    //static std::vector<MenuItem> items_;

};


#endif 
