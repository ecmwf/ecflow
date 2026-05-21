/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "InfoPanelHandler.hpp"

#include "NodeExpression.hpp"
#include "UiLog.hpp"
#include "UserMessage.hpp"
#include "ecflow/core/PTree.hpp"

InfoPanelHandler* InfoPanelHandler::instance_ = nullptr;

InfoPanelDef::InfoPanelDef(const std::string& name)
    : name_(name),
      hidden_(false),
      visibleCondition_(nullptr),
      enabledCondition_(nullptr) {
}

InfoPanelHandler::InfoPanelHandler() = default;

InfoPanelHandler* InfoPanelHandler::instance() {
    if (!instance_) {
        instance_ = new InfoPanelHandler();
    }

    return instance_;
}

void InfoPanelHandler::init(const std::string& configFile) {
    // parse the response using the boost JSON property tree parser

    ecf::PTree pt;

    try {
        read_json(configFile, pt);
    }
    catch (const ecf::PTreeParseError& e) {
        std::string errorMessage = e.what();
        UserMessage::message(
            UserMessage::ERROR, true, std::string("Error, unable to parse JSON menu file : " + errorMessage));
        return;
    }

    // iterate over the top level of the tree
    for (auto& [topLevelName, topLevelValue] : pt) {
        if (topLevelName == "info_panel") {
            UiLog().dbg() << "Panels:";

            // iterate through all the panels
            for (auto& [_, panelValue] : topLevelValue) {
                std::string cname = panelValue.get("name", "");

                UiLog().dbg() << "  " << cname;

                auto* def = new InfoPanelDef(cname);

                def->setLabel(panelValue.get("label", ""));
                def->setIcon(panelValue.get("icon", ""));
                def->setDockIcon(panelValue.get("dock_icon", ""));
                def->setShow(panelValue.get("show", ""));
                def->setTooltip(panelValue.get("tooltip", ""));
                def->setButtonTooltip(panelValue.get("button_tooltip", ""));

                std::string enabled = panelValue.get("enabled_for", "");
                std::string visible = panelValue.get("visible_for", "");

                if (panelValue.get("hidden", "") == "1") {
                    def->setHidden(true);
                }

                BaseNodeCondition* enabledCond = NodeExpressionParser::instance()->parseWholeExpression(enabled);
                if (enabledCond == nullptr) {
                    UserMessage::message(
                        UserMessage::ERROR, true, std::string("Error, unable to parse enabled condition: " + enabled));
                    enabledCond = new FalseNodeCondition();
                }
                def->setEnabledCondition(enabledCond);

                BaseNodeCondition* visibleCond = NodeExpressionParser::instance()->parseWholeExpression(visible);
                if (visibleCond == nullptr) {
                    UserMessage::message(
                        UserMessage::ERROR, true, std::string("Error, unable to parse visible condition: " + visible));
                    visibleCond = new FalseNodeCondition();
                }
                def->setVisibleCondition(visibleCond);

                panels_.push_back(def);
            }
        }
    }
}

void InfoPanelHandler::visible(VInfo_ptr info, std::vector<InfoPanelDef*>& lst) {
    if (!info || !info.get()) {
        return;
    }

    for (const auto& panel : panels_) {
        if (!panel->hidden() && panel->visibleCondition()->execute(info)) {
            lst.push_back(panel);
        }
    }
}
