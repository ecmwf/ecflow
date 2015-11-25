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
#include <QLabel>
#include <QLinearGradient>
#include <QWidgetAction>
#include <QDebug>
#include <QObject>
#include <QVBoxLayout>

#include "Str.hpp"
#include "MenuHandler.hpp"
#include "ServerHandler.hpp"
#include "UserMessage.hpp"
#include "NodeExpression.hpp"
#include "VConfig.hpp"

std::vector<Menu *> MenuHandler::menus_;

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
                Menu *menu = new Menu(cname);

                //ptree const &menuModesDef = menuDef.get_child("modes");

		        //for (ptree::const_iterator itMenuModes = menuModesDef.begin(); itMenuModes != menuModesDef.end(); ++itMenuModes)
                //{
                //    std::cout << "   +" << itMenuModes->second.data() << std::endl;
                //}

                std::string parentMenuName = menuDef.get("parent", "None");

                if (parentMenuName != "None")
                {
                }

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

                std::string name     = ItemDef.get("name",        "NoName");
                std::string menuName = ItemDef.get("menu",        "NoMenu");
                std::string command  = ItemDef.get("command",     "NoCommand");
                std::string type     = ItemDef.get("type",        "Command");
                std::string enabled  = ItemDef.get("enabled_for", "");
                std::string visible  = ItemDef.get("visible_for", "");
                std::string questFor = ItemDef.get("question_for","");
                std::string question = ItemDef.get("question", "");
                std::string handler  = ItemDef.get("handler", "");
                std::string views    = ItemDef.get("view", "");
                std::string icon     = ItemDef.get("icon", "");
                std::string hidden   = ItemDef.get("hidden", "false");

                //std::cout << "  " << name << " :" << menuName << std::endl;

                UserMessage::message(UserMessage::DBG, false, std::string("  " + name));
                MenuItem *item = new MenuItem(name);
                item->setCommand(command);


                BaseNodeCondition *enabledCond = NodeExpressionParser::parseWholeExpression(enabled);
                if (enabledCond == NULL)
                {
                    UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse enabled condition: " + enabled));
                    enabledCond = new FalseNodeCondition();
                }
                item->setEnabledCondition(enabledCond);


                BaseNodeCondition *visibleCond = NodeExpressionParser::parseWholeExpression(visible);
                if (visibleCond == NULL)
                {
                    UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse visible condition: " + visible));
                    visibleCond = new FalseNodeCondition();
                }
                item->setVisibleCondition(visibleCond);

                BaseNodeCondition *questionCond = NodeExpressionParser::parseWholeExpression(questFor);
                if (questionCond == NULL)
                {
                    UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse question condition: " + questFor));
                    questionCond = new FalseNodeCondition();
                }
                item->setQuestionCondition(questionCond);


                item->setQuestion(question);
                item->setHandler(handler);
                item->setIcon(icon);

                if(!views.empty())
                {
                	std::vector<std::string> viewsVec;
                	QStringList vLst=QString::fromStdString(views).split("/");
                	for(int i=0; i < vLst.count(); i++)
                	{
                		viewsVec.push_back(vLst[i].toStdString());
                	}

                	item->setViews(viewsVec);
                }

                if(hidden == "true")
                	item->setHidden(true);

                if (type == "Submenu")
                    item->setAsSubMenu();

                addItemToMenu(item, menuName);
                //std::cout << "   added" << std::endl;


                // parse the valid node types/states for this menu item

                //if( ItemDef.count("valid_types") > 0 )  // does this node exist on the tree?
                //{
                //    ptree ptValidTypes = ItemDef.get_child("valid_types");
		        //    for (ptree::const_iterator itTypes = ptValidTypes.begin(); itTypes != ptValidTypes.end(); ++itTypes)
                //    {
                //        std::string type(itTypes->second.data());
                //        //item->addValidType(type);
                //    }
                //}
                //else
                //{
                //    //item->addValidType("all");
                //}


                if( ItemDef.count("valid_states") > 0 )  // does this node exist on the tree?
                {
                    ptree ptValidStates = ItemDef.get_child("valid_states");
		            for (ptree::const_iterator itStates = ptValidStates.begin(); itStates != ptValidStates.end(); ++itStates)
                    {
                        std::string state(itStates->second.data());
                        //item->addValidState(state);
                    }
                }
                else
                {
                    //item->addValidState("all");
                }
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
    for (std::vector<Menu *>::iterator itMenus = menus_.begin(); itMenus != menus_.end(); ++itMenus)
    {
        if ((*itMenus)->name() == name)
        {
            return (*itMenus);
        }
    }

    return NULL; // if we got to here, then the menu was not found
}

MenuItem* MenuHandler::findItem(QAction* ac)
{
	for(std::vector<Menu*>::iterator itMenus = menus_.begin(); itMenus != menus_.end(); ++itMenus)
	{
		for(std::vector<MenuItem*>::iterator it=(*itMenus)->items().begin(); it!=(*itMenus)->items().end(); ++it)
		{
			if((*it)->action() == ac)
			{
				return *it;
			}
		}
	}

	return NULL;
}

MenuItem* MenuHandler::newItem(const std::string &name)
{
	return NULL;
}

bool MenuHandler::addItemToMenu(MenuItem *item, const std::string &menuName)
{
    Menu *menu = findMenu(menuName);
    // items_.push_back(item); // add to our global list of menu items
    
    if (menu)
    {
        menu->addItem(item);
        return true;
    }
    else
    {
        UserMessage::message(UserMessage::ERROR, false, std::string("Could not find menu called " + 
                             menuName + " to add item " + item->name() + " to."));
        return false;
    }

    return false;
}


QAction *MenuHandler::invokeMenu(const std::string &menuName, std::vector<VInfo_ptr> nodes, QPoint pos, QWidget *parent,const std::string& view)
{
    QAction *selectedAction = NULL;
    Menu *menu = findMenu(menuName);

    if (menu)
    {
        QMenu *qMenu = menu->generateMenu(nodes, parent, view);

        if (qMenu)
        {
            selectedAction = qMenu->exec(pos);
            delete qMenu;
        }
    }

    return selectedAction;
}



// -----------------------------------------------------------------


///////////////////////////////////////////////////////////

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


QMenu *Menu::generateMenu(std::vector<VInfo_ptr> nodes, QWidget *parent,const std::string& view)
{
    QMenu *qmenu=new QMenu(parent);	
    qmenu->setTitle(QString::fromStdString(name()));

    if (nodes.empty())
        return NULL;

    //qmenu->setWindowFlags(Qt::Tool);
    //qmenu->setWindowTitle("my title");

    // add an inactive action(!) to the top of the menu in order to show which
    // node has been selected

    buildMenuTitle(nodes,qmenu);

    /*

    QLabel *nodeLabel = NULL;

    if (nodes.size() == 1)
    {
        //single node selected put a label with the node name + colour
        nodeLabel = new QLabel(QString::fromStdString((*nodes[0]).name()));

        QBrush bgBrush(nodes.at(0)->node()->stateColour());

        if(VProperty* p=VConfig::instance()->find("view.common.node_gradient"))
        {
            if(p->value().toBool())
            {
            	int lighter=150;
            	QColor bg=bgBrush.color();
            	QColor bgLight=bg.lighter(lighter);
            	QColor border=bg.darker(125);

            	QLinearGradient grad;
            	grad.setCoordinateMode(QGradient::ObjectBoundingMode);
            	grad.setStart(0,0);
            	grad.setFinalStop(0,1);

            	grad.setColorAt(0,bgLight);
                grad.setColorAt(1,bg);
            	bgBrush=QBrush(grad);
             }
        }

        QPalette labelPalette;
        labelPalette.setBrush(QPalette::Window,bgBrush);
        labelPalette.setColor(QPalette::WindowText,nodes.at(0)->node()->stateFontColour());//QColor(96,96,96));
        nodeLabel->setAutoFillBackground(true);
        nodeLabel->setPalette(labelPalette);

    	QString titleQss="QLabel {padding: 2px;}";
    	nodeLabel->setStyleSheet(titleQss);
    }
    else
    {
        // multiple nodes selected - say how many
        nodeLabel = new QLabel(QObject::tr("%1 nodes selected").arg(nodes.size()));
    }

    QFont menuTitleFont;
    menuTitleFont.setBold(true);
    menuTitleFont.setItalic(true);
    nodeLabel->setFont(menuTitleFont);
    nodeLabel->setAlignment(Qt::AlignHCenter);
    nodeLabel->setObjectName("nodeLabel");

    QWidget* titleW=new QWidget(qmenu);
    QVBoxLayout *titleLayout=new QVBoxLayout(titleW);
    titleLayout->setContentsMargins(2,2,2,2);
    titleLayout->addWidget(nodeLabel);
    nodeLabel->setParent(titleW);

    QWidgetAction *wAction = new QWidgetAction(qmenu);
    //Qt doc says: the ownership of the widget is passed to the widgetaction.
    //So when the action is deleted it will be deleted as well.
    wAction->setDefaultWidget(titleW);
    wAction->setEnabled(false);
    qmenu->addAction(wAction);


*/
    //TypeNodeCondition  typeCondFamily   (MenuItem::FAMILY);
    //TypeNodeCondition  typeCondTask     (MenuItem::TASK);
    //StateNodeCondition stateCondUnknown ("unknown");
    //OrNodeCondition    orCond           (&typeCondFamily, &typeCondTask);
    //AndNodeCondition   andCond          (&orCond, &stateCondUnknown);

    //std::string condString("not task");
    //BaseNodeCondition *nodeCond = NodeExpressionParser::parseWholeExpression(condString);

    //if (nodeCond == NULL)
    //{
    //    UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse condition: " + condString));
    //}



    for (std::vector<MenuItem*>::iterator itItems = items_.begin(); itItems != items_.end(); ++itItems)
    {
        //  is this item valid for the current selection?

    	if((*itItems)->hidden())
    		continue;

    	if(!(*itItems)->isValidView(view))
    		continue;

        bool visible = true;

        for (std::vector<VInfo_ptr>::iterator itNodes = nodes.begin(); itNodes != nodes.end(); ++itNodes)
        {
            //compatible = compatible && (*itItems)->compatibleWithNode(*itNodes);
            //compatible = compatible && (nodeCond != NULL && nodeCond->execute(*itNodes));
            visible = visible && (*itItems)->visibleCondition()->execute(*itNodes);
        }

        if (visible)
        {
            bool enabled = true;

            for (std::vector<VInfo_ptr>::iterator itNodes = nodes.begin(); itNodes != nodes.end(); ++itNodes)
            {
                enabled = enabled && (*itItems)->enabledCondition()->execute(*itNodes);
            }


            if ((*itItems)->isSubMenu())
            {
                //QMenu *subMenu = qmenu->addMenu(QString::fromStdString((*itItems)->name()));
                Menu *menu = MenuHandler::findMenu((*itItems)->name());
                if (menu)
                {
                    QMenu *subMenu = menu->generateMenu(nodes, 0, view);
                    qmenu->addMenu(subMenu);
                    subMenu->setEnabled(enabled);
                }
            }
            else if  ((*itItems)->isDivider())
            {
                qmenu->addSeparator();
            }
            else
            {
                //When we add the action to the menu its parent (NULL a.i. the QApplication) does not change.
            	//So when the menu is deleted the action is not deleted.
            	//At least this is the behaviour with Qt 4.8. and 5.5.
            	QAction *action = (*itItems)->action();
                qmenu->addAction(action);
                action->setEnabled(enabled);
            }
        }
    }

    return qmenu;
}

void Menu::buildMenuTitle(std::vector<VInfo_ptr> nodes, QMenu* qmenu)
{
	QLabel *nodeLabel = NULL;

	if (nodes.size() == 1)
	{
		VNode *node=nodes.at(0)->node();

		if(!node)
			return;

		//single node selected put a label with the node name + colour
		nodeLabel = new QLabel(node->name());

		QBrush bgBrush(node->stateColour());

		if(VProperty* p=VConfig::instance()->find("view.common.node_gradient"))
		{
			if(p->value().toBool())
			{
				int lighter=150;
				QColor bg=bgBrush.color();
				QColor bgLight=bg.lighter(lighter);
				QColor border=bg.darker(125);

				QLinearGradient grad;
				grad.setCoordinateMode(QGradient::ObjectBoundingMode);
				grad.setStart(0,0);
				grad.setFinalStop(0,1);

				grad.setColorAt(0,bgLight);
				grad.setColorAt(1,bg);
				bgBrush=QBrush(grad);
			}
		}

		QPalette labelPalette;
		labelPalette.setBrush(QPalette::Window,bgBrush);
		labelPalette.setColor(QPalette::WindowText,node->stateFontColour());//QColor(96,96,96));
		nodeLabel->setAutoFillBackground(true);
		nodeLabel->setPalette(labelPalette);

		QString titleQss="QLabel {padding: 2px;}";
		nodeLabel->setStyleSheet(titleQss);
	}
	else
	{
		// multiple nodes selected - say how many
		nodeLabel = new QLabel(QObject::tr("%1 nodes selected").arg(nodes.size()));
	}

	QFont menuTitleFont;
	menuTitleFont.setBold(true);
	menuTitleFont.setItalic(true);
	nodeLabel->setFont(menuTitleFont);
	nodeLabel->setAlignment(Qt::AlignHCenter);
	nodeLabel->setObjectName("nodeLabel");

	QWidget* titleW=new QWidget(qmenu);
	QVBoxLayout *titleLayout=new QVBoxLayout(titleW);
	titleLayout->setContentsMargins(2,2,2,2);
	titleLayout->addWidget(nodeLabel);
	nodeLabel->setParent(titleW);

	QWidgetAction *wAction = new QWidgetAction(qmenu);
	//Qt doc says: the ownership of the widget is passed to the widgetaction.
	//So when the action is deleted it will be deleted as well.
	wAction->setDefaultWidget(titleW);
	//wAction->setEnabled(false);
	qmenu->addAction(wAction);
}


// ------------------------
// MenuItem class functions
// ------------------------

MenuItem::MenuItem(const std::string &name) :
   name_(name),
   action_(0),
   hidden_(false),
   visibleCondition_(NULL),
   enabledCondition_(NULL),
   questionCondition_(NULL),
   isSubMenu_(false),
   isDivider_(false)
{
    if (name == "-")
    {
        isDivider_ = true;
    }
    else
    {
        action_ = new QAction(0);
        action_->setText(QString(name.c_str()));
    }
}

MenuItem::~MenuItem()
{
    if (action_)
        delete action_;
}

void MenuItem::setCommand(const std::string &command)
{
    command_ = command;

    if (action_)
        action_->setStatusTip(QString(command.c_str()));  // so we see the command in the status bar
}

void MenuItem::setHandler(const std::string& handler)
{
    handler_ = handler;
}

void MenuItem::setIcon(const std::string& icon)
{
	if(!icon.empty())
	{
		action_->setIcon(QPixmap(":/viewer/" + QString::fromStdString(icon)));
	}
}

bool MenuItem::shouldAskQuestion(std::vector<VInfo_ptr> &nodes)
{
    bool askQuestion = false;

    // ask the question if any of the nodes require it
    for (std::vector<VInfo_ptr>::iterator itNodes = nodes.begin(); itNodes != nodes.end(); ++itNodes)
    {
        askQuestion = askQuestion || questionCondition()->execute(*itNodes);
    }

    return askQuestion;
}

bool MenuItem::isValidView(const std::string& view) const
{
	if(views_.empty())
		return true;

	return (std::find(views_.begin(),views_.end(),view) != views_.end());
}


// // adds an entry to the list of valid node types for this menu item
// void MenuItem::addValidType(std::string type)
// {
//     static NodeType all[] = {TASK, FAMILY, SUITE, SERVER, ALIAS};
// 
//     if     (type == "server")
//         validNodeTypes_.push_back(SERVER);
//     else if(type == "suite")
//         validNodeTypes_.push_back(SUITE);
//     else if(type == "task")
//         validNodeTypes_.push_back(TASK);
//     else if(type == "family")
//         validNodeTypes_.push_back(FAMILY);
//     else if(type == "alias")
//         validNodeTypes_.push_back(ALIAS);
//     else if(type == "all")
//         validNodeTypes_.insert(validNodeTypes_.begin(), all, all+5);
// }
// 
// 
// // adds an entry to the list of valid node types for this menu item
// void MenuItem::addValidState(std::string state)
// {
//     DState::State dstate;
// 
//     if (DState::isValid(state))
//     {
//         dstate = DState::toState(state);
//         validNodeStates_.push_back(dstate);
//     }
//     else if (state == "all")
//     {
//         // add the list of all states
//         std::vector<DState::State> allDstates = DState::states();
//         validNodeStates_.insert(validNodeStates_.end(), allDstates.begin(), allDstates.end());
//     }
//     else
//     {
//         UserMessage::message(UserMessage::ERROR, false, std::string("Bad node state in menu file: " + state));
//     }
// }


// bool MenuItem::isNodeTypeValidForMenuItem(NodeType type)
// {
//     if(std::find(validNodeTypes_.begin(), validNodeTypes_.end(), type) == validNodeTypes_.end())
//         return false;
//     else
//         return true;
// }
// 
// 
// bool MenuItem::compatibleWithNode(VInfo_ptr nodeInfo)
// {
//     // check each node type and return false if we don't match
// 
//     if(nodeInfo->isServer())
//         if(std::find(validNodeTypes_.begin(), validNodeTypes_.end(), SERVER) == validNodeTypes_.end())
//             return false;
// 
//     if(nodeInfo->isNode())
//     {
//         Node *node = nodeInfo->node()->node();
// 
//         if(node->isSuite())
//             if (!isNodeTypeValidForMenuItem(SUITE))
//                 return false;
// 
//         if(node->isTask())
//             if (!isNodeTypeValidForMenuItem(TASK))
//                 return false;
// 
//         if(node->isAlias())
//             if (!isNodeTypeValidForMenuItem(ALIAS))
//                 return false;
// 
//         if(node->isFamily())
//             if (!isNodeTypeValidForMenuItem(FAMILY))
//                 return false;
//     }
// 
//     return true;
// }
