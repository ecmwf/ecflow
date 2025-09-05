/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cassert>
#include <iostream>

#include <QDebug>
#include <QLabel>
#include <QLinearGradient>
#include <QMenu>
#include <QMessageBox>
#include <QObject>
#include <QShortcut>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
    #include <QRegExp>
#endif
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include "CustomCommandHandler.hpp"
#include "MenuHandler.hpp"
#include "ServerHandler.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"
#include "UserMessage.hpp"
#include "VConfig.hpp"
#include "VNode.hpp"
#include "VProperty.hpp"
#include "ViewerUtil.hpp"
#include "ecflow/core/Str.hpp"

int MenuItem::idCnt_ = 0;

std::vector<Menu*> MenuHandler::menus_;
MenuHandler::ConfirmationMap MenuHandler::commandsWhichRequireConfirmation_;
TrueNodeCondition MenuHandler::trueCond_;
FalseNodeCondition MenuHandler::falseCond_;

MenuHandler::MenuHandler() {
    menus_.clear();
}

// ---------------------------------------------------------
// MenuHandler::readMenuConfigFile
// Read the given config file and store the resulting menus
// internally.
// ---------------------------------------------------------

bool MenuHandler::readMenuConfigFile(const std::string& configFile) {
    // parse the response using the boost JSON property tree parser

    using boost::property_tree::ptree;
    ptree pt;

    try {
        read_json(configFile, pt);
    }
    catch (const boost::property_tree::json_parser::json_parser_error& e) {
        std::string errorMessage = e.what();
        UserMessage::message(
            UserMessage::ERROR, true, std::string("Error, unable to parse JSON menu file : " + errorMessage));
        return false;
    }

    // iterate over the top level of the tree

    for (ptree::const_iterator itTopLevel = pt.begin(); itTopLevel != pt.end(); ++itTopLevel) {
        // parse the menu definitions?

        if (itTopLevel->first == "menus") {
            UiLog().dbg() << "Menus:";

            ptree const& menusDef = itTopLevel->second;

            // iterate through all the menus

            for (ptree::const_iterator itMenus = menusDef.begin(); itMenus != menusDef.end(); ++itMenus) {
                ptree const& menuDef = itMenus->second;

                std::string cname = menuDef.get("name", "NoName");
                UiLog().dbg() << "  " << cname;
                auto* menu = new Menu(cname);

                // ptree const &menuModesDef = menuDef.get_child("modes");

                // for (ptree::const_iterator itMenuModes = menuModesDef.begin(); itMenuModes != menuModesDef.end();
                // ++itMenuModes)
                //{
                //    std::cout << "   +" << itMenuModes->second.data() << std::endl;
                //}

                std::string parentMenuName = menuDef.get("parent", "None");

                if (parentMenuName != "None") {}

                addMenu(menu); // add to our list of available menus
            }
        }

        // parse the menu items?

        else if (itTopLevel->first == "menu_items") {
            UiLog().dbg() << "Menu items:";

            ptree const& itemsDef = itTopLevel->second;

            // iterate through all the items

            for (ptree::const_iterator itItems = itemsDef.begin(); itItems != itemsDef.end(); ++itItems) {
                ptree const& ItemDef = itItems->second;

                std::string name              = ItemDef.get("name", "NoName");
                std::string menuName          = ItemDef.get("menu", "NoMenu");
                std::string command           = ItemDef.get("command", "NoCommand");
                std::string type              = ItemDef.get("type", "Command");
                std::string enabled           = ItemDef.get("enabled_for", "");
                std::string visible           = ItemDef.get("visible_for", "");
                std::string questFor          = ItemDef.get("question_for", "");
                std::string question          = ItemDef.get("question", "");
                std::string questionControl   = ItemDef.get("question_control", "");
                std::string warning           = ItemDef.get("warning", "");
                std::string handler           = ItemDef.get("handler", "");
                std::string views             = ItemDef.get("view", "");
                std::string icon              = ItemDef.get("icon", "");
                std::string hidden            = ItemDef.get("hidden", "false");
                std::string multiSelect       = ItemDef.get("multi", "true");
                std::string statustip         = ItemDef.get("status_tip", "");
                std::string shortcut          = ItemDef.get("shortcut", "");
                std::string panelPopupControl = ItemDef.get("panel_control", "");

                // std::cout << "  " << name << " :" << menuName << std::endl;

                UiLog().dbg() << "  " << name;
                auto* item = new MenuItem(name);
                item->setCommand(command);

                BaseNodeCondition* enabledCond = NodeExpressionParser::instance()->parseWholeExpression(enabled);
                if (enabledCond == nullptr) {
                    UserMessage::message(
                        UserMessage::ERROR, true, std::string("Error, unable to parse enabled condition: " + enabled));
                    enabledCond = new FalseNodeCondition();
                }
                item->setEnabledCondition(enabledCond);

                BaseNodeCondition* visibleCond = NodeExpressionParser::instance()->parseWholeExpression(visible);
                if (visibleCond == nullptr) {
                    UserMessage::message(
                        UserMessage::ERROR, true, std::string("Error, unable to parse visible condition: " + visible));
                    visibleCond = new FalseNodeCondition();
                }
                item->setVisibleCondition(visibleCond);

                BaseNodeCondition* questionCond = NodeExpressionParser::instance()->parseWholeExpression(questFor);
                if (questionCond == nullptr) {
                    UserMessage::message(UserMessage::ERROR,
                                         true,
                                         std::string("Error, unable to parse question condition: " + questFor));
                    questionCond = new FalseNodeCondition();
                }
                item->setQuestionCondition(questionCond);

                item->setQuestion(question);
                item->setQuestionControl(questionControl);
                item->setWarning(warning);
                item->setHandler(handler);
                item->setIcon(icon);
                item->setStatustip(statustip);
                item->setShortcut(shortcut);
                item->setPanelPopupControl(panelPopupControl);

                if (!views.empty()) {
                    std::vector<std::string> viewsVec;
                    QStringList vLst = QString::fromStdString(views).split("/");
                    for (int i = 0; i < vLst.count(); i++) {
                        viewsVec.push_back(vLst[i].toStdString());
                    }

                    item->setViews(viewsVec);
                }

                item->setHidden((hidden == "true") ? true : false);
                item->setMultiSelect((multiSelect == "true") ? true : false);

                if (type == "Submenu") {
                    item->setAsSubMenu();
                }

                addItemToMenu(item, menuName);
                // std::cout << "   added" << std::endl;

                // parse the valid node types/states for this menu item

                // if( ItemDef.count("valid_types") > 0 )  // does this node exist on the tree?
                //{
                //     ptree ptValidTypes = ItemDef.get_child("valid_types");
                //     for (ptree::const_iterator itTypes = ptValidTypes.begin(); itTypes != ptValidTypes.end();
                //     ++itTypes)
                //    {
                //        std::string type(itTypes->second.data());
                //        //item->addValidType(type);
                //    }
                //}
                // else
                //{
                //    //item->addValidType("all");
                //}

                if (ItemDef.count("valid_states") > 0) // does this node exist on the tree?
                {
                    ptree ptValidStates = ItemDef.get_child("valid_states");
                    for (ptree::const_iterator itStates = ptValidStates.begin(); itStates != ptValidStates.end();
                         ++itStates) {
                        std::string state(itStates->second.data());
                        // item->addValidState(state);
                    }
                }
                else {
                    // item->addValidState("all");
                }
            }
        }
    }

    // ptree ptMenus = pt.get_child("menus");

    // for (ptree::const_iterator itTopLevel = pt.begin(); itTopLevel != pt.end(); ++itTopLevel)
    //{
    //    if (itTopLevel->first == "menus")

    //}

    return true;
}

// ---------------------------------------------------------
// MenuHandler::addCustomMenuCommands
// Obtains the current list of custom commands and adds them
// to the list of custom menu items.
// ---------------------------------------------------------

void MenuHandler::refreshCustomMenuCommands() {
    CustomCommandHistoryHandler* customRecentCmds = CustomCommandHistoryHandler::instance();
    CustomSavedCommandHandler* customSavedCmds    = CustomSavedCommandHandler::instance();

    Menu* menu = findMenu("User defined");
    if (menu) {
        menu->clearFixedList();

        // create the 'compulsary' menu items
        auto* item1 = new MenuItem("Manage commands...");
        item1->setCommand("custom");
        menu->addItemToFixedList(item1);
        item1->setEnabledCondition(&trueCond_);
        item1->setVisibleCondition(&trueCond_);
        item1->setQuestionCondition(&falseCond_);
        item1->setIcon("configure.svg");

        // Saved commands
        auto* item2 = new MenuItem("-");
        menu->addItemToFixedList(item2);
        item2->setEnabledCondition(&trueCond_);
        item2->setVisibleCondition(&trueCond_);
        item2->setQuestionCondition(&falseCond_);

        int numSavedCommands = customSavedCmds->numCommands();

        for (int i = 0; i < numSavedCommands; i++) {
            CustomCommand* cmd = customSavedCmds->commandFromIndex(i);
            if (cmd->inContextMenu()) {
                auto* item = new MenuItem(cmd->name());
                item->setCommand(cmd->command());
                item->setEnabledCondition(&trueCond_);
                item->setVisibleCondition(&trueCond_);
                item->setQuestionCondition(&trueCond_);
                item->setCustom(true);
                item->setStatustip("__cmd__");
                menu->addItemToFixedList(item);
            }
        }

        // Recently executed commands
        auto* item3 = new MenuItem("-");
        menu->addItemToFixedList(item3);
        item3->setEnabledCondition(&trueCond_);
        item3->setVisibleCondition(&trueCond_);
        item3->setQuestionCondition(&falseCond_);

        auto* item4 = new MenuItem("Recent");
        menu->addItemToFixedList(item4);
        item4->setEnabledCondition(&falseCond_);
        item4->setVisibleCondition(&trueCond_);
        item4->setQuestionCondition(&falseCond_);

        int numRecentCommands = customRecentCmds->numCommands();

        for (int i = 0; i < numRecentCommands; i++) {
            CustomCommand* cmd = customRecentCmds->commandFromIndex(i);

            auto* item = new MenuItem(cmd->name());
            item->setCommand(cmd->command());
            item->setEnabledCondition(&trueCond_);
            item->setVisibleCondition(&trueCond_);
            item->setQuestionCondition(&trueCond_);
            item->setCustom(true);
            item->setStatustip("__cmd__");
            menu->addItemToFixedList(item);
        }
    }
}

Menu* MenuHandler::findMenu(const std::string& name) {
    for (auto& menu : menus_) {
        if (menu->name() == name) {
            return menu;
        }
    }

    return nullptr; // if we got to here, then the menu was not found
}

MenuItem* MenuHandler::findItem(QAction* ac) {
    // ac could be NULL, e.g. if the user clicked on a separator instead of a menu item
    if (ac) {
        for (auto& menu : menus_) {
            for (auto it : menu->items()) {
                if (it->id() == ac->data().toInt()) {
                    return it;
                }
            }
        }
    }

    return nullptr;
}

MenuItem* MenuHandler::findItemById(int id) {
    for (auto& menu : menus_) {
        for (auto& item : menu->itemsFixed()) {
            if (item->id() == id) {
                return item;
            }
        }
    }

    return nullptr;
}

MenuItem* MenuHandler::newItem(const std::string& /*name*/) {
    return nullptr;
}

bool MenuHandler::addItemToMenu(MenuItem* item, const std::string& menuName) {
    Menu* menu = findMenu(menuName);
    // items_.push_back(item); // add to our global list of menu items

    if (menu) {
        menu->addItemToFixedList(item);
        return true;
    }
    else {
        UiLog().err() << "Could not find menu called " << menuName << " to add item " << item->name() << " to.";
        return false;
    }

    return false;
}

MenuItem* MenuHandler::invokeMenu(const std::string& menuName,
                                  std::vector<VInfo_ptr> nodes,
                                  QPoint pos,
                                  QWidget* parent,
                                  const std::string& view) {
    MenuItem* selectedItem = nullptr;
    Menu* menu             = findMenu(menuName);

    if (menu) {
        QList<QAction*> acLst;

        // While create the menus we collect all the actions created with "parent" as the parent.
        // QMenu does not take ownership of these actions so we need to delete them.
        QMenu* qMenu = menu->generateMenu(nodes, parent, nullptr, view, acLst);

        if (qMenu) {
            QAction* selectedAction = qMenu->exec(pos);
            selectedItem            = MenuHandler::findItem(selectedAction);

            delete qMenu;

            // Delete all the actions with "parent" as the parent;
            Q_FOREACH (QAction* ac, acLst) {
                assert(parent == ac->parent());
                delete ac;
            }
        }
    }

    return selectedItem;
}

MenuHandler::ConfirmationMap& MenuHandler::getCommandsThatRequireConfirmation() {
    // populate the list only the first time this function is called
    if (commandsWhichRequireConfirmation_.empty()) {
        // list the commands which require a prompt:
        commandsWhichRequireConfirmation_["delete"]    = "Do you really want to delete <full_name> ?";
        commandsWhichRequireConfirmation_["terminate"] = "Do you really want to terminate <full_name> ?";
        commandsWhichRequireConfirmation_["halt"]      = "Do you really want to halt <full_name> ?";
    }
    return commandsWhichRequireConfirmation_;
}

// some commands, such as --delete, prompt the user for confirmation on the command line, which
// causes the application to hang. The way we get around this is to intercept these commands and,
// where possible, add a "yes" argument, which will bypass the prompt.
void MenuHandler::interceptCommandsThatNeedConfirmation(MenuItem* item) {
    std::string command = item->command();
    QString wholeCmd    = QString::fromStdString(command);

    // find the verb in the command
    QString commandName;
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    QRegularExpression rx("ecflow_client\\s+--([a-zA-Z]+).*");
    auto match = rx.match(wholeCmd);
    if (match.hasMatch()) {
        commandName = match.captured(1);
    }
#else
    QRegExp rx("ecflow_client\\s+--([a-zA-Z]+).*"); //  \s=whitespace, \S=non-whitespace
    int i = rx.indexIn(wholeCmd);
    if (i != -1) {
        QString commandName = rx.cap(1);
    }
#endif
    if (!commandName.isEmpty()) {
        std::string cmdName = commandName.toStdString();

        // is this command one of the ones that requires a prompt?
        MenuHandler::ConfirmationMap& list = getCommandsThatRequireConfirmation();
        auto it                            = list.find(cmdName);
        if (it != list.end()) {
            // does the command already have a 'yes'?
            QRegularExpression rx2(".*\\byes\\b.*"); // \b=word boundary
            if (!wholeCmd.contains(rx2)) {
                item->setQuestion((*it).second); // note that we need to ask the user

                // fix the command so that it has "yes" in it
                std::string minusCmd     = std::string("--") + cmdName;
                std::string cmdEquals    = minusCmd + "=";
                std::string cmdEqualsYes = cmdEquals + "yes ";
                std::string cmdYes       = minusCmd + " yes ";
                if (!ecf::Str::replace(command, cmdEquals, cmdEqualsYes)) // --command=foo -> --command=yes foo
                {
                    ecf::Str::replace(command, minusCmd, cmdYes); // --command foo -> --command yes foo
                }
                item->setCommand(command);
            }
        }
    }
}

void MenuHandler::setupShortcut(QObject* receiver, QWidget* w, const std::string& view) {
    for (auto m : menus_) {
        for (auto item : m->itemsFixed()) {
            if (QShortcut* sc = item->createShortcut(w, view)) {
                QObject::connect(sc, SIGNAL(activated()), receiver, SLOT(slotCommandShortcut()));
            }
        }
    }
}

// -----------------------------------------------------------------
///////////////////////////////////////////////////////////

// --------------------
// Menu class functions
// --------------------

Menu::Menu(const std::string& name) : name_(name) {
}

Menu::~Menu() {
    for (auto& itItems : itemsCombined_) {
        if (itItems) {
            delete itItems;
        }
    }
}

QMenu* Menu::generateMenu(std::vector<VInfo_ptr> nodes,
                          QWidget* parent,
                          QMenu* parentMenu,
                          const std::string& view,
                          QList<QAction*>& acLst) {
    QMenu* qmenu = nullptr;
    if (parentMenu) {
        qmenu = parentMenu->addMenu(QString::fromStdString(name()));
    }
    else {
        qmenu = new QMenu(parent);
        qmenu->setObjectName("cm");
        qmenu->setTitle(QString::fromStdString(name()));
    }

    if (nodes.empty()) {
        return nullptr;
    }

    // qmenu->setWindowFlags(Qt::Tool);
    // qmenu->setWindowTitle("my title");

    // TypeNodeCondition  typeCondFamily   (MenuItem::FAMILY);
    // TypeNodeCondition  typeCondTask     (MenuItem::TASK);
    // StateNodeCondition stateCondUnknown ("unknown");
    // OrNodeCondition    orCond           (&typeCondFamily, &typeCondTask);
    // AndNodeCondition   andCond          (&orCond, &stateCondUnknown);

    // std::string condString("not task");
    // BaseNodeCondition *nodeCond = NodeExpressionParser::parseWholeExpression(condString);

    // if (nodeCond == NULL)
    //{
    //     UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse condition: " +
    //     condString));
    // }

    // add an inactive action(!) to the top of the menu in order to show which
    // node has been selected

    buildMenuTitle(nodes, qmenu);

    // if multiple attributes are selected, then tell the user we can't help them
    // NOTE that ActionHandler.cpp ensures that we cannot have a mix of attr and non-attr nodes
    if (nodes[0]->isAttribute() && nodes.size() > 1) {
        auto* noAction = new QAction("No action for multiple attributes", parent);
        noAction->setEnabled(false);
        qmenu->addAction(noAction);
        return qmenu;
    }

    // merge the fixed menu items (from the config file) with the dynamic ones
    itemsCombined_ = itemsFixed_;
    itemsCombined_.insert(itemsCombined_.end(), itemsCustom_.begin(), itemsCustom_.end());

    for (auto& itItems : itemsCombined_) {
        //  is this item valid for the current selection?

        if (itItems->hidden()) {
            continue;
        }

        if (!itItems->isValidView(view)) {
            continue;
        }

        // Control if some items are shown
        if (!itItems->panelPopupControl().empty()) {
            if (VProperty* prop = VConfig::instance()->find(itItems->panelPopupControl())) {
                if (!prop->valueAsString().contains(QString::fromStdString(itItems->command()), Qt::CaseSensitive)) {
                    continue;
                }
            }
        }

        bool visible = true;
        for (auto& node : nodes) {
            // compatible = compatible && (*itItems)->compatibleWithNode(*itNodes);
            // compatible = compatible && (nodeCond != NULL && nodeCond->execute(*itNodes));
            visible = visible && itItems->visibleCondition()->execute(node);
        }

        if (visible) {
            bool enabled = true;

            for (auto& node : nodes) {
                enabled = enabled && itItems->enabledCondition()->execute(node);
            }

            // Check multiple selection
            if (nodes.size() > 1 && !itItems->multiSelect()) {
                enabled = false;
            }

            if (itItems->isSubMenu()) {
                Menu* menu = MenuHandler::findMenu(itItems->name());
                if (menu) {
                    // The submenu will be added to qmenu and it will take ownership of it.
                    QMenu* subMenu = menu->generateMenu(nodes, parent, qmenu, view, acLst);
                    subMenu->setEnabled(enabled);
                }
            }
            else if (itItems->isDivider()) {
                qmenu->addSeparator();
            }
            else {
                // When we add the action to the menu its parent (NULL e.i. the QApplication) does not change.
                // So when the menu is deleted the action is not deleted.
                // At least this is the behaviour with Qt 4.8. and 5.5.
                // QAction *action = (*itItems)->action();
                // action->setParent(parent);

                // These actions will have "parent" as the parent, otherwise the statustip would not work
                // on qmainwindows. The downside is that we need to delete these actions separately when the qmenu is
                // deleted. In theory the parent of the actions could be the qmenu as well, but in this case the
                // statustip does not work!
                QAction* action = itItems->createAction(parent);
                qmenu->addAction(action);
                action->setEnabled(enabled);
                acLst << action;
            }
        }
    }

    return qmenu;
}

/*
void Menu::addSubHeading(std::string &name)
{
    QLabel *nodeLabel = new QLabel(name);

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
*/
void Menu::buildMenuTitle(std::vector<VInfo_ptr> nodes, QMenu* qmenu) {
    QLabel* nodeLabel = nullptr;

    // we will only create a multiple-entry context menu if we have multiple non-attribute nodes
    // it is already ensured that if we have multiple nodes, they will be non-attribute nodes
    bool multiple = (nodes.size() > 1);

    if (!multiple) {
        VNode* node = nodes.at(0)->node();

        if (!node) {
            return;
        }

        // single node selected put a label with the node name + colour
        nodeLabel = new QLabel(node->name());

        QBrush bgBrush(node->stateColour());

        if (VProperty* p = VConfig::instance()->find("view.common.node_gradient")) {
            if (p->value().toBool()) {
                int lighter    = 150;
                QColor bg      = bgBrush.color();
                QColor bgLight = bg.lighter(lighter);

                QLinearGradient grad;
                grad.setCoordinateMode(QGradient::ObjectBoundingMode);
                grad.setStart(0, 0);
                grad.setFinalStop(0, 1);

                grad.setColorAt(0, bgLight);
                grad.setColorAt(1, bg);
                bgBrush = QBrush(grad);
            }
        }

        QPalette labelPalette;
        labelPalette.setBrush(QPalette::Window, bgBrush);
        labelPalette.setColor(QPalette::WindowText, node->stateFontColour()); // QColor(96,96,96));
        nodeLabel->setAutoFillBackground(true);
        nodeLabel->setPalette(labelPalette);

        QString titleQss = "QLabel {padding: 2px;}";
        nodeLabel->setStyleSheet(titleQss);
    }
    else {
        // multiple nodes selected - say how many
        nodeLabel = new QLabel(QObject::tr("%1 nodes selected").arg(nodes.size()));
    }

    QFont menuTitleFont;
    menuTitleFont.setBold(true);
    menuTitleFont.setItalic(true);
    nodeLabel->setFont(menuTitleFont);
    nodeLabel->setAlignment(Qt::AlignHCenter);
    nodeLabel->setObjectName("nodeLabel");

    auto* titleW      = new QWidget(qmenu);
    auto* titleLayout = new QVBoxLayout(titleW);
    titleLayout->setContentsMargins(2, 2, 2, 2);
    titleLayout->addWidget(nodeLabel);
    nodeLabel->setParent(titleW);

    auto* wAction = new QWidgetAction(qmenu);
    wAction->setObjectName("title");
    // Qt doc says: the ownership of the widget is passed to the widgetaction.
    // So when the action is deleted it will be deleted as well.
    wAction->setDefaultWidget(titleW);
    wAction->setEnabled(false);
    qmenu->addAction(wAction);
}

// ------------------------
// MenuItem class functions
// ------------------------

MenuItem::MenuItem(const std::string& name)
    : name_(name),
      id_(idCnt_++),
      hidden_(false),
      multiSelect_(true),
      visibleCondition_(nullptr),
      enabledCondition_(nullptr),
      questionCondition_(nullptr),
      isSubMenu_(false),
      isDivider_(false),
      isCustom_(false) {
    if (name == "-") {
        isDivider_ = true;
    }
}

MenuItem::~MenuItem() = default;

void MenuItem::setCommand(const std::string& command) {
    command_ = command;

    // if (action_)
    //     action_->setStatusTip(QString(command.c_str()));  // so we see the command in the status bar
}

void MenuItem::setHandler(const std::string& handler) {
    handler_ = handler;
}

void MenuItem::setIcon(const std::string& icon) {
    if (!icon.empty()) {
        icon_ = QIcon(QPixmap(":/viewer/" + QString::fromStdString(icon)));
    }
}

bool MenuItem::shouldAskQuestion(const std::vector<VInfo_ptr>& nodes) const {
    bool askQuestion = false;

    // ask the question if any of the nodes require it
    for (auto& node : nodes) {
        askQuestion = askQuestion || questionCondition()->execute(node);
    }

    return askQuestion;
}

bool MenuItem::isValidView(const std::string& view) const {
    if (views_.empty()) {
        return true;
    }

    return (std::find(views_.begin(), views_.end(), view) != views_.end());
}

QAction* MenuItem::createAction(QWidget* parent) {
    auto* ac = new QAction(parent);
    ac->setObjectName(QString::fromStdString(name_));
    ac->setText(QString::fromStdString(name_));
    ac->setIcon(icon_);

    if (!shortcut_.empty()) {
        ac->setShortcut(QKeySequence(QString::fromStdString(shortcut_)));
        ViewerUtil::showShortcutInContextMenu(ac);
    }

    if (!statustip_.empty()) {
        if (statustip_ == "__cmd__") {
            ac->setStatusTip(QString::fromStdString(command_)); // so we see the command in the status bar
        }
        else {
            ac->setStatusTip(QString::fromStdString(statustip_));
        }
    }
    ac->setData(id_);
    return ac;
}

QShortcut* MenuItem::createShortcut(QWidget* parent, const std::string& view) {
    if (!shortcut_.empty() && isValidView(view)) {
        auto* sc = new QShortcut(QString::fromStdString(shortcut_), parent);
        sc->setContext(Qt::WidgetShortcut);
        sc->setProperty("id", id_);
        return sc;
    }
    return nullptr;
}

bool MenuItem::isValidFor(const std::vector<VInfo_ptr>& nodes, bool allowHidden) const {
    if (!allowHidden && hidden()) {
        return false;
    }

    if (nodes.size() == 0 || (nodes.size() > 1 && !multiSelect())) {
        return false;
    }

    for (auto& node : nodes) {
        if (!visibleCondition()->execute(node)) {
            return false;
        }
    }

    for (auto& node : nodes) {
        if (!enabledCondition()->execute(node)) {
            return false;
        }
    }

    return true;
}
